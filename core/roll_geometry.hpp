// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll in numbers: one line a step, read from the bottom (played first) up, a bar filling the view; each
// line an arc, its middle lifted (the ding higher than the fields outwards); the view scrolls by steps. All in the
// view's pixels, without a graphics library.
#pragma once

#include <algorithm>
#include <cmath>

#include "../models/roll.hpp"
#include "roll_layout.hpp"

namespace grmh
{
class RollGeometry
{
public:
    // a view of `width` x `height`, `steps_per_bar` lines on screen, `reach` columns on each side of the middle, the
    // step `bottom` at the bottom line
    RollGeometry(double width, double height, int steps_per_bar, int reach, double bottom)
        : width_(width), height_(height), lines_(std::max(1, steps_per_bar)), reach_(std::max(1, reach)), bottom_(bottom)
    {
        row_height_ = height_ / (lines_ + ROLL_ARC_LIFT);
        column_spacing_ = width_ / (2.0 * reach_ + 1.0);
        base_radius_ = ROLL_CIRCLE_FILL * std::min(row_height_, column_spacing_);
    }

    double row_height() const { return row_height_; }
    double column_spacing() const { return column_spacing_; }

    // the baseline of a step's line (its outermost circles' centre)
    double row_y(double step) const { return height_ - (step - bottom_ + 0.5) * row_height_; }

    // the centre of a field's circle on a step's line
    RollPoint centre(int step, int column) const
    {
        const double share = double(column) / reach_;
        return {width_ * 0.5 + column * column_spacing_,
                row_y(step) - ROLL_ARC_LIFT * row_height_ * (1.0 - share * share)};
    }

    double radius(int column) const { return base_radius_ * RollLayout::circle_scale(column); }

    // a hit's circle (size 0..1) inside a field's
    double hit_radius(int column, double size) const
    {
        return radius(column) * (HIT_RADIUS_MIN + std::clamp(size, 0.0, 1.0) * (HIT_RADIUS_MAX - HIT_RADIUS_MIN));
    }

    // where a hit drawn at (x, y) (in the field's radius) sits, so that its circle stays inside the field's
    RollPoint hit_centre(int step, int column, double x, double y, double size) const
    {
        const RollPoint c = centre(step, column);
        const double room = radius(column) - hit_radius(column, size);
        return {c.x + std::clamp(x, -1.0, 1.0) * room, c.y + std::clamp(y, -1.0, 1.0) * room};
    }

    // the steps on screen (a bar's worth of lines from the bottom one), of a pattern of `step_count` steps
    StepRange visible(int step_count) const
    {
        StepRange range;
        range.first = std::max(0, (int)std::floor(bottom_));
        range.last = std::min(step_count - 1, range.first + lines_ - 1);
        return range;
    }

    // how far the view can scroll: the bottom step of the last screen
    static double last_bottom(int step_count, int steps_per_bar)
    {
        return std::max(0.0, double(step_count - steps_per_bar));
    }

private:
    double width_, height_;
    int lines_, reach_;
    double bottom_;
    double row_height_ = 0.0, column_spacing_ = 0.0, base_radius_ = 0.0;
};
} // namespace grmh
