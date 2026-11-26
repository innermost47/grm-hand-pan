// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "PatternSlotButton.h"

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../style/HandPanLookAndFeel.h"
#include "../style/Typefaces.h"

using namespace grmh::ui;

void PatternSlotButton::show(int number, const juce::String& name, bool used, bool current)
{
    if (number == number_ && name == name_ && used == used_ && current == current_)
        return;
    number_ = number;
    name_ = name;
    used_ = used;
    current_ = current;
    repaint();
}

void PatternSlotButton::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced(BORDERS.hairline * 0.5f);
    g.setColour(HandPanLookAndFeel::colour(COLORS.raised));
    g.fillRoundedRectangle(area, BORDERS.corner);
    if (isMouseOver())
    {
        g.setColour(HandPanLookAndFeel::colour(COLORS.text).withAlpha(OPACITY.hover));
        g.fillRoundedRectangle(area, BORDERS.corner);
    }
    g.setColour(HandPanLookAndFeel::colour(current_ ? COLORS.accent : COLORS.line));
    g.drawRoundedRectangle(area, BORDERS.corner, current_ ? BORDERS.ring : BORDERS.hairline);

    auto text = getLocalBounds().reduced(SPACING.md, 0);
    g.setColour(HandPanLookAndFeel::colour(COLORS.muted));
    g.setFont(Typefaces::get().interface(TYPOGRAPHY.caption, Typefaces::Weight::light));
    g.drawText(juce::String(number_), text.removeFromLeft(DIMENSIONS.slot_number), juce::Justification::centredLeft);
    if (used_)
    {
        const float dot = 5.0f;
        g.setColour(HandPanLookAndFeel::colour(COLORS.accent));
        g.fillEllipse(juce::Rectangle<float>(dot, dot).withCentre(
            {(float)text.getRight() - dot, (float)text.getCentreY()}));
        text.removeFromRight(SPACING.lg);
    }
    g.setColour(HandPanLookAndFeel::colour(current_ ? COLORS.accent : (used_ ? COLORS.text : COLORS.text_dim)));
    g.setFont(Typefaces::get().interface(TYPOGRAPHY.label));
    g.drawText(name_, text, juce::Justification::centredLeft, true);
}

void PatternSlotButton::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        if (on_menu)
            on_menu();
    }
    else if (on_select)
        on_select();
}
