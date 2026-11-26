// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin's window (design/maquette.png): the top bar, then the patterns on the left, the sequencer in the middle,
// the arranger on the right. Every widget draws through the plugin's look and feel.
#pragma once

#include "HandPanProcessor.h"
#include "JuceHeader.h"
#include "style/HandPanLookAndFeel.h"
#include "views/ArrangerView.h"
#include "views/PatternsView.h"
#include "views/SequencerView.h"
#include "views/TopBar.h"
#include "widgets/Panel.h"

class HandPanEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer // patterns to the arranger
{
public:
    explicit HandPanEditor(HandPanProcessor& processor);
    ~HandPanEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    // Space plays or stops the plugin's own clock, wherever the keyboard is (the views pass on what they do not use)
    bool keyPressed(const juce::KeyPress& key) override;

    TopBar& top_bar() { return top_bar_; }             // for tests
    SequencerView& sequencer_view() { return roll_; }   // for tests
    PatternsView& patterns_view() { return bank_; }     // for tests
    ArrangerView& arranger_view() { return song_; }     // for tests

private:
    HandPanLookAndFeel look_; // declared first: outlives the widgets that draw through it
    TopBar top_bar_;
    Panel patterns_{"Patterns"}, sequencer_{"Sequencer"}, arranger_{"Arranger"};
    SequencerView roll_;
    PatternsView bank_;
    ArrangerView song_;
    juce::TooltipWindow tooltips_{this};
    HandPanProcessor& processor_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HandPanEditor)
};
