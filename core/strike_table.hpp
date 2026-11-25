// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The strike of every MIDI velocity, solved once: a strike depends on the finger only, never on the settings.
#pragma once

#include <array>
#include <cmath>

#include "../constants/audio.hpp"
#include "../models/core.hpp"
#include "strike_network.hpp"

namespace grmh
{
// the fingertip velocity (m/s) a MIDI velocity (1-127) stands for
inline double fingertip_velocity(int midi_velocity)
{
    return TAP_VELOCITY_REFERENCE *
           std::pow(2.0, (midi_velocity - MIDI_VELOCITY_REFERENCE) / MIDI_VELOCITY_STEPS_PER_DOUBLING);
}

// the strikes of MIDI velocities 1-127 (index 0 is a silent strike), solved on first use
inline const std::array<Strike, MIDI_VELOCITIES>& strike_table()
{
    static const std::array<Strike, MIDI_VELOCITIES> table = []
    {
        std::array<Strike, MIDI_VELOCITIES> strikes{};
        for (int velocity = 1; velocity < MIDI_VELOCITIES; ++velocity)
            strikes[(size_t)velocity] = solve_strike(fingertip_velocity(velocity));
        return strikes;
    }();
    return table;
}
} // namespace grmh
