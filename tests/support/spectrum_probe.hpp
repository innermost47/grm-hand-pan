// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Measures a rendered sound: the level of one frequency over a window (Goertzel, Hann window), the peak.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace grmh::test
{
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
} // namespace grmh::test
