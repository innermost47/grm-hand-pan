// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// One block of the arrangement: its variation (the bank's letter: a click turns to the next one), its pattern's name
// and place (A2), how many times it plays (− / +), and × to remove it; a silence shows its bars. Sakura when its
// pattern is the one being edited, vermilion while it plays. A click elsewhere edits its pattern, a drag moves it.
#pragma once

#include <functional>

#include "../JuceHeader.h"

class ArrangementRow : public juce::Component
{
public:
    // `letter` empty: a silence (no variation)
    void show(const juce::String& letter, const juce::String& name, const juce::String& count, bool edited,
              bool playing);

    std::function<void()> on_variation, on_less, on_more, on_remove, on_select;
    std::function<void(const juce::MouseEvent&)> on_drag, on_drop;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    const juce::String& name() const { return name_; } // for tests
    bool playing() const { return playing_; }          // for tests

private:
    enum class Part
    {
        variation,
        less,
        more,
        remove,
        body
    };
    juce::Rectangle<int> area(Part part) const;
    Part part_at(juce::Point<int> point) const;

    juce::String letter_, name_, count_;
    bool edited_ = false, playing_ = false;
    Part pressed_ = Part::body;
    bool dragged_ = false;
};
