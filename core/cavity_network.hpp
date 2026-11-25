// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The network of the cavity: the air inside the two shells and the plug of air in the Gu, a Helmholtz resonator
// [KF00]. Its frequency is measured, its geometry is not: the network gives the effective neck that the measured
// frequency asks of the published vessel, and the losses that neck radiates.
#pragma once

#include <cmath>
#include <map>
#include <stdexcept>
#include <string>

#include "../constants/core.hpp"
#include "../models/core.hpp"
#include "grm.hpp"
#include "network_support.hpp"

namespace grmh
{
// the cavity whose air mode is measured at `resonance` (Hz): CAVITY_RESONANCE_INTEGRAL_HANG played free,
// CAVITY_RESONANCE_ON_LAP held on the lap (the knees lengthen the neck)
inline Cavity solve_cavity(const Measured& resonance, double temperature = ROOM_TEMPERATURE)
{
    grm::Network net;
    net.add_node("T", temperature, false, true);
    for (auto [name, guess] : {std::pair{"c", 343.0}, {"V", 0.02}, {"s", 0.4}, {"D", DING_SHELL_DIAMETER},
                               {"H", HANG_HEIGHT}, {"K", 0.05}, {"fH", resonance.value}, {"r", 9.0}, {"Q", 50.0},
                               {"t60", 1.0}})
        net.add_node(name, guess);

    net.add_log("speed of sound", {{"c", 2}, {"T", -1}}, std::log10(GAMMA_AIR * R_GAS / M_AIR), 1e-9);
    net.add_log("cavity volume", {{"V", 1}, {"s", -1}, {"D", -2}, {"H", -1}}, 0.0, 1e-9);
    // fH = c / (2 pi) sqrt(K / V), K = S / L'
    net.add_log("Helmholtz", {{"fH", 1}, {"c", -1}, {"K", -0.5}, {"V", 0.5}}, -std::log10(2.0 * PI), 1e-9);
    // Q = r sqrt(V / K^3)
    net.add_log("neck radiation", {{"Q", 1}, {"r", -1}, {"V", -0.5}, {"K", 1.5}}, 0.0, 1e-9);
    // energy falls 60 dB in T60 = ln(1000) Q / (pi f)
    net.add_log("decay", {{"t60", 1}, {"Q", -1}, {"fH", 1}}, std::log10(std::log(1000.0) / PI), 1e-9);

    const double dimension_dex = MEASURED_DIMENSION_SPREAD / grm::LN10;
    const auto r = net.solve({{"D", {DING_SHELL_DIAMETER, dimension_dex}},
                              {"H", {HANG_HEIGHT, dimension_dex}},
                              {"s", network::between(CAVITY_SHAPE_CONES, CAVITY_SHAPE_ELLIPSOIDS)},
                              {"r", network::between(NECK_RADIATION_FLANGED, NECK_RADIATION_UNFLANGED)},
                              {"fH", {resonance.value, resonance.sigma_dex()}}});
    if (!r.converged)
        throw std::runtime_error("the cavity network did not converge");

    const auto measured = [&](const char* node) { return network::measured(r, net, node); };
    return {measured("fH"), measured("t60"), measured("Q"), measured("V"), measured("K"), r["c"]};
}
} // namespace grmh
