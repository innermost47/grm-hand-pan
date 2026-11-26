// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Which pattern plays along the song, when patterns are asked for (a MIDI key, later the arranger): a pattern asked
// for starts on the next bar line (its own bar, counted from the song's start), from its first step; one asked for
// while another plays takes over on that bar line; a pattern let go stops at once (what rings, rings on). Audio
// thread: nothing allocated.
#pragma once

#include <cmath>

#include "../models/transport.hpp"
#include "block_strikes.hpp"

namespace grmh
{
class PatternPlayer
{
public:
    static constexpr int NONE = -1;

    // pattern `index` from the next bar line
    void request(int index) { requested_ = index; }

    // pattern `index` let go: it stops now, and is no longer awaited
    void release(int index)
    {
        if (playing_ == index)
            playing_ = NONE;
        if (requested_ == index)
            requested_ = NONE;
    }

    void stop() { playing_ = requested_ = NONE; }

    int playing() const { return playing_; }
    int requested() const { return requested_; }

    // the block's strikes into `out`: the pattern playing, then the one asked for from its bar line on
    void gather(const CompiledLibrary& library, const ClockBlock& block, int samples, BlockStrikes& out)
    {
        out.clear();
        if (!block.playing || block.beats_per_sample <= 0.0 || samples <= 0)
            return;
        const double end = BlockStrikes::end_beat(block, samples);
        double from = block.start_beat;
        if (valid(library, requested_))
        {
            const double bar = library.patterns[(size_t)requested_].bar_beats;
            const double line = std::ceil(block.start_beat / bar - 1e-9) * bar;
            if (line < end)
            {
                add(library, playing_, block, samples, from, line, out);
                playing_ = requested_;
                origin_ = line;
                requested_ = NONE;
                from = line;
            }
        }
        add(library, playing_, block, samples, from, end, out);
        out.sort();
    }

private:
    static bool valid(const CompiledLibrary& library, int index)
    {
        return index >= 0 && index < (int)library.patterns.size();
    }

    // pattern `index`'s strikes from the song's beat `from` to `to`
    void add(const CompiledLibrary& library, int index, const ClockBlock& block, int samples, double from, double to,
             BlockStrikes& out) const
    {
        if (!valid(library, index))
            return;
        const CompiledPattern& p = library.patterns[(size_t)index];
        PatternSequencer sequencer;
        sequencer.set(&p.strikes, p.length_beats);
        out.add(sequencer, block, samples, origin_, from, to);
    }

    int playing_ = NONE, requested_ = NONE;
    double origin_ = 0.0; // the song's beat where the pattern playing started
};
} // namespace grmh
