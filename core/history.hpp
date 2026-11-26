// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What can be undone and redone (a pattern in the roll, the song in the arranger): the states before each change (a
// whole gesture is one change), the oldest forgotten past HISTORY_LIMIT. A new change forgets what was undone.
#pragma once

#include <vector>

#include "../constants/sequencer.hpp"

namespace grmh
{
template <class State>
class History
{
public:
    // a change made: `before` is what undo brings back (nothing when nothing changed)
    void record(const State& before, const State& after)
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
    bool undo(State& current) { return step(undo_, redo_, current); }
    bool redo(State& current) { return step(redo_, undo_, current); }

private:
    static bool step(std::vector<State>& from, std::vector<State>& to, State& current)
    {
        if (from.empty())
            return false;
        to.push_back(current);
        current = from.back();
        from.pop_back();
        return true;
    }

    std::vector<State> undo_, redo_;
};
} // namespace grmh
