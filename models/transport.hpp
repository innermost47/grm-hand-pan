// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Where the song is: what the host says, what one audio block covers, and the strikes that fall in it.
#pragma once

#include <vector>

#include "pattern.hpp"

namespace grmh
{
// what the host says at the start of a block (available false: no host clock, as in the standalone)
struct HostPosition
{
    bool available = false;
    bool playing = false;
    double beat = 0.0;  // quarter notes from the song's start (ppq)
    double tempo = 0.0; // BPM, 0 when unknown
};

// one audio block in the song
struct ClockBlock
{
    bool playing = false;
    double start_beat = 0.0; // quarter notes
    double tempo = 0.0;      // BPM
    double beats_per_sample = 0.0;
};

// a strike of the pattern in a block
struct BlockStrike
{
    int sample = 0; // from the block's start
    int note = 0;   // MIDI
    int velocity = 0;
};

// a place in the song as a musician reads it, each counted from 1
struct SongPosition
{
    int bar = 1, beat = 1, step = 1;
};

// a pattern ready for the audio thread
struct CompiledPattern
{
    std::vector<ScheduledStrike> strikes;
    double length_beats = 0.0;
};
} // namespace grmh
