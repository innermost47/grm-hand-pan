// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Measures the levels of a struck note's modes against its fundamental: the same measure for a recording and for the
// model, so that the two compare. The window starts 20 ms after the onset (past the contact) and lasts 0.5 s.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "../models/core.hpp"
#include "spectrum_probe.hpp"

namespace grmh
{
class ModeLevelMeter
{
public:
    static constexpr double SKIP = 0.02, WINDOW = 0.5; // s

    static ModeLevels measure(const std::vector<float>& samples, double sample_rate, double nominal_frequency)
    {
        const SpectrumProbe probe(samples, sample_rate);
        const double t = onset(samples, sample_rate) + SKIP;
        const SpectralPeak f1 = probe.strongest(nominal_frequency * 0.96, nominal_frequency * 1.04, t, WINDOW);
        const auto relative = [&](double ratio, double spread)
        {
            const double f = f1.frequency * ratio;
            return probe.strongest(f * (1 - spread), f * (1 + spread), t, WINDOW).level - f1.level;
        };
        ModeLevels levels;
        levels.fundamental_frequency = f1.frequency;
        levels.octave = relative(2.0, 0.03);
        levels.fifth = relative(3.0, 0.03);
        levels.untuned = relative(8.0, 0.1875); // 6.5 to 9.5
        levels.untuned_band_floor = probe.floor(6.5 * f1.frequency, 9.5 * f1.frequency, t, WINDOW) - f1.level;
        return levels;
    }

private:
    // the first sample above a tenth of the peak
    static double onset(const std::vector<float>& samples, double sample_rate)
    {
        float peak = 0.0f;
        for (const float s : samples)
            peak = std::max(peak, std::fabs(s));
        for (size_t n = 0; n < samples.size(); ++n)
            if (std::fabs(samples[n]) > 0.1f * peak)
                return double(n) / sample_rate;
        return 0.0;
    }
};
} // namespace grmh
