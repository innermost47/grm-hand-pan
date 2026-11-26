// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The attack of each recorded note [MUMT22] against the model's: what the modes put above themselves as they start, the
// finger's click alone (plate theory, weight 1), and the weight the click would need to sound as the recording does.
// Usage: probe_click
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "../core/participation_network.hpp"

using namespace grmh;

int main()
{
    const auto power = [](double db) { return std::pow(10.0, db / 10.0); };
    for (const RecordedNote& r : RECORDED_NOTES)
    {
        const UnitLevels u = unit_participation_levels(r);
        const double base = power(u.attack_without_click), click = power(u.levels.attack) - base;
        const double needed = (power(r.attack) - base) / std::max(click, 1e-30);
        std::printf("%s: model attack %6.1f dB (modes alone %6.1f, click alone %6.1f) | recording %6.1f dB | "
                    "click weight needed %+5.1f dB\n",
                    r.name, u.levels.attack, u.attack_without_click, 10 * std::log10(std::max(click, 1e-30)), r.attack,
                    10 * std::log10(std::max(needed, 1e-30)));
    }
    return 0;
}
