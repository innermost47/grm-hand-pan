// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The networks of a handpan's tone fields: the losses of the tuned modes, solved from the measured decays, and one
// tone field, from the note it is tuned to and the steel it is made of.
#pragma once

#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "../constants/core.hpp"
#include "../constants/decay_measurements.hpp"
#include "../models/core.hpp"
#include "grm.hpp"

namespace grmh
{
namespace detail
{
inline std::string mode_name(const char* prefix, int mode) { return prefix + std::to_string(mode); }

// log10(T60) = log10(T60 at the reference) + slope * log10(f1 / reference), f1 the field's fundamental: the
// relation the loss law and the tone field share
inline void add_decay_relation(grm::Network& net, const std::string& name, const std::string& t60,
                               const std::string& t60_at_reference, const std::string& slope,
                               const std::string& fundamental)
{
    const int it = net.index(t60), ir = net.index(t60_at_reference), is = net.index(slope),
              iff = net.index(fundamental);
    net.add_fn(
        name, {t60, t60_at_reference, slope, fundamental},
        [=](const grm::Values& v)
        { return std::log10(v[it]) - std::log10(v[ir]) - v[is] * std::log10(v[iff] / LOSS_REFERENCE_FREQUENCY); },
        1e-9);
}

inline Measured measured(const grm::Result& r, const grm::Network& net, const std::string& node)
{
    const auto i = static_cast<size_t>(net.index(node));
    return {r.values[i], r.uncertainty[i]};
}
} // namespace detail

// the losses of the tuned modes, from the 84 T60 of [A15]: one T60 at the reference per mode, and how the T60 of a
// field changes with its fundamental. Solved once: its inputs are measurements.
inline LossLaw solve_loss_law()
{
    grm::Network net;
    for (int mode = 1; mode <= TUNED_MODES; ++mode)
        net.add_node(detail::mode_name("t60_ref_", mode), DECAY_ALL.mean);
    net.add_node("slope", 0.0, true);

    const double field_spread_dex = DECAY_ALL.standard_deviation / DECAY_ALL.mean / grm::LN10;
    std::map<std::string, grm::Measurement> measured;
    for (size_t i = 0; i < DECAY_MEASUREMENTS.size(); ++i)
    {
        const auto& m = DECAY_MEASUREMENTS[i];
        const std::string t60 = "t60_" + std::to_string(i), fundamental = "f1_" + std::to_string(i);
        net.add_node(t60, m.t60);
        net.add_node(fundamental, m.frequency / m.mode);
        detail::add_decay_relation(net, "decay of measured field " + std::to_string(i), t60,
                                   detail::mode_name("t60_ref_", m.mode), "slope", fundamental);
        measured[t60] = {m.t60, field_spread_dex};
        measured[fundamental] = {m.frequency / m.mode, 1e-9}; // the field's fundamental, from its mode
    }
    const auto r = net.solve(measured);

    LossLaw law{};
    for (int mode = 1; mode <= TUNED_MODES; ++mode)
        law.t60_at_reference[(size_t)mode - 1] = detail::measured(r, net, detail::mode_name("t60_ref_", mode));
    law.slope = r["slope"];
    law.slope_sigma = r.uncertainty[(size_t)net.index("slope")];
    law.reference_frequency = LOSS_REFERENCE_FREQUENCY;
    return law;
}

// the law solved once for the whole program (its inputs never change)
inline const LossLaw& loss_law()
{
    static const LossLaw law = solve_loss_law();
    return law;
}

// one tone field tuned to `fundamental` (Hz): its tuned modes (tuning 1:2:3 within the measured spread), how long
// each rings (the loss law), and the radius of the steel plate that rings at that fundamental
inline ToneField solve_tone_field(double fundamental, const LossLaw& law = loss_law())
{
    grm::Network net;
    const char* frequency_of[TUNED_MODES] = {"f1", "f2", "f3"};
    const double ratio_of[TUNED_MODES] = {1.0, OCTAVE_RATIO, COMPOUND_FIFTH_RATIO};
    for (int m = 0; m < TUNED_MODES; ++m)
        net.add_node(frequency_of[m], fundamental * ratio_of[m]);
    for (auto [name, guess] : {std::pair{"E", STEEL_YOUNG_MODULUS.value}, {"rho", STEEL_DENSITY.value},
                               {"nu", STEEL_POISSON_RATIO.value}, {"nu_term", 0.9}, {"h", SHELL_THICKNESS.value},
                               {"lambda2", 7.0}, {"a", 0.1}})
        net.add_node(name, guess);
    net.add_node("slope", law.slope, true);

    // the tuning: a tuner's aim, which real fields reach within MODE_RATIO_SPREAD
    const double tuning_sigma = MODE_RATIO_SPREAD / grm::LN10;
    net.add_log("octave tuning", {{"f2", 1}, {"f1", -1}}, std::log10(OCTAVE_RATIO), tuning_sigma);
    net.add_log("compound fifth tuning", {{"f3", 1}, {"f1", -1}}, std::log10(COMPOUND_FIFTH_RATIO), tuning_sigma);

    // the plate: f1 a^2 sqrt(12 rho (1 - nu^2)) / (lambda2 h sqrt(E)) = 1 / (2 pi)
    net.add_sum("Poisson factor", {{"nu_term", 1}, {"nu", 1}}, 1.0, 1e-9, {{"nu", 2.0}});
    net.add_log("plate fundamental",
                {{"f1", 1}, {"a", 2}, {"rho", 0.5}, {"nu_term", 0.5}, {"E", -0.5}, {"h", -1}, {"lambda2", -1}},
                -std::log10(2.0 * PI * std::sqrt(12.0)), 1e-9);

    // the losses
    std::map<std::string, grm::Measurement> measured{
        {"f1", {fundamental, 1e-9}},
        {"E", {STEEL_YOUNG_MODULUS.value, STEEL_YOUNG_MODULUS.sigma_dex()}},
        {"rho", {STEEL_DENSITY.value, STEEL_DENSITY.sigma_dex()}},
        {"nu", {STEEL_POISSON_RATIO.value, STEEL_POISSON_RATIO.sigma_dex()}},
        {"h", {SHELL_THICKNESS.value, SHELL_THICKNESS.sigma_dex()}},
        {"lambda2", {std::sqrt(PLATE_LAMBDA2_CLAMPED * PLATE_LAMBDA2_SIMPLY_SUPPORTED),
                     0.5 * std::log10(PLATE_LAMBDA2_CLAMPED / PLATE_LAMBDA2_SIMPLY_SUPPORTED)}},
        {"slope", {law.slope, law.slope_sigma}},
    };
    for (int mode = 1; mode <= TUNED_MODES; ++mode)
    {
        const auto& at_reference = law.t60_at_reference[(size_t)mode - 1];
        const std::string t60 = detail::mode_name("t60_", mode), t60_ref = detail::mode_name("t60_ref_", mode);
        net.add_node(t60, at_reference.value);
        net.add_node(t60_ref, at_reference.value);
        detail::add_decay_relation(net, "decay of mode " + std::to_string(mode), t60, t60_ref, "slope", "f1");
        measured[t60_ref] = {at_reference.value, at_reference.sigma_dex()};
    }
    const auto r = net.solve(measured);
    if (!r.converged)
        throw std::runtime_error("the tone field network did not converge at " + std::to_string(fundamental) + " Hz");

    ToneField field{fundamental, detail::measured(r, net, "a"), {}};
    for (int m = 0; m < TUNED_MODES; ++m)
        field.modes.push_back({detail::measured(r, net, frequency_of[m]),
                               detail::measured(r, net, detail::mode_name("t60_", m + 1))});
    return field;
}
} // namespace grmh
