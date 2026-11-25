// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Measures a sound, rendered or recorded: the level of one frequency over a window (Goertzel, Hann window), the
// strongest frequency in a band, the peak.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace grmh
{
// a frequency found in a band, and its level (dB, relative)
struct SpectralPeak
{
    double frequency;
    double level;
};

class SpectrumProbe
{
public:
    SpectrumProbe(const std::vector<float>& samples, double sample_rate) : samples_(samples), rate_(sample_rate) {}

    // the level (dB, relative) of `frequency` in the window [start, start + length) seconds
    double level(double frequency, double start, double length) const
    {
        const auto first = static_cast<size_t>(start * rate_);
        const auto count = static_cast<size_t>(length * rate_);
        const double pi = 3.14159265358979323846, w = 2.0 * pi * frequency / rate_;
        double re = 0.0, im = 0.0;
        for (size_t n = 0; n < count && first + n < samples_.size(); ++n)
        {
            const double window = 0.5 - 0.5 * std::cos(2.0 * pi * n / (count - 1));
            re += window * samples_[first + n] * std::cos(w * n);
            im -= window * samples_[first + n] * std::sin(w * n);
        }
        return 10.0 * std::log10(re * re + im * im + 1e-30);
    }

    // the strongest frequency in [low, high] Hz over the window, searched in steps finer than its resolution
    SpectralPeak strongest(double low, double high, double start, double length) const
    {
        const double step = 0.25 / length; // a quarter of the window's resolution
        SpectralPeak best{low, level(low, start, length)};
        for (double f = low + step; f <= high; f += step)
        {
            const double l = level(f, start, length);
            if (l > best.level)
                best = {f, l};
        }
        return best;
    }

    // the median level over [low, high] Hz: the floor a peak stands above
    double floor(double low, double high, double start, double length) const
    {
        std::vector<double> levels;
        for (double f = low; f <= high; f += 1.0 / length)
            levels.push_back(level(f, start, length));
        std::nth_element(levels.begin(), levels.begin() + levels.size() / 2, levels.end());
        return levels[levels.size() / 2];
    }

    float peak() const
    {
        float p = 0.0f;
        for (const float s : samples_)
            p = std::max(p, std::fabs(s));
        return p;
    }

    bool finite() const
    {
        return std::all_of(samples_.begin(), samples_.end(), [](float s) { return std::isfinite(s); });
    }

private:
    const std::vector<float>& samples_;
    double rate_;
};
} // namespace grmh
