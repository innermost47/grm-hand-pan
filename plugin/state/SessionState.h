// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What a session keeps besides the parameters: every pattern of the banks (the ones the player touched), the one being
// edited, and the player's hands, as children of the plugin's state (constants/state.hpp names them).
#pragma once

#include "../../core/pattern_library.hpp"
#include "../JuceHeader.h"

class SessionState
{
public:
    // the library and the hands added to `state`
    static void write(juce::ValueTree& state, const grmh::PatternLibrary& library, const grmh::PlayerHands& hands);

    // the library and the hands read from `state` (left as they are when it holds none), and taken out of it
    static void read(juce::ValueTree& state, grmh::PatternLibrary& library, grmh::PlayerHands& hands);

private:
    static juce::var exact(double value);
    static double exact(const juce::var& value, double fallback);
    static juce::ValueTree write_slot(grmh::SlotId id, const grmh::PatternSlot& slot);
    static grmh::PatternSlot read_slot(const juce::ValueTree& tree);
};
