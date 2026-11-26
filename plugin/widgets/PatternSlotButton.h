// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// One pattern of a bank: its number and its name, a dot when it holds hits, a sakura border when it is the one being
// edited, a vermilion one while a key plays it. A click chooses it, a right click asks for its menu.
#pragma once

#include <functional>

#include "../JuceHeader.h"

class PatternSlotButton : public juce::Component
{
public:
    void show(int number, const juce::String& name, bool used, bool current, bool playing);
    bool playing() const { return playing_; } // for tests

    std::function<void()> on_select, on_menu, on_drag; // on_drag: dragged out (to the arranger)

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }

    const juce::String& name() const { return name_; } // for tests

private:
    int number_ = 0;
    juce::String name_;
    bool used_ = false, current_ = false, playing_ = false;
};
