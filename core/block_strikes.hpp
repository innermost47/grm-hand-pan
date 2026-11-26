// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The pattern's strikes that fall in one audio block, at their sample, in time order: what the audio thread plays
// between the host's MIDI notes. A fixed room, nothing allocated.
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
    // the strikes of `sequencer` in `block` (of `samples` samples); none while the clock is still
    void gather(const PatternSequencer& sequencer, const ClockBlock& block, int samples)
    {
        count_ = 0;
        if (!block.playing || block.beats_per_sample <= 0.0 || samples <= 0)
            return;
        const double end = block.start_beat + block.beats_per_sample * samples;
        sequencer.strikes_in(block.start_beat, end, block.tempo, [&](double beat, const ScheduledStrike& s)
                             {
                                 if (count_ == MAX_BLOCK_STRIKES)
                                     return;
                                 const int at = (int)std::floor((beat - block.start_beat) / block.beats_per_sample);
                                 strikes_[(size_t)count_++] = {std::clamp(at, 0, samples - 1), s.note, s.velocity};
                             });
        std::sort(strikes_.begin(), strikes_.begin() + count_,
                  [](const BlockStrike& a, const BlockStrike& b) { return a.sample < b.sample; });
    }

    int size() const { return count_; }
    const BlockStrike& operator[](int i) const { return strikes_[(size_t)i]; }

private:
    std::array<BlockStrike, MAX_BLOCK_STRIKES> strikes_{};
    int count_ = 0;
};
} // namespace grmh
