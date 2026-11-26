// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A song: blocks one after the other, each a pattern played a number of times, or a silence of some bars.
#pragma once

#include <vector>

#include "library.hpp"

namespace grmh
{
struct ArrangementBlock
{
    bool rest = false;       // a silence
    SlotId slot;             // the pattern (its bank is the variation: A, B, C, D)
    int count = 1;           // a pattern: how many times it plays; a silence: how many bars
    TimeSignature signature; // a silence's bars

    // a silence's bar, in quarter notes
    double rest_bar_beats() const { return 4.0 * signature.numerator / signature.denominator; }

    bool operator==(const ArrangementBlock& o) const
    {
        return rest == o.rest && count == o.count && (rest ? signature == o.signature : slot == o.slot);
    }
};

struct Arrangement
{
    std::vector<ArrangementBlock> blocks;
    bool loop = false;                     // the blocks selected (or the whole song) played over and over
    int selected_first = -1, selected_last = -1; // the blocks selected, inclusive; -1: none

    bool operator==(const Arrangement& o) const
    {
        return blocks == o.blocks && loop == o.loop && selected_first == o.selected_first &&
               selected_last == o.selected_last;
    }
    bool operator!=(const Arrangement& o) const { return !(*this == o); }
};

// where a block lies in the song, in quarter notes
struct BlockSpan
{
    double start = 0.0, length = 0.0;
};
} // namespace grmh
