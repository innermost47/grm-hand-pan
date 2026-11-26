// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A beat of the song (quarter notes) as bar, beat and step of a pattern's grid.
#pragma once

#include <algorithm>
#include <cmath>

#include "../models/pattern.hpp"
#include "../models/transport.hpp"

namespace grmh
{
inline SongPosition song_position(double quarter_notes, const Pattern& grid)
{
    const double step_beats = grid.step_beats();
    const long steps = std::lround(std::floor(std::max(0.0, quarter_notes) / step_beats + 1e-9));
    const long per_bar = grid.steps_per_bar();
    SongPosition p;
    p.bar = (int)(steps / per_bar) + 1;
    p.beat = (int)((steps % per_bar) / grid.steps_per_beat) + 1;
    p.step = (int)(steps % grid.steps_per_beat) + 1;
    return p;
}
} // namespace grmh
