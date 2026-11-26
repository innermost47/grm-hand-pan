// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "HandPanEditor.h"

#include "../constants/sizes.hpp"
#include "../constants/theme.hpp"

using namespace grmh::ui;

HandPanEditor::HandPanEditor(HandPanProcessor& processor)
    : juce::AudioProcessorEditor(processor), top_bar_(processor), roll_(processor), bank_(processor), song_(processor)
{
    setLookAndFeel(&look_);
    for (juce::Component* c : {(juce::Component*)&top_bar_, (juce::Component*)&patterns_,
                               (juce::Component*)&sequencer_, (juce::Component*)&arranger_})
        addAndMakeVisible(c);
    sequencer_.addAndMakeVisible(roll_);
    patterns_.addAndMakeVisible(bank_);
    arranger_.addAndMakeVisible(song_);
    setSize(DIMENSIONS.editor_width, DIMENSIONS.editor_height);
}

HandPanEditor::~HandPanEditor() { setLookAndFeel(nullptr); }

void HandPanEditor::paint(juce::Graphics& g) { g.fillAll(HandPanLookAndFeel::colour(COLORS.window)); }

void HandPanEditor::resized()
{
    auto area = getLocalBounds();
    top_bar_.setBounds(area.removeFromTop(DIMENSIONS.top_bar_height));
    area.reduce(SPACING.xl, SPACING.xl);
    patterns_.setBounds(area.removeFromLeft(DIMENSIONS.side_column));
    area.removeFromLeft(SPACING.xl);
    arranger_.setBounds(area.removeFromRight(DIMENSIONS.side_column));
    area.removeFromRight(SPACING.xl);
    sequencer_.setBounds(area);
    roll_.setBounds(sequencer_.content_area());
    bank_.setBounds(patterns_.content_area());
    song_.setBounds(arranger_.content_area());
}
