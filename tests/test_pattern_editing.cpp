// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The roll's changes to a pattern: two hits a step at most (a flam is two), several removed at once, a selection made
// louder and moved from the pattern as the gesture began (refused when a step would hold three hits or a hit would
// leave the pattern), copied, pasted, cloned, cleared; undone and redone, a whole gesture at a time.
#include <vector>

#include "../core/pattern_editor.hpp"
#include "../core/history.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
PatternHit hit(int step, int field, double size = 0.5)
{
    PatternHit h;
    h.step = step;
    h.field = field;
    h.size = size;
    return h;
}

void check_add_remove(test::TestReport& report)
{
    Pattern p;
    report.expect(PatternEditor::add(p, hit(3, 5)) == 0 && PatternEditor::add(p, hit(3, 5, 0.2)) == 1,
                  "two hits on one field at one step: a flam");
    report.expect(PatternEditor::add(p, hit(3, 2)) == -1 && p.hits.size() == 2, "a third hit at that step: refused");
    report.expect(PatternEditor::add(p, hit(16, 0)) == -1, "a hit past the pattern's end: refused");
    PatternEditor::add(p, hit(5, 1));
    PatternEditor::add(p, hit(7, 4));
    PatternEditor::remove(p, {0, 2, 2});
    report.expect(p.hits.size() == 2 && p.hits[0].step == 3 && p.hits[0].size == 0.2 && p.hits[1].step == 7,
                  "several hits removed at once, the others kept in order");
    PatternEditor::clear(p);
    report.expect(p.hits.empty(), "cleared: no hit left");
}

void check_gestures(test::TestReport& report)
{
    Pattern base;
    base.hits = {hit(0, 0, 0.5), hit(2, 4, 0.9), hit(4, 5, 0.3)};
    Pattern p;
    PatternEditor::resize(p, base, {0, 1}, 0.2);
    report.expect(p.hits[0].size == 0.7 && p.hits[1].size == 1.0 && p.hits[2].size == 0.3,
                  "a selection louder, from where the gesture began, never past the biggest");

    report.expect(PatternEditor::move(p, base, {0, 1}, 3, -1, 0.5, -0.25) && p.hits[0].step == 3 &&
                      p.hits[1].step == 5 && p.hits[1].field == 4 && p.hits[0].x == 0.5 && p.hits[0].y == -0.25,
                  "a selection moved three steps later, its fields kept");
    report.expect(PatternEditor::move(p, base, {2}, 1, 8, 0.0, 0.0) && p.hits[2].field == 8 && p.hits[2].step == 5,
                  "one hit moved onto another field");
    Pattern crowded = base;
    crowded.hits.push_back(hit(4, 1));
    report.expect(!PatternEditor::move(p, crowded, {0, 1}, 4, -1, 0.0, 0.0) && p == crowded,
                  "a move that would put three hits on a step: refused, the pattern as it was");
    report.expect(!PatternEditor::move(p, base, {2}, 12, -1, 0.0, 0.0) && p == base,
                  "a move past the pattern's end: refused");
    PatternEditor::set_hand(p, {0, 2}, HandChoice::left);
    report.expect(p.hits[0].hand == HandChoice::left && p.hits[1].hand == HandChoice::automatic &&
                      p.hits[2].hand == HandChoice::left,
                  "the selection given the left hand");
}

void check_clipboard(test::TestReport& report)
{
    Pattern p;
    p.hits = {hit(4, 0), hit(6, 5), hit(9, 2)};
    const auto clip = PatternEditor::copy(p, {0, 1});
    report.expect(clip.size() == 2 && clip[0].step == 0 && clip[1].step == 2, "copied: steps from the earliest");
    const auto pasted = PatternEditor::paste(p, clip, 12);
    report.expect(pasted == std::vector<int>{3, 4} && p.hits[3].step == 12 && p.hits[4].step == 14 &&
                      p.hits[4].field == 5,
                  "pasted from step 12, appended (the selection's indices hold)");
    const auto past = PatternEditor::paste(p, clip, 14);
    report.expect(past.size() == 1, "pasted near the end: what does not fit is left out");
    Pattern q;
    q.hits = {hit(0, 0), hit(1, 3)};
    const auto cloned = PatternEditor::clone(q, {0, 1});
    report.expect(cloned.size() == 2 && q.hits[2].step == 2 && q.hits[3].step == 3, "cloned right after itself");
}

void check_history(test::TestReport& report)
{
    History<Pattern> history;
    Pattern p;
    const Pattern empty = p;
    PatternEditor::add(p, hit(0, 0));
    history.record(empty, p);
    const Pattern one = p;
    PatternEditor::add(p, hit(2, 4));
    history.record(one, p);
    history.record(p, p); // no change: nothing recorded
    report.expect(history.undo(p) && p == one && history.undo(p) && p == empty && !history.undo(p),
                  "undone twice, back to the empty pattern, then nothing more to undo");
    report.expect(history.redo(p) && p == one, "redone");
    Pattern other = p;
    PatternEditor::add(other, hit(5, 6));
    history.record(p, other);
    report.expect(!history.can_redo(), "a new change forgets what was undone");
}
} // namespace

int main()
{
    test::TestReport report("pattern editing");
    check_add_remove(report);
    check_gestures(report);
    check_clipboard(report);
    check_history(report);
    return report.finish();
}
