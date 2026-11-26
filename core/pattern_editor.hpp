// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The changes the roll makes to a pattern. A player has two hands, a finger each: a step never holds more than two
// hits (a second hit on the same field is a flam). Hits are added at the end of the list, so that the indices of the
// others (a selection) still hold; a gesture (a drag) is computed from the pattern as it was when it began, never
// from its own last step, so that nothing drifts.
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

    // every step within the pattern and holding two hits at most
    static bool playable(const Pattern& pattern)
    {
        std::vector<int> count((size_t)std::max(0, pattern.step_count()), 0);
        for (const PatternHit& h : pattern.hits)
        {
            if (h.step < 0 || h.step >= pattern.step_count())
                return false;
            if (++count[(size_t)h.step] > MAX_HITS_PER_STEP)
                return false;
        }
        return true;
    }

    static std::vector<int> all(const Pattern& pattern)
    {
        std::vector<int> indices(pattern.hits.size());
        for (size_t i = 0; i < indices.size(); ++i)
            indices[i] = (int)i;
        return indices;
    }

    // the hit's index, or -1 when it does not fit (past the pattern, or a third hit at its step)
    static int add(Pattern& pattern, const PatternHit& hit)
    {
        if (hit.step < 0 || hit.step >= pattern.step_count() || hits_at(pattern, hit.step) >= MAX_HITS_PER_STEP)
            return -1;
        pattern.hits.push_back(hit);
        return (int)pattern.hits.size() - 1;
    }

    static void remove(Pattern& pattern, std::vector<int> indices)
    {
        std::sort(indices.begin(), indices.end(), std::greater<int>());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
        for (int i : indices)
            if (i >= 0 && i < (int)pattern.hits.size())
                pattern.hits.erase(pattern.hits.begin() + i);
    }

    // the selection `delta` louder (or softer), from the pattern as the gesture began
    static void resize(Pattern& pattern, const Pattern& base, const std::vector<int>& indices, double delta)
    {
        pattern = base;
        for (int i : indices)
            if (valid(base, i))
                pattern.hits[(size_t)i].size = std::clamp(base.hits[(size_t)i].size + delta, 0.0, 1.0);
    }

    // the selection moved, from the pattern as the gesture began: `steps` later (or earlier), (dx, dy) further on
    // their fields (in their radius), and onto `field` when it is one hit (-1: the fields stay); false, and the
    // pattern as it began, when a hit would leave the pattern or a step would hold more than two
    static bool move(Pattern& pattern, const Pattern& base, const std::vector<int>& indices, int steps, int field,
                     double dx, double dy)
    {
        Pattern moved = base;
        for (int i : indices)
        {
            if (!valid(base, i))
                continue;
            PatternHit& h = moved.hits[(size_t)i];
            h.step += steps;
            if (field >= 0 && indices.size() == 1)
                h.field = field;
            h.x = std::clamp(h.x + dx, -1.0, 1.0);
            h.y = std::clamp(h.y + dy, -1.0, 1.0);
        }
        const bool fits = playable(moved);
        pattern = fits ? moved : base;
        return fits;
    }

    static void set_hand(Pattern& pattern, const std::vector<int>& indices, HandChoice hand)
    {
        for (int i : indices)
            if (valid(pattern, i))
                pattern.hits[(size_t)i].hand = hand;
    }

    // the selected hits, their steps from the earliest one (a clipboard)
    static std::vector<PatternHit> copy(const Pattern& pattern, const std::vector<int>& indices)
    {
        std::vector<PatternHit> clip;
        int first = pattern.step_count();
        for (int i : indices)
            if (valid(pattern, i))
            {
                clip.push_back(pattern.hits[(size_t)i]);
                first = std::min(first, pattern.hits[(size_t)i].step);
            }
        for (PatternHit& h : clip)
            h.step -= first;
        return clip;
    }

    // a clipboard put from `step` on; the indices of the hits that fit (the others are left out)
    static std::vector<int> paste(Pattern& pattern, const std::vector<PatternHit>& clip, int step)
    {
        std::vector<int> added;
        for (PatternHit h : clip)
        {
            h.step += step;
            const int index = add(pattern, h);
            if (index >= 0)
                added.push_back(index);
        }
        return added;
    }

    // the selection again, right after itself; the indices of the copies that fit
    static std::vector<int> clone(Pattern& pattern, const std::vector<int>& indices)
    {
        int last = -1;
        for (int i : indices)
            if (valid(pattern, i))
                last = std::max(last, pattern.hits[(size_t)i].step);
        if (last < 0)
            return {};
        return paste(pattern, copy(pattern, indices), last + 1);
    }

    static void clear(Pattern& pattern) { pattern.hits.clear(); }

private:
    static bool valid(const Pattern& pattern, int i) { return i >= 0 && i < (int)pattern.hits.size(); }
};
} // namespace grmh
