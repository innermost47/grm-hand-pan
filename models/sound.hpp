// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The settings of the instrument, and what the audio thread reads: plain data, sized at compile time, ready to play.
#pragma once

#include <array>
#include <vector>

#include "../constants/audio.hpp"
#include "core.hpp"

namespace grmh
{
// what the player chooses
struct Settings
{
    std::vector<int> notes;              // MIDI notes of the tone fields, the ding first
    bool on_lap = false;                 // the knees lengthen the Gu's neck (air mode 74 Hz instead of 85 Hz)
    double temperature = ROOM_TEMPERATURE; // K
};

// one mode, ready for a damped complex oscillator: its rotation per sample, its angular frequency and decay rate
// (velocity z, acceleration (i w - a) z), and the velocity a strike gives it
struct ModeSound
{
    double rotation_re = 0.0, rotation_im = 0.0;      // exp((i w - a) / sample rate)
    double angular_frequency = 0.0;                    // w, rad/s
    double decay_rate = 0.0;                           // a, 1/s
    std::array<double, MIDI_VELOCITIES> strike_velocity{}; // m/s per MIDI velocity
};

// one tone field, ready to play
struct FieldSound
{
    int note = 0;
    double area = 0.0; // m2, the field's effective plate: the air it pushes per unit of velocity
    std::array<ModeSound, TUNED_MODES> modes{};
};

// the air of the cavity, ready to play: the Gu's plug as a resonator driven by the fields' volume velocity
struct CavitySound
{
    double frequency = 0.0; // Hz
    double quality_factor = 0.0;
};

// everything the audio thread needs, solved off it
struct SoundModel
{
    double sample_rate = 0.0;
    int field_count = 0;
    std::array<FieldSound, MAX_FIELDS> fields{};
    CavitySound cavity;
    double pressure_gain = 0.0; // full scale per volume acceleration (m3/s2): rho / (4 pi r) / FULL_SCALE_PRESSURE

    // the field tuned to a MIDI note, or -1
    int field_of(int note) const
    {
        for (int i = 0; i < field_count; ++i)
            if (fields[(size_t)i].note == note)
                return i;
        return -1;
    }
};
} // namespace grmh
