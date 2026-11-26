// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A song as it is played: its blocks one after the other from the song's start, each pattern as many times as the
// block says, each silence as many bars; all their strikes in one pattern, played once. Looped: the blocks selected
// (or the whole song) from the song's start, over and over.
#pragma once

#include <cmath>
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

    // the blocks played: the selection when it loops and there is one, else every block (first, last inclusive)
    static void played_range(const Arrangement& arrangement, int& first, int& last)
    {
        const int count = (int)arrangement.blocks.size();
        const bool selection = arrangement.selected_first >= 0 && arrangement.selected_last >= arrangement.selected_first &&
                               arrangement.selected_last < count;
        first = arrangement.loop && selection ? arrangement.selected_first : 0;
        last = arrangement.loop && selection ? arrangement.selected_last : count - 1;
    }

    // the place in the song (quarter notes) heard at the song's beat `beat`: itself, or within the loop
    static double song_place(const Arrangement& arrangement, const std::vector<BlockSpan>& spans, double beat)
    {
        if (!arrangement.loop || spans.empty())
            return beat;
        int first = 0, last = 0;
        played_range(arrangement, first, last);
        if (last < first)
            return beat;
        const double start = spans[(size_t)first].start;
        const double length = spans[(size_t)last].start + spans[(size_t)last].length - start;
        return length > 0.0 ? start + std::fmod(std::max(0.0, beat), length) : beat;
    }

    // the block playing at the place `place` of the song, -1 before the song or after its end
    static int block_at(const std::vector<BlockSpan>& spans, double place)
    {
        for (size_t i = 0; i < spans.size(); ++i)
            if (place >= spans[i].start && place < spans[i].start + spans[i].length)
                return (int)i;
        return -1;
    }

    static CompiledPattern compile(const Arrangement& arrangement, const PatternLibrary& library,
                                   const std::vector<int>& notes, const PlayerHands& hands)
    {
        CompiledPattern song;
        song.loop = arrangement.loop;
        const std::vector<BlockSpan> places = spans(arrangement, library);
        int first = 0, last = -1;
        played_range(arrangement, first, last);
        if (last < first)
            return song;
        const double origin = places[(size_t)first].start; // the loop starts with its first block
        for (int i = first; i <= last; ++i)
        {
            const ArrangementBlock& block = arrangement.blocks[(size_t)i];
            if (block.rest)
                continue;
            const Pattern& pattern = library.slot(block.slot).pattern;
            const std::vector<ScheduledStrike> once = PatternCompiler::compile(pattern, notes, hands);
            for (int r = 0; r < std::max(1, block.count); ++r)
                for (ScheduledStrike s : once)
                {
                    s.beat += places[(size_t)i].start - origin + r * pattern.length_beats();
                    song.strikes.push_back(s);
                }
        }
        song.length_beats = places[(size_t)last].start + places[(size_t)last].length - origin;
        return song;
    }
};
} // namespace grmh
