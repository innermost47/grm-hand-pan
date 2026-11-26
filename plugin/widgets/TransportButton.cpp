// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "TransportButton.h"

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../style/HandPanLookAndFeel.h"

using namespace grmh::ui;

TransportButton::TransportButton(Sign sign, const juce::String& tooltip) : juce::Button(tooltip), sign_(sign)
{
    setTooltip(tooltip);
}

void TransportButton::set_lit(bool lit)
{
    if (lit == lit_)
        return;
    lit_ = lit;
    repaint();
}

juce::Path TransportButton::sign_path(juce::Rectangle<float> area) const
{
    juce::Path path;
    const float s = std::min(area.getWidth(), area.getHeight());
    const auto box = juce::Rectangle<float>(s, s).withCentre(area.getCentre());
    switch (sign_)
    {
    case Sign::play:
        path.addTriangle(box.getX() + s * 0.1f, box.getY(), box.getX() + s * 0.1f, box.getBottom(), box.getRight(),
                         box.getCentreY());
        break;
    case Sign::pause:
        path.addRectangle(box.getX() + s * 0.12f, box.getY(), s * 0.26f, s);
        path.addRectangle(box.getRight() - s * 0.38f, box.getY(), s * 0.26f, s);
        break;
    case Sign::stop:
        path.addRectangle(box.reduced(s * 0.06f));
        break;
    }
    return path;
}

void TransportButton::paintButton(juce::Graphics& g, bool hovered, bool down)
{
    const auto area = getLocalBounds().toFloat().reduced(BORDERS.hairline * 0.5f);
    g.setColour(HandPanLookAndFeel::colour(COLORS.raised));
    g.fillRoundedRectangle(area, BORDERS.corner);
    if (hovered || down)
    {
        g.setColour(HandPanLookAndFeel::colour(COLORS.text).withAlpha(down ? OPACITY.pressed : OPACITY.hover));
        g.fillRoundedRectangle(area, BORDERS.corner);
    }
    g.setColour(HandPanLookAndFeel::colour(lit_ ? COLORS.accent : COLORS.line));
    g.drawRoundedRectangle(area, BORDERS.corner, BORDERS.hairline);

    g.setColour(HandPanLookAndFeel::colour(lit_ ? COLORS.accent : COLORS.text));
    g.fillPath(sign_path(area.reduced(area.getHeight() * 0.32f)));
}
