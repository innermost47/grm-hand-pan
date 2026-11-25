// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The energy a D4 strike gains from its neighbours, for a range of coupling rates, and the rate the sympathy network
// solves: how the network sees its one unknown. Usage: probe_sympathy
#include <cstdio>

#include "../core/sympathy_network.hpp"

using namespace grmh;

int main()
{
    const Measured target = measured_sympathy_gain_db();
    std::printf("measured: %.1f dB (+/- %.1f dB) [A15 Table 4.7]\n", target.value, target.spread);
    const SympathyProbe probe;
    for (const double g : {1.0, 2.0, 4.0, 8.0, 16.0, 32.0})
        std::printf("g %5.1f 1/s: gain %5.2f dB, full handover in %.0f ms\n", g, probe.energy_gain(g),
                    1e3 * PI / (2.0 * g));
    const Measured g = coupling_rate();
    std::printf("solved: g %.2f 1/s (+/- %.0f %%), gain %.2f dB\n", g.value, 100 * g.spread,
                probe.energy_gain(g.value));
    return 0;
}
