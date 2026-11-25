// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// How the fields of one instrument sing together, as [A15] measured it: each field struck free, then with its
// neighbours damped by magnetic pads (section 4.4).
#pragma once

#include <array>

namespace grmh
{
// the total energy of a D4 strike with its neighbours free, against the same strike with them damped (dB), on the
// four handpans of [A15] Table 4.7 ("-10 dB" damped for instrument 1, undamped "-1.1 dB" below damped for
// instrument 2, then -5.7 and -4.3 dB): what the neighbours add
constexpr std::array<double, 4> SYMPATHY_ENERGY_GAINS_DB{10.0, -1.1, 5.7, 4.3};

// the time a coupled mode of an F#3 strike takes to reach its maximum, [A15] Table 4.4: the 748 Hz peak (four times
// the fundamental, a neighbour's) at 209 ms with the neighbours free, 92 ms damped; the onsets are "accurate within
// -23 ms" (the analysis' time resolution). The 556 Hz peak is the F#3's own fifth, late even damped (372 ms): a
// transfer within the field, not between fields.
constexpr double SYMPATHY_COUPLED_ONSET = 0.209;    // s
constexpr double SYMPATHY_ONSET_RESOLUTION = 0.023; // s

// beating of coupled modes [A15] Table 4.3 (Hz at Hz): the mistuning between two modes that sing together
struct Beating
{
    double frequency;
    double rate;
};
constexpr std::array<Beating, 5> MEASURED_BEATINGS{{{662.0, 3.3}, {438.0, 8.6}, {248.0, 3.9}, {373.0, 3.1}, {374.0, 3.3}}};

// the modes the coupling joins: every pair that exchanges at least this share of its amplitude, 2g / sqrt(4g^2 + dw^2)
// (-30 dB). [A15] Table 4.5 hears neighbours that are not in tune with the struck field (an E4, 39 Hz from a D4, at
// +16 dB): the coupling does not stop at coincident modes.
constexpr double SYMPATHY_MIN_EXCHANGE = 0.03;

// the instrument whose mistuning the sound plays when none is chosen
constexpr unsigned INSTRUMENT_SEED_DEFAULT = 1;
} // namespace grmh
