// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A second-order filter: its state (transposed direct form II), and the design of a Butterworth high-pass (R.
// Bristow-Johnson's audio EQ cookbook).
#pragma once

#include <cmath>

#include "../constants/core.hpp"
#include "../models/core.hpp"

namespace grmh
{
// a Butterworth high-pass (Q = 1 / sqrt 2) at `cutoff` Hz
inline BiquadCoefficients high_pass(double cutoff, double sample_rate)
{
    const double w = 2.0 * PI * std::fmin(cutoff, 0.45 * sample_rate) / sample_rate;
    const double alpha = std::sin(w) / std::sqrt(2.0), c = std::cos(w), a0 = 1.0 + alpha;
    return {(1.0 + c) / (2.0 * a0), -(1.0 + c) / a0, (1.0 + c) / (2.0 * a0), -2.0 * c / a0, (1.0 - alpha) / a0};
}

class Biquad
{
public:
    double process(const BiquadCoefficients& k, double x)
    {
        const double y = k.b0 * x + s1_;
        s1_ = k.b1 * x - k.a1 * y + s2_;
        s2_ = k.b2 * x - k.a2 * y;
        return y;
    }

    void reset() { s1_ = s2_ = 0.0; }

private:
    double s1_ = 0.0, s2_ = 0.0;
};
} // namespace grmh
