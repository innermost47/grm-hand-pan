// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll as it is drawn: a point of the view, and the range of steps it shows.
#pragma once

namespace grmh
{
struct RollPoint
{
    double x = 0.0, y = 0.0;
};

// the steps the view shows, bottom (played first) to top
struct StepRange
{
    int first = 0, last = -1; // inclusive; empty when last < first
};
} // namespace grmh
