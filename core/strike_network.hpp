// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The network of a finger strike: the finger's mass lands on its pulp, which pushes the tone field for the pulp's
// force transient and bounces back with what it did not dissipate. The pulp hardens as it is pressed (its stiffness
// grows with the force): a faster finger presses harder, for a shorter time. It gives the force pulse; the pulse's
// spectrum says how much of each mode the strike excites: a long contact leaves the high modes silent.
#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

#include "../constants/core.hpp"
#include "../constants/player.hpp"
#include "../models/core.hpp"
#include "grm.hpp"
#include "network_support.hpp"

namespace grmh
{
namespace detail
{
// one impact of the finger (mass m, restitution "bounce" = 1 + e, pulp hardening b, all shared) landing at node
// v<suffix>: its impulse J, peak force F, pulp stiffness k and contact time tc, suffixed
inline void add_impact(grm::Network& net, const std::string& suffix, double velocity_guess)
{
    const auto node = [&](const char* name) { return name + suffix; };
    for (auto [name, guess] : {std::pair{"v", velocity_guess}, {"J", 4e-3}, {"F", 3.0}, {"k", 1e4}, {"tc", 3e-3}})
        net.add_node(node(name), guess);
    // the field takes the finger's momentum and what bounces back: J = m v (1 + e)
    net.add_log("impulse" + suffix, {{node("J"), 1}, {"m", -1}, {node("v"), -1}, {"bounce", -1}}, 0.0, 1e-9);
    // a half-sine pulse of area J and length tc peaks at pi J / (2 tc)
    net.add_log("peak force" + suffix, {{node("F"), 1}, {node("J"), -1}, {node("tc"), 1}}, std::log10(PI / 2.0), 1e-9);
    // an exponential pulp, F = a exp(b d), is as stiff as it is loaded: k = b F
    net.add_log("pulp hardening" + suffix, {{node("k"), 1}, {"b", -1}, {node("F"), -1}}, 0.0, 1e-9);
    // a mass on a spring stays in contact for half a period: tc = pi sqrt(m / k)
    net.add_log("pulp contact" + suffix, {{node("tc"), 1}, {"m", -0.5}, {node("k"), 0.5}}, std::log10(PI), 1e-9);
}
} // namespace detail

// the strike of a fingertip landing at `velocity` (m/s). The pulp's measurements were made on taps at the reference
// velocity [K06, J03]: the network holds that tap too, and both share the finger and its pulp
inline Strike solve_strike(double velocity)
{
    if (!(velocity > 0.0))
        throw std::invalid_argument("a strike needs a positive fingertip velocity");

    grm::Network net;
    for (auto [name, guess] : {std::pair{"m", 6e-3}, {"e", 0.4}, {"bounce", 1.4},
                               {"dissipated", PULP_ENERGY_DISSIPATED}, {"b", 3000.0}, {"deflection_step", 2e-4}})
        net.add_node(name, guess);
    // the energy the pulp keeps comes back: e^2 + dissipated = 1
    net.add_sum("restitution", {{"e", 1}, {"dissipated", 1}}, 1.0, 1e-9, {{"e", 2.0}});
    net.add_sum("bounce", {{"bounce", 1}, {"e", -1}}, 1.0, 1e-9);
    // doubling the force presses the pulp one step deeper: b = ln(2) / step
    net.add_log("pulp hardening step", {{"b", 1}, {"deflection_step", 1}}, std::log10(std::log(PULP_FORCE_RATIO)),
                1e-9);
    detail::add_impact(net, "_ref", TAP_VELOCITY_REFERENCE);
    detail::add_impact(net, "", velocity);

    const auto r = net.solve(
        {{"m", network::between(FINGER_MASS_MIN, FINGER_MASS_MAX)},
         {"dissipated", {PULP_ENERGY_DISSIPATED, PULP_ENERGY_DISSIPATED_SPREAD / grm::LN10}},
         {"deflection_step", network::between(PULP_DEFLECTION_STEP_MIN, PULP_DEFLECTION_STEP_MAX)},
         {"v_ref", {TAP_VELOCITY_REFERENCE, TAP_VELOCITY_REFERENCE_SPREAD / grm::LN10}},
         {"tc_ref", network::between(PULP_CONTACT_MIN, PULP_CONTACT_MAX)},
         {"v", {velocity, 1e-9}}});
    if (!r.converged)
        throw std::runtime_error("the strike network did not converge");

    const auto measured = [&](const char* node) { return network::measured(r, net, node); };
    return {velocity, measured("tc"), measured("J"), measured("F"), measured("k"), measured("e"), measured("b")};
}

// the force a half-sine pulse of length `contact_time` puts at `frequency`, over its impulse (1 at 0 Hz):
// |cos(pi f tc) / (1 - (2 f tc)^2)|
inline double strike_spectrum(double frequency, double contact_time)
{
    const double x = 2.0 * frequency * contact_time;
    if (std::fabs(1.0 - x * x) < 1e-9)
        return PI / 4.0; // the limit at x = 1
    return std::fabs(std::cos(PI * frequency * contact_time) / (1.0 - x * x));
}

// the same, averaged in energy over the uncertainty of the contact time (log-normal, its relative uncertainty as
// sigma): the sharp zeros of one exact half-sine are not those of a strike whose length is known within a range
inline double expected_strike_spectrum(double frequency, const Measured& contact_time)
{
    constexpr int half_width = 24;  // samples of the distribution on each side
    constexpr double extent = 3.0;  // sigmas covered
    double energy = 0.0, weights = 0.0;
    for (int i = -half_width; i <= half_width; ++i)
    {
        const double z = extent * i / half_width, w = std::exp(-0.5 * z * z);
        const double s = strike_spectrum(frequency, contact_time.value * std::exp(z * contact_time.spread));
        energy += w * s * s;
        weights += w;
    }
    return std::sqrt(energy / weights);
}
} // namespace grmh
