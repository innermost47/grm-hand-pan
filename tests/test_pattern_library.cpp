// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The banks of patterns: four banks of sixteen, their names (their own or "Pattern N"), the one edited, a used one;
// a pattern made longer or shorter (its hits past the end removed), or put in another signature.
#include "../core/pattern_editor.hpp"
#include "../core/pattern_library.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
void check_library(test::TestReport& report)
{
    PatternLibrary library;
    report.expect(library.current() == SlotId{0, 0} && PatternLibrary::bank_letter(3) == 'D',
                  "four banks, A to D, the first pattern of A edited first");
    report.expect(library.name({1, 2}) == "Pattern 3" && !library.used({1, 2}), "an untouched pattern: 'Pattern 3'");
    library.slot({1, 2}).name = "Verse";
    PatternHit hit;
    library.slot({1, 2}).pattern.hits.push_back(hit);
    report.expect(library.name({1, 2}) == "Verse" && library.used({1, 2}), "named, with a hit: used");
    library.select({1, 2});
    report.expect(library.current() == SlotId{1, 2} && library.pattern().hits.size() == 1,
                  "chosen: the pattern edited and played");
    library.select({7, 2});
    report.expect(library.current() == SlotId{1, 2}, "a bank that does not exist: nothing changes");
}

void check_length(test::TestReport& report)
{
    Pattern p;
    p.bars = 2;
    PatternHit early, late;
    early.step = 3;
    late.step = 20;
    p.hits = {early, late};
    PatternEditor::set_bars(p, 1);
    report.expect(p.bars == 1 && p.hits.size() == 1 && p.hits[0].step == 3, "one bar: the hits of the second removed");
    PatternEditor::set_bars(p, 0);
    report.expect(p.bars == 1, "never shorter than a bar");
    PatternEditor::set_signature(p, {3, 4}, 4);
    report.expect(p.step_count() == 12 && p.hits.size() == 1, "in 3/4: twelve sixteenths, the hit kept");
}
} // namespace

int main()
{
    test::TestReport report("pattern library");
    check_library(report);
    check_length(report);
    return report.finish();
}
