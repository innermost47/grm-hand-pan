// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "ArrangementRow.h"

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../style/HandPanLookAndFeel.h"
#include "../style/Typefaces.h"

using namespace grmh::ui;

namespace
{
juce::Colour colour(std::uint32_t argb) { return HandPanLookAndFeel::colour(argb); }
} // namespace

void ArrangementRow::show(const juce::String& letter, const juce::String& name, const juce::String& count, bool edited,
                          bool playing, bool selected)
{
    if (letter == letter_ && name == name_ && count == count_ && edited == edited_ && playing == playing_ &&
        selected == selected_)
        return;
    letter_ = letter;
    name_ = name;
    count_ = count;
    edited_ = edited;
    playing_ = playing;
    selected_ = selected;
    repaint();
}

juce::Rectangle<int> ArrangementRow::area(Part part) const
{
    auto r = getLocalBounds().reduced(SPACING.sm);
    const int square = r.getHeight();
    const auto variation = r.removeFromLeft(square);
    const auto remove = r.removeFromRight(square);
    r.removeFromRight(SPACING.sm);
    const auto more = r.removeFromRight(square);
    r.removeFromRight(DIMENSIONS.block_count);
    const auto less = r.removeFromRight(square);
    switch (part)
    {
    case Part::variation:
        return variation;
    case Part::less:
        return less;
    case Part::more:
        return more;
    case Part::remove:
        return remove;
    case Part::body:
        break;
    }
    return getLocalBounds();
}

ArrangementRow::Part ArrangementRow::part_at(juce::Point<int> point) const
{
    for (Part p : {Part::variation, Part::less, Part::more, Part::remove})
        if (area(p).contains(point) && (p != Part::variation || letter_.isNotEmpty()))
            return p;
    return Part::body;
}

void ArrangementRow::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(BORDERS.hairline * 0.5f);
    g.setColour(colour(COLORS.raised));
    g.fillRoundedRectangle(bounds, BORDERS.corner);
    g.setColour(colour(playing_ ? COLORS.song_playing : (edited_ ? COLORS.accent : COLORS.line)));
    g.drawRoundedRectangle(bounds, BORDERS.corner, playing_ || edited_ ? BORDERS.ring : BORDERS.hairline);
    if (selected_)
    {
        g.setColour(colour(COLORS.accent));
        const float inset = (float)SPACING.sm;
        g.fillRoundedRectangle(bounds.withWidth(BORDERS.corner).withTrimmedTop(inset).withTrimmedBottom(inset),
                               BORDERS.corner * 0.5f);
    }

    const Typefaces& faces = Typefaces::get();
    if (letter_.isNotEmpty())
    {
        const auto box = area(Part::variation).toFloat();
        g.setColour(colour(COLORS.line));
        g.drawRoundedRectangle(box, BORDERS.corner, BORDERS.hairline);
        g.setColour(colour(COLORS.accent));
        g.setFont(HandPanLookAndFeel::heading_font(TYPOGRAPHY.label));
        g.drawText(letter_, box, juce::Justification::centred, false);
    }
    const auto name_area = getLocalBounds().reduced(SPACING.sm)
                               .withLeft(area(Part::variation).getRight() + SPACING.md)
                               .withRight(area(Part::less).getX() - SPACING.sm);
    g.setColour(colour(letter_.isEmpty() ? COLORS.text_dim : COLORS.text));
    g.setFont(faces.interface(TYPOGRAPHY.label));
    g.drawText(name_, name_area, juce::Justification::centredLeft, true);

    g.setColour(colour(COLORS.text_dim));
    g.setFont(faces.interface(TYPOGRAPHY.label, Typefaces::Weight::medium));
    g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x88\x92")), area(Part::less), juce::Justification::centred);
    g.drawText("+", area(Part::more), juce::Justification::centred);
    g.drawText(juce::String(juce::CharPointer_UTF8("\xc3\x97")), area(Part::remove), juce::Justification::centred);
    g.setColour(colour(COLORS.text));
    g.setFont(faces.interface(TYPOGRAPHY.small));
    g.drawText(count_, area(Part::less).withX(area(Part::less).getRight()).withWidth(DIMENSIONS.block_count),
               juce::Justification::centred);
}

void ArrangementRow::mouseDown(const juce::MouseEvent& event)
{
    pressed_ = part_at(event.getPosition());
    dragged_ = false;
    extend_ = event.mods.isShiftDown();
}

void ArrangementRow::mouseDrag(const juce::MouseEvent& event)
{
    if (pressed_ != Part::body || event.getDistanceFromDragStart() < SPACING.sm)
        return;
    dragged_ = true;
    if (on_drag)
        on_drag(event);
}

void ArrangementRow::mouseUp(const juce::MouseEvent& event)
{
    if (dragged_)
    {
        if (on_drop)
            on_drop(event);
        return;
    }
    if (pressed_ == Part::body)
    {
        if (on_select)
            on_select(extend_);
        return;
    }
    const std::function<void()>* action = nullptr;
    switch (pressed_)
    {
    case Part::variation:
        action = &on_variation;
        break;
    case Part::less:
        action = &on_less;
        break;
    case Part::more:
        action = &on_more;
        break;
    case Part::remove:
        action = &on_remove;
        break;
    case Part::body:
        return;
    }
    if (*action)
        (*action)();
}
