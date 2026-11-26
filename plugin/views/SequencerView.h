// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll (design/README.md): one line a step, read from the bottom (played first) up, a bar on screen; on
// each line the instrument's fields in an arc, the ding in the middle, the others in satellite right and left; on
// them the hits, in the colour of the hand that strikes them, as big as they are loud; a hit glows when it plays,
// then fades. The lines of the bar's first beat, of the beats and of the off-beats stand out, to read the rhythm. On
// the left the bars and beats, on the right a LED a step (vermilion: the step playing); under the arcs the fields'
// names. The wheel scrolls
// through the bars; while playing the view follows the playhead. A switch mirrors the roll for a left-hander. What
// moves is refreshed at the screen's rhythm (VBlankAttachment).
#pragma once

#include <vector>

#include "../../core/roll_geometry.hpp"
#include "../HandPanProcessor.h"
#include "../JuceHeader.h"

class SequencerView : public juce::Component
{
public:
    explicit SequencerView(HandPanProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // drawing the hits: a click on a field puts a hit there (a drag up or down sizes it); a click on a hit selects it,
    // a drag moves it on its field, Shift + drag sizes it; a double-click or Delete removes it; a right click chooses
    // its hand. A second hit on a field at a step is a flam; a third hit at a step is refused (two hands). A hit drawn
    // while the transport is still is heard.
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // scrolls `steps` up (later steps) or down, a step at least, within the pattern (the wheel's way)
    void scroll_by(double steps);
    // the step at the bottom line (for tests)
    double bottom_step() const { return bottom_; }
    // a hit's light, 0 (dark) to 1 (just struck) (for tests)
    float glow(size_t hit) const { return hit < glow_.size() ? glow_[hit] : 0.0f; }

    // at every frame (VBlankAttachment): follows the pattern, the playhead, the glows
    void refresh();

private:
    void reload();  // the pattern or the hands changed
    void scroll_to(double bottom);
    int playing_step() const; // -1: none
    grmh::RollGeometry geometry() const;

    void paint_gutters(juce::Graphics& g, const grmh::RollGeometry& geometry, grmh::StepRange range) const;
    void paint_fields(juce::Graphics& g, const grmh::RollGeometry& geometry, grmh::StepRange range) const;
    void paint_hits(juce::Graphics& g, const grmh::RollGeometry& geometry, grmh::StepRange range) const;
    void paint_names(juce::Graphics& g, const grmh::RollGeometry& geometry) const;

    HandPanProcessor& processor_;
    juce::ToggleButton left_handed_;
    juce::Rectangle<int> roll_area_, left_gutter_, right_gutter_, names_area_;

    int shown_version_ = -1;
    std::vector<int> columns_;          // each field's column
    std::vector<grmh::Hand> hit_hands_; // each hit's hand
    double bottom_ = 0.0;               // the step at the bottom line
    int shown_step_ = -1;               // the step playing, as last drawn
    std::vector<float> glow_;           // each hit's light
    double last_frame_ms_ = 0.0;

    enum class Drag
    {
        none,
        size,
        move
    };
    int selected_ = -1; // the hit selected, -1: none
    Drag drag_ = Drag::none;
    double drag_start_y_ = 0.0, drag_start_size_ = 0.0;

    grmh::RollPoint roll_point(const juce::MouseEvent& event) const; // in the roll's area
    int hit_under(grmh::RollPoint point) const;                       // -1: none
    bool field_under(grmh::RollPoint point, int& step, int& field) const;
    int column_of(int field) const;
    void edit(const std::function<void(grmh::Pattern&)>& change);
    void hear(const grmh::PatternHit& hit);
    void show_hand_menu(int hit);

    void light_step(int step); // the hits of a step that starts playing
    bool fade(double seconds); // true while something still glows
    float rhythm_mark(int step) const;

    juce::VBlankAttachment vblank_{this, [this] { refresh(); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerView)
};
