// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The fingers on the steel: the force pulses under way, each a half-sine as long as its contact. Below a field's
// highest mode, the sheet's answer to that force is in the modes the voices play; above it, the sheet answers as an
// infinite plate does on average (the mean-value theorem of Skudrzyk; Cremer & Heckl): each pulse is heard through a
// high-pass at its field's highest mode, the residual of the modes. A fixed number of pulses at once (a roll of the
// fastest hands overlaps a few), nothing allocated: when every slot is busy, the pulse nearest its end gives its place.
#pragma once

#include <array>
#include <cmath>

#include "../constants/core.hpp"
#include "biquad.hpp"

namespace grmh
{
class FingerContacts
{
public:
    static constexpr int MAX_CONTACTS = 32;

    // a finger lands: its pulp pushes for `length` samples, peaking at `peak_force` newtons, heard through `residual`
    // (none: the whole force) for `tail` samples more once the finger has left
    void press(int length, double peak_force, const BiquadCoefficients* residual = nullptr, int tail = 0)
    {
        if (length <= 0 || peak_force <= 0.0)
            return;
        Pulse* slot = &pulses_[0];
        for (Pulse& p : pulses_)
        {
            if (p.remaining() <= 0)
            {
                slot = &p;
                break;
            }
            if (p.remaining() < slot->remaining())
                slot = &p;
        }
        *slot = {};
        slot->length = length;
        slot->end = length + (residual != nullptr ? tail : 0);
        slot->peak = peak_force;
        slot->step = PI / length;
        slot->residual = residual;
    }

    // the force on the steel the listener hears during this sample (N), then one sample on
    double step()
    {
        double force = 0.0;
        for (Pulse& p : pulses_)
            if (p.remaining() > 0)
            {
                const double f = p.position < p.length ? p.peak * std::sin(p.step * (p.position + 0.5)) : 0.0;
                force += p.residual != nullptr ? p.filter.process(*p.residual, f) : f;
                ++p.position;
            }
        return force;
    }

    bool idle() const
    {
        for (const Pulse& p : pulses_)
            if (p.remaining() > 0)
                return false;
        return true;
    }

    void release() { pulses_ = {}; }

private:
    struct Pulse
    {
        int position = 0, length = 0, end = 0;
        double peak = 0.0, step = 0.0; // radians per sample
        const BiquadCoefficients* residual = nullptr;
        Biquad filter;
        int remaining() const { return end - position; }
    };
    std::array<Pulse, MAX_CONTACTS> pulses_{};
};
} // namespace grmh
