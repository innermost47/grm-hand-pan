// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The changes the roll makes to a pattern: a hit added (refused past two at one step: a player has two hands, a finger
// each; a second hit on the same field is a flam), removed, moved on its field, made louder or softer, given a hand.
#pragma once

#include <algorithm>
#include <vector>

#include "../constants/sequencer.hpp"
#include "../models/pattern.hpp"

namespace grmh
{
class PatternEditor
{
public:
    static int hits_at(const Pattern& pattern, int step)
    {
        return (int)std::count_if(pattern.hits.begin(), pattern.hits.end(),
                                  [step](const PatternHit& h) { return h.step == step; });
    }

    // the hit's index, or -1 when the step already holds two
    static int add(Pattern& pattern, const PatternHit& hit)
    {
        if (hit.step < 0 || hit.step >= pattern.step_count() || hits_at(pattern, hit.step) >= MAX_HITS_PER_STEP)
            return -1;
        pattern.hits.push_back(hit);
        return (int)pattern.hits.size() - 1;
    }

    static void remove(Pattern& pattern, int index)
    {
        if (index >= 0 && index < (int)pattern.hits.size())
            pattern.hits.erase(pattern.hits.begin() + index);
    }

    // where on its field (in its radius, -1..1)
    static void move(Pattern& pattern, int index, double x, double y)
    {
        if (index < 0 || index >= (int)pattern.hits.size())
            return;
        pattern.hits[(size_t)index].x = std::clamp(x, -1.0, 1.0);
        pattern.hits[(size_t)index].y = std::clamp(y, -1.0, 1.0);
    }

    static void resize(Pattern& pattern, int index, double size)
    {
        if (index >= 0 && index < (int)pattern.hits.size())
            pattern.hits[(size_t)index].size = std::clamp(size, 0.0, 1.0);
    }

    static void set_hand(Pattern& pattern, int index, HandChoice hand)
    {
        if (index >= 0 && index < (int)pattern.hits.size())
            pattern.hits[(size_t)index].hand = hand;
    }
};
} // namespace grmh
