// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll's drawing: the ding in the middle, the fields alternating right and left as the scale rises,
// mirrored for a left-hander, smaller outwards; lines from the bottom up, each an arc lifted in its middle, a bar on
// screen; a hit's circle inside its field; the names of the notes.
#include <cmath>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/note_names.hpp"
#include "../core/roll_geometry.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
const std::vector<int> NOTES(D_KURD.begin(), D_KURD.end()); // D3 A3 Bb3 C4 D4 E4 F4 G4 A4

void check_columns(test::TestReport& report)
{
    const auto right = RollLayout::columns(NOTES, Handedness::right_handed);
    report.expect(right == std::vector<int>{0, 1, -1, 2, -2, 3, -3, 4, -4},
                  "the ding in the middle, then A3 right, Bb3 left, C4 right... A4 outermost left");
    const auto left = RollLayout::columns(NOTES, Handedness::left_handed);
    report.expect(left == std::vector<int>{0, -1, 1, -2, 2, -3, 3, -4, 4}, "mirrored for a left-hander");
    report.expect(RollLayout::reach(right) == 4, "four fields on each side");
    report.expect(RollLayout::circle_scale(0) == 1.0 && RollLayout::circle_scale(4) < RollLayout::circle_scale(1),
                  "the circles grow smaller outwards");
}

void check_geometry(test::TestReport& report)
{
    const RollGeometry geo(900.0, 1000.0, 16, 4, 0.0);
    report.expect(geo.row_y(0) > geo.row_y(1) && geo.row_y(15) > 0.0, "step 0 at the bottom, the bar's 16 lines on screen");
    report.expect(geo.centre(0, 0).y < geo.centre(0, 4).y && std::fabs(geo.centre(0, 4).y - geo.row_y(0)) < 1e-9,
                  "a line is an arc: its middle (the ding) higher than its ends");
    report.expect(std::fabs(geo.centre(3, -2).x + geo.centre(3, 2).x - 900.0) < 1e-9, "the arc is symmetric");
    bool apart = true;
    for (int c = -4; c <= 4; ++c)
        apart = apart && geo.centre(0, c).y - geo.centre(1, c).y >= 2.0 * geo.radius(c);
    report.expect(apart, "two lines' circles never overlap");

    const RollPoint edge = geo.hit_centre(2, 3, 1.0, -1.0, 0.4);
    const RollPoint field = geo.centre(2, 3);
    const double reach = std::hypot(edge.x - field.x, edge.y - field.y) / std::sqrt(2.0) + geo.hit_radius(3, 0.4);
    report.expect(reach <= geo.radius(3) + 1e-9, "a hit drawn at the field's edge stays inside it");
    report.expect(geo.hit_radius(0, 1.0) > geo.hit_radius(0, 0.0), "a louder hit, a bigger circle");

    const StepRange shown = geo.visible(32);
    report.expect(shown.first == 0 && shown.last == 15, "exactly a bar on screen out of two");
    const RollGeometry scrolled(900.0, 1000.0, 16, 4, 16.0);
    report.expect(scrolled.visible(32).first == 16 && scrolled.visible(32).last == 31 &&
                      RollGeometry::last_bottom(32, 16) == 16.0,
                  "scrolled to the second bar, the last one");
    bool inside = true;
    for (int c = -4; c <= 4; ++c)
        inside = inside && geo.centre(15, c).y - geo.radius(c) >= 0.0 && geo.row_y(0) + geo.radius(c) <= 1000.0;
    report.expect(inside, "the bar's lines, arcs included, fit in the view");
}

void check_names(test::TestReport& report)
{
    report.expect(name_of_midi(50) == "D3" && name_of_midi(58) == "B\xe2\x99\xad" "3" && name_of_midi(69) == "A4",
                  "D3, B\xe2\x99\xad" "3, A4");
    bool round_trip = true;
    for (int note : NOTES)
        round_trip = round_trip && (name_of_midi(note).size() > 3 || midi_of_name(name_of_midi(note)) == note);
    report.expect(round_trip, "a natural note's name reads back as its MIDI number");
}
} // namespace

int main()
{
    test::TestReport report("roll layout");
    check_columns(report);
    check_geometry(report);
    check_names(report);
    return report.finish();
}
