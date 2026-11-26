// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A zone of the editor: a sumi panel with its title in spaced capitals; what it holds sits under the title.
#pragma once

#include "../JuceHeader.h"

class Panel : public juce::Component
{
public:
    explicit Panel(const juce::String& title);

    // where the panel's content goes
    juce::Rectangle<int> content_area() const;

    void paint(juce::Graphics& g) override;

private:
    juce::String title_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Panel)
};
