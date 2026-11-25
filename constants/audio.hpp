// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The sound as it leaves the instrument: the air it travels in, where it is heard, how loud full scale is, and how a
// MIDI velocity becomes a finger. The listening choices (distance, full scale, velocity curve) are choices, not
// measurements: they are Anthony's to tune by ear.
#pragma once

#include "core.hpp"
#include "player.hpp"

namespace grmh
{
// ---- the air: rho = p M / (R T)
constexpr double ATMOSPHERIC_PRESSURE = 101325.0; // Pa, standard atmosphere

// ---- the listener
constexpr double LISTENING_DISTANCE = 1.0;   // m
constexpr double FULL_SCALE_PRESSURE = 1.0;  // Pa at full scale: 94 dB SPL

// ---- a MIDI velocity is a fingertip velocity: the reference tap [K06] at velocity 64, an octave of speed every
// 32 steps (0.11 m/s at 1, 1.8 m/s at 127)
constexpr int MIDI_VELOCITY_REFERENCE = 64;
constexpr double MIDI_VELOCITY_STEPS_PER_DOUBLING = 32.0;
constexpr int MIDI_VELOCITIES = 128;

// ---- the instrument's size in the audio thread: fixed, so that nothing is allocated there
constexpr int MAX_FIELDS = 16;
} // namespace grmh
