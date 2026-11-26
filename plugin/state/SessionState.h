// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What a session keeps besides the parameters: every pattern of the banks (the ones the player touched), the one being
// edited, the arranger's song and the player's hands, as children of the plugin's state (constants/state.hpp names
// them). Doubles are written with all their digits: a session comes back bit for bit.
#pragma once

#include "../../core/pattern_library.hpp"
#include "../../models/arrangement.hpp"
#include "../JuceHeader.h"

class SessionState
{
public:
    // the session's content added to `state`
    static void write(juce::ValueTree& state, const grmh::PatternLibrary& library, const grmh::Arrangement& arrangement,
                      const grmh::PlayerHands& hands);

    // the session's content read from `state` (each part left as it is when the state holds none of it), and taken
    // out of it
    static void read(juce::ValueTree& state, grmh::PatternLibrary& library, grmh::Arrangement& arrangement,
                     grmh::PlayerHands& hands);

private:
    static juce::var exact(double value);
    static double exact(const juce::var& value, double fallback);
    static juce::ValueTree write_slot(grmh::SlotId id, const grmh::PatternSlot& slot);
    static grmh::PatternSlot read_slot(const juce::ValueTree& tree);
    static juce::ValueTree write_arrangement(const grmh::Arrangement& arrangement);
    static grmh::Arrangement read_arrangement(const juce::ValueTree& tree);
};
