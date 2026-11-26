// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "LoadingIndicator.h"

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../style/HandPanLookAndFeel.h"
#include "../style/Typefaces.h"

using namespace grmh::ui;

LoadingIndicator::LoadingIndicator()
{
    setInterceptsMouseClicks(false, false);
    setVisible(false);
}

void LoadingIndicator::set_active(bool active)
{
    if (active == active_)
        return;
    active_ = active;
    started_ms_ = juce::Time::getMillisecondCounterHiRes();
    setVisible(active);
}

void LoadingIndicator::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const float size = std::min(area.getHeight(), (float)DIMENSIONS.loader_size);
    const auto circle = area.removeFromLeft(size).withSizeKeepingCentre(size, size).reduced(BORDERS.ring);
    const double turns = (juce::Time::getMillisecondCounterHiRes() - started_ms_) / 1000.0 / MOTION.loader_turn_seconds;
    const float start = (float)(juce::MathConstants<double>::twoPi * (turns - std::floor(turns)));

    juce::Path track, arc;
    track.addEllipse(circle);
    g.setColour(HandPanLookAndFeel::colour(COLORS.line));
    g.strokePath(track, juce::PathStrokeType(BORDERS.ring));
    arc.addCentredArc(circle.getCentreX(), circle.getCentreY(), circle.getWidth() * 0.5f, circle.getHeight() * 0.5f, 0.0f,
                      start, start + juce::MathConstants<float>::pi * 0.6f, true);
    g.setColour(HandPanLookAndFeel::colour(COLORS.accent));
    g.strokePath(arc, juce::PathStrokeType(BORDERS.ring, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    area.removeFromLeft((float)SPACING.md);
    g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
    g.setFont(Typefaces::get().interface(TYPOGRAPHY.small, Typefaces::Weight::light));
    g.drawText("Solving the instrument", area, juce::Justification::centredLeft, true);
}
