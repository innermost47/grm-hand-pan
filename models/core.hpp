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

// the number of tuned modes of a tone field, and of all the modes it plays (the tuned ones, then the untuned)
constexpr int TUNED_MODES = 3;
constexpr int FIELD_MODES = 5;

// one mode of a field, as measured: its ratio to the fundamental and the spread of that ratio, and how it radiates
// (0 monopole, 1 dipole, 2 quadrupole)
struct FieldModeShape
{
    const char* name;
    double ratio;
    double spread;
    int multipole_order;
};

// how long the tuned modes ring: T60 = t60_at_reference[mode] * (f1 / reference)^slope, f1 the field's fundamental
struct LossLaw
{
    std::array<Measured, TUNED_MODES> t60_at_reference; // s, for a field whose fundamental is the reference
    double slope;                                       // dimensionless, signed
    double slope_sigma;                                 // absolute
    double reference_frequency;                         // Hz
};

// one mode of a tone field: its frequency (Hz) and T60 (s), each with its relative uncertainty, and how it radiates
struct Mode
{
    Measured frequency;
    Measured t60;
    int multipole_order = 0;
};

// what a tone field gives the sound
struct ToneField
{
    double fundamental;        // Hz, the note it is tuned to
    Measured effective_radius; // m, the radius of the plate that rings at the fundamental
    std::vector<Mode> modes;   // FIELD_MODES modes: the tuned ones, fundamental first, then the untuned
};

// the levels of a note's modes against its fundamental, as a microphone hears them (dB)
struct ModeLevels
{
    double fundamental_frequency = 0.0; // Hz, as found
    double octave = 0.0;
    double fifth = 0.0;
    double untuned = 0.0; // the strongest peak 6.5-9.5 times the fundamental
    double untuned_band_floor = 0.0;
};

// how strongly a strike at the sweet spot excites each kind of mode, against the fundamental (amplitude ratios)
struct Participation
{
    Measured octave{1.0, 0.0};
    Measured fifth{1.0, 0.0};
    Measured untuned{1.0, 0.0};

    // the weight of a field's mode, by its index (0 the fundamental)
    double of_mode(size_t index) const
    {
        return index == 0 ? 1.0 : index == 1 ? octave.value : index == 2 ? fifth.value : untuned.value;
    }
};

// one finger strike on a tone field: a half-sine force pulse of the pulp
struct Strike
{
    double velocity;           // m/s, the fingertip at contact
    Measured contact_time;     // s, the pulp's force transient
    Measured impulse;          // N s, the momentum the finger gives the field
    Measured peak_force;       // N
    Measured pulp_stiffness;   // N/m
    Measured restitution;      // the share of the finger's velocity that bounces back
    Measured pulp_hardening;   // 1/m, b in k = b F: how much stiffer the pulp gets per newton
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
