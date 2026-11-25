// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The structs shared by the core (no JUCE).
#pragma once

#include <array>
#include <cmath>
#include <vector>

namespace grmh
{
// a published value and its spread: the spread is relative (one standard deviation, or half the published range),
// and becomes the sigma of the measurement in a GRM network
struct Measured
{
    double value;
    double spread;

    // the sigma a network takes, in dex (log10 units), for a positive quantity
    double sigma_dex() const { return spread / std::log(10.0); }
    double low() const { return value * (1.0 - spread); }
    double high() const { return value * (1.0 + spread); }
};

// the three lowest modes of one tone field, as measured: (0,1), (1,1)a, (1,1)b in Hz
struct MeasuredToneField
{
    const char* note;
    double fundamental;
    double octave;
    double compound_fifth;
};

// T60 statistics of the three strongest modes of a group of tone fields (seconds)
struct DecayStatistics
{
    const char* group;
    double mean;
    double standard_deviation;
    double min;
    double max;
};

// the T60 of one tuned mode of one tone field: the mode's frequency (Hz), its rank (1 fundamental, 2 octave,
// 3 compound fifth), its T60 (s)
struct DecayMeasurement
{
    double frequency;
    int mode;
    double t60;
};

// the number of tuned modes of a tone field
constexpr int TUNED_MODES = 3;

// how long the tuned modes ring: T60 = t60_at_reference[mode] * (f1 / reference)^slope, f1 the field's fundamental
struct LossLaw
{
    std::array<Measured, TUNED_MODES> t60_at_reference; // s, for a field whose fundamental is the reference
    double slope;                                       // dimensionless, signed
    double slope_sigma;                                 // absolute
    double reference_frequency;                         // Hz
};

// one mode of a tone field: its frequency (Hz) and T60 (s), each with its relative uncertainty
struct Mode
{
    Measured frequency;
    Measured t60;
};

// what a tone field gives the sound
struct ToneField
{
    double fundamental;        // Hz, the note it is tuned to
    Measured effective_radius; // m, the radius of the plate that rings at the fundamental
    std::vector<Mode> modes;   // the tuned modes, fundamental first
};

// what the cavity gives the sound: the air mode (Helmholtz), driven by the volume the ding pushes into the cavity
struct Cavity
{
    Measured frequency;         // Hz
    Measured t60;               // s, radiation losses only: an upper bound
    Measured quality_factor;    // radiation losses only
    Measured volume;            // m3
    Measured neck_conductance;  // m, S / L' of the Gu: its opening over its effective length
    double speed_of_sound;      // m/s
};
} // namespace grmh
