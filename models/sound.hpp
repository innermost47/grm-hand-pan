// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The settings of the instrument, and what the audio thread reads: plain data, sized at compile time, ready to play.
#pragma once

#include <array>
#include <vector>

#include "../constants/audio.hpp"
#include "../constants/sympathy.hpp"
#include "core.hpp"

namespace grmh
{
// what the player chooses
struct Settings
{
    std::vector<int> notes;              // MIDI notes of the tone fields, the ding first
    bool on_lap = false;                 // the knees lengthen the Gu's neck (air mode 74 Hz instead of 85 Hz)
    double temperature = ROOM_TEMPERATURE; // K
    unsigned instrument_seed = INSTRUMENT_SEED_DEFAULT; // which instrument: the small mistuning of its modes
    double stereo_width = STEREO_WIDTH_DEFAULT;          // 0 mono, 1 the side fields hard in one channel
};

// what the networks solve from measurements, for the model to play: how strongly a strike excites each kind of mode,
// how fast two modes of different fields at the same frequency exchange their energy, and how far an instrument's
// overtones stray from their ratios
struct Calibration
{
    Participation participation;
    double coupling_rate = 0.0;    // g, 1/s
    double mistuning_spread = 0.0; // relative, rms
};

// two modes of different fields that sing together: their exact exchange over one sample (a rotation of their
// complex amplitudes by g / sample rate, which keeps their total energy)
struct Coupling
{
    int field_a = 0, mode_a = 0, field_b = 0, mode_b = 0;
    double cos_angle = 1.0, sin_angle = 0.0;
};

constexpr int MAX_COUPLINGS = 192;

// one mode, ready for a damped complex oscillator: its rotation per sample, its angular frequency and decay rate
// (velocity z, acceleration (i w - a) z), the air it moves per unit of velocity, and the velocity a strike gives it
struct ModeSound
{
    double rotation_re = 0.0, rotation_im = 0.0;      // exp((i w - a) / sample rate)
    double angular_frequency = 0.0;                    // w, rad/s
    double decay_rate = 0.0;                           // a, 1/s
    double radiating_area = 0.0;                       // m2: the field's area times the mode's radiation factor
    std::array<double, MIDI_VELOCITIES> strike_velocity{}; // m/s per MIDI velocity
};

// one tone field, ready to play
struct FieldSound
{
    int note = 0;
    double area = 0.0; // m2, the field's effective plate: the air its fundamental pushes into the cavity
    std::array<ModeSound, FIELD_MODES> modes{};
    BiquadCoefficients click_residual; // a high-pass at the field's highest mode: the finger's click above the modes
    int click_tail = 0;                // samples the residual rings once the finger has left
    double left_gain = 1.0, right_gain = 1.0; // where the field sounds between the channels (core/layout.hpp)
};

// the air of the cavity, ready to play: the Gu's plug as a resonator driven by the fields' volume velocity
struct CavitySound
{
    double frequency = 0.0; // Hz
    double quality_factor = 0.0;
};

// the finger on the steel, ready to play: for each MIDI velocity, how long the pulp pushes and how hard (a half-sine
// force pulse), and the pressure that force radiates at the listener per newton (full scale): the sheet's volume
// accelerates by F / rho_s, a baffled source, p = rho0 F / (2 pi r rho_s)
struct ContactSound
{
    std::array<int, MIDI_VELOCITIES> length{};        // samples
    std::array<double, MIDI_VELOCITIES> peak_force{}; // N
    double click_gain = 0.0;                          // full scale per newton
};

// everything the audio thread needs, solved off it
struct SoundModel
{
    double sample_rate = 0.0;
    int field_count = 0;
    std::array<FieldSound, MAX_FIELDS> fields{};
    CavitySound cavity;
    ContactSound contact;
    int coupling_count = 0;
    std::array<Coupling, MAX_COUPLINGS> couplings{};
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
