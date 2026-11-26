// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A song as it is played: its blocks one after the other from the song's start, each pattern as many times as the
// block says, each silence as many bars; all their strikes in one pattern, played once.
#pragma once

#include <vector>

#include "../constants/sequencer.hpp"
#include "../models/arrangement.hpp"
#include "../models/transport.hpp"
#include "pattern_compiler.hpp"
#include "pattern_library.hpp"

namespace grmh
{
class ArrangementCompiler
{
public:
    // a block's length, in quarter notes
    static double length(const ArrangementBlock& block, const PatternLibrary& library)
    {
        const double one = block.rest ? block.rest_bar_beats() : library.slot(block.slot).pattern.length_beats();
        return one * std::max(1, block.count);
    }

    // where each block lies in the song
    static std::vector<BlockSpan> spans(const Arrangement& arrangement, const PatternLibrary& library)
    {
        std::vector<BlockSpan> spans;
        double start = 0.0;
        for (const ArrangementBlock& block : arrangement.blocks)
        {
            spans.push_back({start, length(block, library)});
            start += spans.back().length;
        }
        return spans;
    }

    // the block playing at `beat`, -1 before the song or after its end
    static int block_at(const std::vector<BlockSpan>& spans, double beat)
    {
        for (size_t i = 0; i < spans.size(); ++i)
            if (beat >= spans[i].start && beat < spans[i].start + spans[i].length)
                return (int)i;
        return -1;
    }

    static CompiledPattern compile(const Arrangement& arrangement, const PatternLibrary& library,
                                   const std::vector<int>& notes, const PlayerHands& hands)
    {
        CompiledPattern song;
        song.loop = false;
        const std::vector<BlockSpan> places = spans(arrangement, library);
        for (size_t i = 0; i < arrangement.blocks.size(); ++i)
        {
            const ArrangementBlock& block = arrangement.blocks[i];
            if (block.rest)
                continue;
            const Pattern& pattern = library.slot(block.slot).pattern;
            const std::vector<ScheduledStrike> once = PatternCompiler::compile(pattern, notes, hands);
            for (int r = 0; r < std::max(1, block.count); ++r)
                for (ScheduledStrike s : once)
                {
                    s.beat += places[i].start + r * pattern.length_beats();
                    song.strikes.push_back(s);
                }
        }
        song.length_beats = places.empty() ? 0.0 : places.back().start + places.back().length;
        return song;
    }
};
} // namespace grmh
