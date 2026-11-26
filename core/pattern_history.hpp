// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What the roll can undo and redo: the patterns before each change (a whole gesture, a drag, is one change), the
// oldest forgotten past HISTORY_LIMIT. A new change forgets what was undone.
#pragma once

#include <vector>

#include "../constants/sequencer.hpp"
#include "../models/pattern.hpp"

namespace grmh
{
class PatternHistory
{
public:
    // a change made: `before` is what undo brings back (nothing when the pattern did not change)
    void record(const Pattern& before, const Pattern& after)
    {
        if (before == after)
            return;
        undo_.push_back(before);
        if ((int)undo_.size() > HISTORY_LIMIT)
            undo_.erase(undo_.begin());
        redo_.clear();
    }

    bool can_undo() const { return !undo_.empty(); }
    bool can_redo() const { return !redo_.empty(); }

    // `current` back to what it was before the last change; false when there is none
    bool undo(Pattern& current) { return step(undo_, redo_, current); }
    bool redo(Pattern& current) { return step(redo_, undo_, current); }

private:
    static bool step(std::vector<Pattern>& from, std::vector<Pattern>& to, Pattern& current)
    {
        if (from.empty())
            return false;
        to.push_back(current);
        current = from.back();
        from.pop_back();
        return true;
    }

    std::vector<Pattern> undo_, redo_;
};
} // namespace grmh
