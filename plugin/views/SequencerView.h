// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll (design/README.md): one line a step, read from the bottom (played first) up, a bar on screen; on
// each line the instrument's fields in an arc, the ding in the middle, the others in satellite right and left; on
// them the hits, in the colour of the hand that strikes them, as big as they are loud; a hit glows when it plays,
// then fades. The lines of the bar's first beat, of the beats and of the off-beats stand out, to read the rhythm. On
// the left the bars and beats, on the right a LED a step (vermilion: the step playing); under the arcs the fields'
// names; above, the editing buttons and the left-hander's switch (the roll mirrored). The wheel scrolls through the
// bars; while playing the view follows the playhead. What moves is refreshed at the screen's rhythm (VBlankAttachment).
//
// Drawing (Anthony, 26/11/2026): a click on a field puts a hit there, a drag up or down sets how loud; a drag on a hit
// makes the selection louder or softer; Ctrl + drag from a hit moves the selection, Ctrl + drag from elsewhere draws a
// selection rectangle, Ctrl + click on a hit adds it to the selection or takes it out; a double-click removes a hit;
// a right click chooses the hand. A second hit on a field at a step is a flam; a third hit at a step is refused (two
// hands). A hit drawn while the transport is still is heard. The usual shortcuts: Ctrl + A, C, X, V, D (clone), Z,
// Shift + Z or Y, Delete, Escape.
#pragma once

#include <functional>
#include <vector>

#include "../../core/pattern_history.hpp"
#include "../../core/roll_geometry.hpp"
#include "../HandPanProcessor.h"
#include "../JuceHeader.h"
#include "../widgets/RollToolbar.h"

class SequencerView : public juce::Component
{
public:
    explicit SequencerView(HandPanProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // at every frame (VBlankAttachment): follows the pattern, the playhead, the glows
    void refresh();
    // scrolls `steps` up (later steps) or down, a step at least, within the pattern (the wheel's way)
    void scroll_by(double steps);

    // the editing commands (buttons and shortcuts)
    void select_all();
    void copy();
    void cut();
    void paste(); // where the roll was last clicked
    void clone(); // the selection again, right after itself
    void clear(); // the selection, or the whole pattern when nothing is selected
    void remove_selection();
    void undo();
    void redo();

    // for tests: the step at the bottom line, a hit's light (0 dark, 1 just struck), the selection, where a field's
    // circle or a hit is in the view
    double bottom_step() const { return bottom_; }
    float glow(size_t hit) const { return hit < glow_.size() ? glow_[hit] : 0.0f; }
    const std::vector<int>& selection() const { return selection_; }
    juce::Point<float> field_point(int step, int field) const;
    juce::Point<float> hit_point(int hit) const;

private:
    enum class Drag
    {
        none,
        size,
        move,
        rectangle
    };

    void reload(); // the pattern or the hands changed
    void scroll_to(double bottom);
    int playing_step() const; // -1: none
    grmh::RollGeometry geometry() const;
    void light_step(int step); // the hits of a step that starts playing
    bool fade(double seconds); // true while something still glows
    float rhythm_mark(int step) const;

    grmh::RollPoint roll_point(juce::Point<float> view) const; // in the roll's area
    int hit_under(grmh::RollPoint point) const;                // -1: none
    bool field_under(grmh::RollPoint point, int& step, int& field) const;
    int column_of(int field) const;
    bool is_selected(int hit) const;
    void select_only(int hit);
    void toggle(int hit);
    void apply(const grmh::Pattern& pattern);                        // to the processor, no history
    void commit(const std::function<void(grmh::Pattern&)>& change); // a change of its own, undoable
    void hear(const grmh::PatternHit& hit);
    void show_hand_menu();
    void add_hit(grmh::RollPoint point, int step, int field);
    void drag_move(grmh::RollPoint point);

    void paint_gutters(juce::Graphics& g, const grmh::RollGeometry& geometry, grmh::StepRange range) const;
    void paint_fields(juce::Graphics& g, const grmh::RollGeometry& geometry, grmh::StepRange range) const;
    void paint_hits(juce::Graphics& g, const grmh::RollGeometry& geometry, grmh::StepRange range) const;
    void paint_names(juce::Graphics& g, const grmh::RollGeometry& geometry) const;

    HandPanProcessor& processor_;
    RollToolbar toolbar_;
    juce::ToggleButton left_handed_;
    juce::Rectangle<int> roll_area_, left_gutter_, right_gutter_, names_area_;

    int shown_version_ = -1;
    std::vector<int> columns_;          // each field's column
    std::vector<grmh::Hand> hit_hands_; // each hit's hand
    double bottom_ = 0.0;               // the step at the bottom line
    int shown_step_ = -1;               // the step playing, as last drawn
    std::vector<float> glow_;           // each hit's light
    double last_frame_ms_ = 0.0;

    std::vector<int> selection_; // the selected hits' indices
    std::vector<grmh::PatternHit> clipboard_;
    int paste_step_ = 0; // the step last clicked
    grmh::PatternHistory history_;
    Drag drag_ = Drag::none;
    grmh::Pattern before_gesture_, drag_base_; // the pattern before the gesture (undo), the one it computes from
    int anchor_ = -1;                          // the hit the gesture began on
    float down_y_ = 0.0f;
    juce::Rectangle<float> rectangle_; // the selection rectangle, in the roll's area

    juce::VBlankAttachment vblank_{this, [this] { refresh(); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerView)
};
