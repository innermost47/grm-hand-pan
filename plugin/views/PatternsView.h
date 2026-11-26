// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The patterns (design/README.md, the zone on the left): the sixteen patterns of a bank in two columns, the banks
// A B C D under them, and what the MIDI keys play (notes, or patterns from C1: the one a key plays shows in
// vermilion). A click on a pattern edits it; a right click renames, copies, pastes over or clears it.
// Refreshed at the screen's rhythm (VBlankAttachment).
#pragma once

#include <array>
#include <optional>

#include "../../core/pattern_library.hpp"
#include "../HandPanProcessor.h"
#include "../JuceHeader.h"
#include "../widgets/PatternSlotButton.h"

class PatternsView : public juce::Component
{
public:
    explicit PatternsView(HandPanProcessor& processor);

    void resized() override;

    // the bank shown (0 is A), and a click on one of its patterns (for tests)
    void show_bank(int bank);
    int shown_bank() const { return bank_; }
    void choose(int slot);
    const PatternSlotButton& slot_button(int slot) const { return slots_[(size_t)slot]; }

    // every frame (VBlankAttachment)
    void refresh();

private:
    void show_menu(int slot);
    void rename(int slot);

    HandPanProcessor& processor_;
    std::array<PatternSlotButton, grmh::PATTERNS_PER_BANK> slots_;
    std::array<juce::TextButton, grmh::BANKS> banks_;
    juce::ComboBox midi_mode_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> midi_mode_attachment_;
    int bank_ = 0;
    std::optional<grmh::PatternSlot> clipboard_;
    std::unique_ptr<juce::AlertWindow> naming_;

    juce::VBlankAttachment vblank_{this, [this] { refresh(); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternsView)
};
