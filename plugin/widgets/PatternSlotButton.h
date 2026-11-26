// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// One pattern of a bank: its number and its name, a dot when it holds hits, a sakura border when it is the one being
// edited and played. A click chooses it, a right click asks for its menu.
#pragma once

#include <functional>

#include "../JuceHeader.h"

class PatternSlotButton : public juce::Component
{
public:
    void show(int number, const juce::String& name, bool used, bool current);

    std::function<void()> on_select, on_menu;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }

    const juce::String& name() const { return name_; } // for tests

private:
    int number_ = 0;
    juce::String name_;
    bool used_ = false, current_ = false;
};
