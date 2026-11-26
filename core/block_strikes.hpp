// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The patterns' strikes that fall in one audio block, at their sample, in time order: what the audio thread plays
// between the host's MIDI notes. A block can hold the end of a pattern and the start of the next one (a pattern
// switched on a bar line): each is added over its own stretch of samples, from its own origin. A fixed room,
// nothing allocated.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include "../constants/sequencer.hpp"
#include "../models/transport.hpp"
#include "pattern_sequencer.hpp"

namespace grmh
{
class BlockStrikes
{
public:
    void clear() { count_ = 0; }

    // the strikes of `sequencer` started at the song's beat `origin`, from the song's beat `from` to `to` (within
    // `block`, of `samples` samples): a stretch ends on a bar line exactly, never on the sample after it
    void add(const PatternSequencer& sequencer, const ClockBlock& block, int samples, double origin, double from,
             double to)
    {
        if (!block.playing || block.beats_per_sample <= 0.0 || to <= from || samples <= 0)
            return;
        sequencer.strikes_in(from - origin, to - origin, block.tempo, [&](double beat, const ScheduledStrike& s)
                             {
                                 if (count_ == MAX_BLOCK_STRIKES)
                                     return;
                                 const double song_beat = beat + origin;
                                 const int at = (int)std::floor((song_beat - block.start_beat) / block.beats_per_sample);
                                 strikes_[(size_t)count_++] = {std::clamp(at, 0, samples - 1), s.note, s.velocity};
                             });
    }

    // the song's beat at the end of `block`
    static double end_beat(const ClockBlock& block, int samples)
    {
        return block.start_beat + block.beats_per_sample * samples;
    }

    // in time order, once every stretch is added
    void sort()
    {
        std::stable_sort(strikes_.begin(), strikes_.begin() + count_,
                         [](const BlockStrike& a, const BlockStrike& b) { return a.sample < b.sample; });
    }

    // a block of one pattern looped from the song's start
    void gather(const PatternSequencer& sequencer, const ClockBlock& block, int samples)
    {
        clear();
        add(sequencer, block, samples, 0.0, block.start_beat, end_beat(block, samples));
        sort();
    }

    int size() const { return count_; }
    const BlockStrike& operator[](int i) const { return strikes_[(size_t)i]; }

private:
    std::array<BlockStrike, MAX_BLOCK_STRIKES> strikes_{};
    int count_ = 0;
};
} // namespace grmh
