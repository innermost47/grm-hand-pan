// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "Knob.h"

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../style/HandPanLookAndFeel.h"

using namespace grmh::ui;

Knob::Knob(const juce::String& caption) : caption_(caption)
{
    slider_.setName(caption);
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider_);
}

juce::Rectangle<int> Knob::caption_area() const
{
    return getLocalBounds().removeFromBottom(DIMENSIONS.knob_caption);
}

void Knob::resized()
{
    auto area = getLocalBounds();
    area.removeFromBottom(DIMENSIONS.knob_caption);
    const int side = std::min(area.getWidth(), area.getHeight());
    slider_.setBounds(area.withSizeKeepingCentre(side, side));
}

void Knob::paint(juce::Graphics& g)
{
    g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
    g.setFont(HandPanLookAndFeel::heading_font(TYPOGRAPHY.caption));
    g.drawText(caption_, caption_area(), juce::Justification::centred, false);
}
