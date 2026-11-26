// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The arranger (design/README.md, the zone on the right): the song as blocks read from the top down, each a pattern
// played a number of times or a silence of some bars. "Song" plays it (once, from the song's start) instead of the
// pattern being edited; the block playing shows in vermilion, and the roll follows its pattern. "+ Pattern" adds the
// pattern being edited, "+ Rest" a bar of silence; a pattern dragged from the zone on the left is dropped where it
// falls; a block dragged moves. On a block: its letter turns to the next variation (A2 -> B2), − / + play it fewer or
// more times, × removes it, a click selects it and edits its pattern (on a silence: chooses its bars' signature),
// Shift + click selects up to it; "Loop" plays the blocks selected (or the whole song) over and over. Ctrl + Z undoes,
// Ctrl + Shift + Z or Ctrl + Y redoes. "Export" writes the song to a WAV file. The list scrolls. Refreshed at the
// screen's rhythm (VBlankAttachment).
#pragma once

#include <memory>
#include <vector>

#include "../../core/history.hpp"
#include "../HandPanProcessor.h"
#include "../JuceHeader.h"
#include "../widgets/ArrangementRow.h"

class ArrangerView : public juce::Component, public juce::DragAndDropTarget
{
public:
    explicit ArrangerView(HandPanProcessor& processor);

    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

    // the arranger's commands (its buttons, its blocks, a drop), for the view and for tests
    void add_edited();
    void add_rest();
    void drop_pattern(int pattern, int at); // pattern: bank * PATTERNS_PER_BANK + slot
    void next_variation(int block);
    void change_count(int block, int delta);
    void remove(int block);
    void move(int from, int to);
    void edit(int block);
    void set_rest_signature(int block, grmh::TimeSignature signature);
    void set_loop(bool loop);
    void select(int block, bool extend); // a click on a block (extend: Shift held)
    void undo();
    void redo();

    bool keyPressed(const juce::KeyPress& key) override;
    // the list taller than the view: it scrolls (for tests)
    bool scrolls() const { return list_.getHeight() > viewport_.getHeight(); }
    juce::Viewport& viewport() { return viewport_; } // for tests

    // every frame (VBlankAttachment): the blocks, the one playing, the roll following it
    void refresh();
    const ArrangementRow& row(int block) const { return *rows_[(size_t)block]; } // for tests
    int row_count() const { return (int)processor_.arrangement().blocks.size(); }

private:
    void change(const std::function<void(grmh::Arrangement&)>& edit);
    void show_signature_menu(int block);
    void rebuild();
    void layout_rows();
    int index_at(int y_in_view) const; // where a block dropped at this height goes
    int playing_block() const;         // -1: none

    HandPanProcessor& processor_;
    juce::ToggleButton song_, loop_;
    juce::TextButton add_pattern_{"+ Pattern"}, add_rest_{"+ Rest"}, export_{"Export"};
    grmh::History<grmh::Arrangement> history_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> song_attachment_;
    juce::Viewport viewport_;
    juce::Component list_;
    std::vector<std::unique_ptr<ArrangementRow>> rows_;
    int shown_version_ = -1;
    int followed_block_ = -1;
    int drop_index_ = -1; // where a drag would put its block (-1: no drag)

    juce::VBlankAttachment vblank_{this, [this] { refresh(); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerView)
};
