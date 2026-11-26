// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A rotary knob with its name under it (V, W): the look and feel draws the knob, the knob writes its name.
#pragma once

#include "../JuceHeader.h"

class Knob : public juce::Component
{
public:
    explicit Knob(const juce::String& caption);

    juce::Slider& slider() { return slider_; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Rectangle<int> caption_area() const;

    juce::Slider slider_;
    juce::String caption_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};
