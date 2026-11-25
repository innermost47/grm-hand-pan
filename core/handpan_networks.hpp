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
#include "network_support.hpp"

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
        law.t60_at_reference[(size_t)mode - 1] = network::measured(r, net, detail::mode_name("t60_ref_", mode));
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

// one tone field tuned to `fundamental` (Hz): its modes (the tuned ones 1:2:3 within the measured spread, the untuned
// ones at their measured ratios), how long each rings (the loss law), and the radius of the steel plate that rings at
// that fundamental. The loss law is measured on the tuned modes only; it does not depend on a mode's frequency
// (its slope is on the field's fundamental), so the untuned modes take the mean of the tuned modes' T60, with the
// spread between them as its uncertainty.
inline ToneField solve_tone_field(double fundamental, const LossLaw& law = loss_law())
{
    grm::Network net;
    for (int m = 0; m < FIELD_MODES; ++m)
        net.add_node(detail::mode_name("f", m + 1), fundamental * FIELD_MODE_SHAPES[(size_t)m].ratio);
    for (auto [name, guess] : {std::pair{"E", STEEL_YOUNG_MODULUS.value}, {"rho", STEEL_DENSITY.value},
                               {"nu", STEEL_POISSON_RATIO.value}, {"nu_term", 0.9}, {"h", SHELL_THICKNESS.value},
                               {"lambda2", 7.0}, {"a", 0.1}})
        net.add_node(name, guess);
    net.add_node("slope", law.slope, true);

    // the tuning: a tuner's aim, which real fields reach within the measured spread; the untuned modes sit where the
    // dome puts them [MR07]
    for (int m = 1; m < FIELD_MODES; ++m)
    {
        const FieldModeShape& shape = FIELD_MODE_SHAPES[(size_t)m];
        net.add_log(std::string("ratio of ") + shape.name, {{detail::mode_name("f", m + 1), 1}, {"f1", -1}},
                    std::log10(shape.ratio), shape.spread / grm::LN10);
    }

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
        {"lambda2", network::between(PLATE_LAMBDA2_SIMPLY_SUPPORTED, PLATE_LAMBDA2_CLAMPED)},
        {"slope", {law.slope, law.slope_sigma}},
    };
    double mean_log = 0.0;
    for (int mode = 1; mode <= TUNED_MODES; ++mode)
    {
        const auto& at_reference = law.t60_at_reference[(size_t)mode - 1];
        const std::string t60_ref = detail::mode_name("t60_ref_", mode);
        net.add_node(t60_ref, at_reference.value);
        measured[t60_ref] = {at_reference.value, at_reference.sigma_dex()};
        mean_log += std::log10(at_reference.value) / TUNED_MODES;
    }
    // the untuned modes: the mean of the tuned modes' T60 at the reference, uncertain by the spread between them
    double spread = 0.0;
    for (const auto& at_reference : law.t60_at_reference)
        spread += std::pow(std::log10(at_reference.value) - mean_log, 2) / (TUNED_MODES - 1);
    net.add_node("t60_ref_untuned", std::pow(10.0, mean_log));
    std::vector<std::pair<std::string, double>> mean_terms{{"t60_ref_untuned", 1.0}};
    for (int mode = 1; mode <= TUNED_MODES; ++mode)
        mean_terms.push_back({detail::mode_name("t60_ref_", mode), -1.0 / TUNED_MODES});
    net.add_log("untuned modes ring as the mean of the tuned", mean_terms, 0.0, std::sqrt(spread));

    for (int m = 0; m < FIELD_MODES; ++m)
    {
        const std::string t60 = detail::mode_name("t60_", m + 1);
        const std::string t60_ref = m < TUNED_MODES ? detail::mode_name("t60_ref_", m + 1) : "t60_ref_untuned";
        net.add_node(t60, law.t60_at_reference[0].value);
        detail::add_decay_relation(net, "decay of mode " + std::to_string(m + 1), t60, t60_ref, "slope", "f1");
    }
    const auto r = net.solve(measured);
    if (!r.converged)
        throw std::runtime_error("the tone field network did not converge at " + std::to_string(fundamental) + " Hz");

    ToneField field{fundamental, network::measured(r, net, "a"), {}};
    for (int m = 0; m < FIELD_MODES; ++m)
        field.modes.push_back({network::measured(r, net, detail::mode_name("f", m + 1)),
                               network::measured(r, net, detail::mode_name("t60_", m + 1)),
                               FIELD_MODE_SHAPES[(size_t)m].multipole_order});
    return field;
}
} // namespace grmh
