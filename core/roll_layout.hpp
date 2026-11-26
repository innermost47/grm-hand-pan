// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Where each field sits on a line of the Hand Pan Roll (design/README.md): the ding in the middle, the other fields in
// satellite, alternating right and left as the scale rises (the lowest on the right, as it is struck by the right
// hand), so that each side grows smaller and higher outwards; a left-hander sees it mirrored.
#pragma once

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <vector>

#include "../constants/sequencer.hpp"
#include "../models/pattern.hpp"

namespace grmh
{
class RollLayout
{
public:
    // the column of each field (in the order of `notes`, the ding first): 0 the middle, + right, - left
    static std::vector<int> columns(const std::vector<int>& notes, Handedness handedness)
    {
        std::vector<int> columns(notes.size(), 0);
        std::vector<size_t> ring(notes.size() > 0 ? notes.size() - 1 : 0);
        std::iota(ring.begin(), ring.end(), size_t{1});
        std::stable_sort(ring.begin(), ring.end(), [&](size_t a, size_t b) { return notes[a] < notes[b]; });
        const int mirror = handedness == Handedness::right_handed ? 1 : -1;
        for (size_t k = 0; k < ring.size(); ++k)
        {
            const int side = k % 2 == 0 ? 1 : -1; // right first
            columns[ring[k]] = mirror * side * (int)(k / 2 + 1);
        }
        return columns;
    }

    // the most columns on either side
    static int reach(const std::vector<int>& columns)
    {
        int most = 0;
        for (int c : columns)
            most = std::max(most, std::abs(c));
        return most;
    }

    // a field's circle, relative to the ding's: smaller outwards
    static double circle_scale(int column) { return 1.0 - ROLL_CIRCLE_SHRINK * std::abs(column); }
};
} // namespace grmh
