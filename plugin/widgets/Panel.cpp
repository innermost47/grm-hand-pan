// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "Panel.h"

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../style/HandPanLookAndFeel.h"

using namespace grmh::ui;

Panel::Panel(const juce::String& title) : title_(title) {}

juce::Rectangle<int> Panel::content_area() const
{
    return getLocalBounds().reduced(SPACING.lg).withTrimmedTop(DIMENSIONS.panel_title);
}

void Panel::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced(BORDERS.hairline * 0.5f);
    g.setColour(HandPanLookAndFeel::colour(COLORS.panel));
    g.fillRoundedRectangle(area, BORDERS.corner);
    g.setColour(HandPanLookAndFeel::colour(COLORS.line));
    g.drawRoundedRectangle(area, BORDERS.corner, BORDERS.hairline);

    g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
    g.setFont(HandPanLookAndFeel::heading_font(TYPOGRAPHY.small));
    g.drawText(title_.toUpperCase(), getLocalBounds().reduced(SPACING.lg, 0).withHeight(DIMENSIONS.panel_title + SPACING.lg),
               juce::Justification::centred, false);
}
