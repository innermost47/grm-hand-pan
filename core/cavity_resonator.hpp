// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plug of air in the Gu, driven by the volume the fields push into the cavity: a Helmholtz resonator, whose
// volume velocity follows the fields' through H(s) = w^2 / (s^2 + s w / Q + w^2) (a trapezoidal state variable
// filter, exact in frequency, stable at any setting). Its low-pass output is the plug's volume velocity; the
// filter's own equation gives its derivative, w times the band-pass output: the plug's volume acceleration.
#pragma once

#include <cmath>

#include "../constants/core.hpp"

namespace grmh
{
class CavityResonator
{
public:
    void set(double frequency, double quality_factor, double sample_rate)
    {
        const double g = std::tan(PI * frequency / sample_rate), k = 1.0 / quality_factor;
        angular_frequency_ = 2.0 * PI * frequency;
        a1_ = 1.0 / (1.0 + g * (g + k));
        a2_ = g * a1_;
        a3_ = g * a2_;
    }

    // the plug's volume acceleration (m3/s2, out of the Gu) for the fields' outward volume velocity: when the walls
    // move out, the cavity grows and the plug is drawn in; at low frequencies it gives back what the walls push
    double step(double walls_volume_velocity)
    {
        const double v3 = walls_volume_velocity - state2_;
        const double v1 = a1_ * state1_ + a2_ * v3; // band-pass
        const double v2 = state2_ + a2_ * state1_ + a3_ * v3; // low-pass
        state1_ = 2.0 * v1 - state1_;
        state2_ = 2.0 * v2 - state2_;
        return -angular_frequency_ * v1;
    }

    void silence() { state1_ = state2_ = 0.0; }

private:
    double angular_frequency_ = 0.0;
    double a1_ = 0.0, a2_ = 0.0, a3_ = 0.0;
    double state1_ = 0.0, state2_ = 0.0;
};
} // namespace grmh
