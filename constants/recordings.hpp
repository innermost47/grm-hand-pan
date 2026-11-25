// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Recorded handpan notes, measured with tools/measure_recordings (the probe the tests use: Hann-windowed Goertzel,
// 0.5 s from 20 ms after the onset). Source [MUMT22]: C. Xu (carrieeex), "Modal Synthesis of the Handpan Sound",
// MUMT 307 final project, McGill University (2022), https://github.com/carrieeex/MUMT307-project, folder
// sounds-original: single taps of a Kurd D handpan, each note area tapped "slightly with one hand or a rubber
// mallet" (which one, per note, is not stated). Levels in dB against the fundamental; the untuned level is the
// strongest peak 6.5-9.5 times the fundamental.
#pragma once

#include <array>

namespace grmh
{
struct RecordedNote
{
    const char* name;
    int note;          // MIDI
    double octave;     // dB
    double fifth;      // dB
    double untuned;    // dB
};

constexpr std::array<RecordedNote, 6> RECORDED_NOTES{{
    {"D3", 50, -12.7, -7.3, -38.6},
    {"A3", 57, -7.6, -7.9, -46.6},
    {"D4", 62, -9.1, -19.0, -66.5},
    {"F4", 65, -7.0, -10.5, -62.6},
    {"A4", 69, -8.8, -27.2, -80.9},
    {"B4", 71, 0.7, -23.2, -64.7},
}};

// how much a mode's level against the fundamental varies from one field to the next: [A15] Tables 6.2-6.6 at the
// 'D' sweet spot, standard deviation 10.4 dB for the octave (33 fields), 8.8 dB for the compound fifth (23 fields)
constexpr double FIELD_LEVEL_SPREAD_DB = 10.0;
} // namespace grmh
