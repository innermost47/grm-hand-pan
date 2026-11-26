// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Measures the levels of a struck note's modes against its fundamental: the same measure for a recording and for the
// model, so that the two compare. The window starts 20 ms after the onset (past the contact) and lasts 0.5 s. The
// attack is what sounds above every mode (ten times the fundamental and up) around the onset, while the finger is on
// the steel: its rms over 2 ms before to 12 ms after the onset, against the fundamental's amplitude.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "../models/core.hpp"
#include "biquad.hpp"
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
        // a Hann-windowed Goertzel sums a tone of amplitude A to A N / 4
        const double fundamental_amplitude = 4.0 * std::pow(10.0, f1.level / 20.0) / (WINDOW * sample_rate);
        levels.attack = 20.0 * std::log10(attack_rms(samples, sample_rate, t - SKIP, 10.0 * f1.frequency) /
                                          fundamental_amplitude + 1e-30);
        return levels;
    }

    static constexpr double ATTACK_BEFORE = 0.002, ATTACK_AFTER = 0.012; // s around the onset

private:
    // the rms above `cutoff` (two Butterworth high-passes: 24 dB per octave) around the onset
    static double attack_rms(const std::vector<float>& samples, double sample_rate, double onset, double cutoff)
    {
        const BiquadCoefficients k = high_pass(cutoff, sample_rate);
        Biquad first, second;
        const auto begin = static_cast<size_t>(std::max(0.0, onset - ATTACK_BEFORE) * sample_rate);
        const auto end = std::min(samples.size(), static_cast<size_t>((onset + ATTACK_AFTER) * sample_rate));
        double sum = 0.0;
        for (size_t n = 0; n < end; ++n)
        {
            const double y = second.process(k, first.process(k, samples[n]));
            if (n >= begin)
                sum += y * y;
        }
        return end > begin ? std::sqrt(sum / double(end - begin)) : 0.0;
    }

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
