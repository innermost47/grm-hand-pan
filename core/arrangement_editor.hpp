// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The changes the arranger makes to a song: a pattern or a silence added (at the end, or at a place), a block removed,
// moved, played more or fewer times, or turned to the next variation (the same pattern in the next bank).
#pragma once

#include <algorithm>

#include "../constants/sequencer.hpp"
#include "../models/arrangement.hpp"

namespace grmh
{
class ArrangementEditor
{
public:
    // the new block's index; `at` -1: at the end
    static int add_pattern(Arrangement& arrangement, SlotId slot, int at = -1)
    {
        ArrangementBlock block;
        block.slot = slot;
        return insert(arrangement, block, at);
    }

    static int add_rest(Arrangement& arrangement, int at = -1)
    {
        ArrangementBlock block;
        block.rest = true;
        return insert(arrangement, block, at);
    }

    static void remove(Arrangement& arrangement, int index)
    {
        if (!valid(arrangement, index))
            return;
        arrangement.blocks.erase(arrangement.blocks.begin() + index);
        select(arrangement, -1, -1);
    }

    // block `from` put at `to` (its index once moved)
    static void move(Arrangement& arrangement, int from, int to)
    {
        if (!valid(arrangement, from))
            return;
        const ArrangementBlock block = arrangement.blocks[(size_t)from];
        arrangement.blocks.erase(arrangement.blocks.begin() + from);
        to = std::clamp(to, 0, (int)arrangement.blocks.size());
        arrangement.blocks.insert(arrangement.blocks.begin() + to, block);
        select(arrangement, to, to);
    }

    // the blocks from `first` to `last` selected (in either order; -1: none)
    static void select(Arrangement& arrangement, int first, int last)
    {
        if (!valid(arrangement, first) || !valid(arrangement, last))
        {
            arrangement.selected_first = arrangement.selected_last = -1;
            return;
        }
        arrangement.selected_first = std::min(first, last);
        arrangement.selected_last = std::max(first, last);
    }

    static void set_loop(Arrangement& arrangement, bool loop) { arrangement.loop = loop; }

    static void set_count(Arrangement& arrangement, int index, int count)
    {
        if (valid(arrangement, index))
            arrangement.blocks[(size_t)index].count = std::clamp(count, 1, BLOCK_COUNT_MAX);
    }

    // a silence's bars in another signature
    static void set_signature(Arrangement& arrangement, int index, TimeSignature signature)
    {
        if (valid(arrangement, index) && arrangement.blocks[(size_t)index].rest)
            arrangement.blocks[(size_t)index].signature = signature;
    }

    // the same pattern in the next bank: A2 -> B2 -> C2 -> D2 -> A2
    static void next_variation(Arrangement& arrangement, int index)
    {
        if (valid(arrangement, index) && !arrangement.blocks[(size_t)index].rest)
        {
            SlotId& slot = arrangement.blocks[(size_t)index].slot;
            slot.bank = (slot.bank + 1) % BANKS;
        }
    }

private:
    static bool valid(const Arrangement& arrangement, int index)
    {
        return index >= 0 && index < (int)arrangement.blocks.size();
    }

    static int insert(Arrangement& arrangement, const ArrangementBlock& block, int at)
    {
        const int place = at < 0 ? (int)arrangement.blocks.size() : std::min(at, (int)arrangement.blocks.size());
        arrangement.blocks.insert(arrangement.blocks.begin() + place, block);
        return place;
    }
};
} // namespace grmh
