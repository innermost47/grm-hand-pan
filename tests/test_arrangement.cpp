// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The arranger's song: blocks one after the other, a pattern as many times as its block says, a silence as many bars;
// all played once from the song's start; blocks added, moved, played more or fewer times, turned to the next
// variation, removed.
#include <vector>

#include "../constants/scales.hpp"
#include "../core/arrangement_compiler.hpp"
#include "../core/arrangement_editor.hpp"
#include "../core/pattern_sequencer.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
const std::vector<int> NOTES(D_KURD.begin(), D_KURD.end());

// A1: one bar, the ding on its first step; A2: two bars, E4 on its fifth step (beat 1)
PatternLibrary library()
{
    PatternLibrary lib;
    PatternHit ding, e4;
    e4.step = 4;
    e4.field = 5;
    lib.slot({0, 0}).pattern.hits = {ding};
    lib.slot({0, 1}).pattern.bars = 2;
    lib.slot({0, 1}).pattern.hits = {e4};
    return lib;
}

void check_song(test::TestReport& report)
{
    const PatternLibrary lib = library();
    Arrangement song;
    ArrangementEditor::add_pattern(song, {0, 0});
    ArrangementEditor::set_count(song, 0, 2);
    ArrangementEditor::add_rest(song);
    ArrangementEditor::add_pattern(song, {0, 1});
    const auto spans = ArrangementCompiler::spans(song, lib);
    report.expect(spans.size() == 3 && spans[0].start == 0.0 && spans[0].length == 8.0 && spans[1].start == 8.0 &&
                      spans[1].length == 4.0 && spans[2].start == 12.0 && spans[2].length == 8.0,
                  "A1 twice (8 beats), a bar of silence, A2 (two bars): blocks one after the other");
    report.expect(ArrangementCompiler::block_at(spans, 9.0) == 1 && ArrangementCompiler::block_at(spans, 20.0) == -1,
                  "beat 9 is in the silence; beat 20 is past the song's end");

    const CompiledPattern compiled = ArrangementCompiler::compile(song, lib, NOTES, PlayerHands{});
    std::vector<double> beats;
    for (const ScheduledStrike& s : compiled.strikes)
        beats.push_back(s.beat);
    report.expect(beats == std::vector<double>{0.0, 4.0, 13.0} && compiled.length_beats == 20.0 && !compiled.loop,
                  "the song's strikes: the ding at 0 and 4, E4 at 13; twenty beats, played once");

    PatternSequencer sequencer;
    sequencer.set(&compiled.strikes, compiled.length_beats, compiled.loop);
    int heard = 0;
    sequencer.strikes_in(0.0, 100.0, 120.0, [&](double, const ScheduledStrike&) { ++heard; });
    report.expect(heard == 3, "played once: nothing after its end");
}

// the loop: the blocks selected (or the whole song), from the song's start, over and over
void check_loop(test::TestReport& report)
{
    const PatternLibrary lib = library();
    Arrangement song;
    ArrangementEditor::add_pattern(song, {0, 0}); // 0-4
    ArrangementEditor::add_rest(song);            // 4-8
    ArrangementEditor::add_pattern(song, {0, 1}); // 8-16, E4 at 9
    ArrangementEditor::set_loop(song, true);
    const CompiledPattern whole = ArrangementCompiler::compile(song, lib, NOTES, PlayerHands{});
    report.expect(whole.loop && whole.length_beats == 16.0 && whole.strikes.size() == 2,
                  "Loop without a selection: the whole song, sixteen beats, over and over");

    ArrangementEditor::select(song, 2, 1); // in either order
    report.expect(song.selected_first == 1 && song.selected_last == 2, "the silence and A2 selected");
    const CompiledPattern part = ArrangementCompiler::compile(song, lib, NOTES, PlayerHands{});
    report.expect(part.loop && part.length_beats == 12.0 && part.strikes.size() == 1 && part.strikes[0].beat == 5.0,
                  "Loop with a selection: the silence then A2, twelve beats, E4 at 5 from the loop's start");
    const auto spans = ArrangementCompiler::spans(song, lib);
    const double place = ArrangementCompiler::song_place(song, spans, 12.0 + 5.0);
    report.expect(place == 9.0 && ArrangementCompiler::block_at(spans, place) == 2,
                  "beat 17: the second time round, five beats in: A2, the song's beat 9");

    ArrangementEditor::select(song, 5, 1);
    report.expect(song.selected_first == -1, "a block that does not exist: nothing selected");
    ArrangementEditor::select(song, 0, 0);
    ArrangementEditor::remove(song, 1);
    report.expect(song.selected_first == -1, "a block removed: the selection let go");
}

void check_editing(test::TestReport& report)
{
    Arrangement song;
    ArrangementEditor::add_pattern(song, {0, 1});
    ArrangementEditor::add_rest(song);
    report.expect(ArrangementEditor::add_pattern(song, {2, 3}, 1) == 1 && song.blocks[1].slot == SlotId{2, 3} &&
                      song.blocks[2].rest,
                  "a pattern dropped between two blocks goes there");
    ArrangementEditor::move(song, 0, 2);
    report.expect(song.blocks[0].slot == SlotId{2, 3} && song.blocks[1].rest && song.blocks[2].slot == SlotId{0, 1},
                  "the first block moved to the end");
    ArrangementEditor::set_count(song, 0, 99);
    ArrangementEditor::set_count(song, 1, 0);
    report.expect(song.blocks[0].count == BLOCK_COUNT_MAX && song.blocks[1].count == 1,
                  "played sixteen times at most, once at least");
    ArrangementEditor::next_variation(song, 0);
    report.expect(song.blocks[0].slot == SlotId{3, 3}, "C4's next variation: D4");
    ArrangementEditor::next_variation(song, 0);
    report.expect(song.blocks[0].slot == SlotId{0, 3}, "D4's next variation: back to A4");
    ArrangementEditor::set_count(song, 1, 2);
    ArrangementEditor::set_signature(song, 1, {3, 4});
    const PatternLibrary empty;
    report.expect(song.blocks[1].signature == TimeSignature{3, 4} &&
                      ArrangementCompiler::length(song.blocks[1], empty) == 6.0,
                  "two bars of silence in 3/4: six quarter notes");
    ArrangementEditor::set_signature(song, 1, {6, 8});
    report.expect(ArrangementCompiler::length(song.blocks[1], empty) == 6.0, "in 6/8: six quarter notes too");
    ArrangementEditor::set_signature(song, 0, {3, 4});
    report.expect(song.blocks[0].signature == TimeSignature{}, "a pattern's block has no signature of its own");
    ArrangementEditor::remove(song, 1);
    report.expect(song.blocks.size() == 2 && !song.blocks[1].rest, "the silence removed");
}
} // namespace

int main()
{
    test::TestReport report("arrangement");
    check_song(report);
    check_loop(report);
    check_editing(report);
    return report.finish();
}
