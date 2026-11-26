// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The roll's buttons, in a row: Select (all), Copy, Paste, Clone, Clear, and Export (the pattern to a WAV file). They
// never take the keyboard from the roll (its shortcuts keep working after a click).
#pragma once

#include <array>
#include <functional>

#include "../JuceHeader.h"

class RollToolbar : public juce::Component
{
public:
    enum Command
    {
        select_all,
        copy,
        paste,
        clone,
        clear,
        export_audio,
        count
    };

    RollToolbar();

    std::function<void(Command)> on_command;

    // the width the buttons need
    int preferred_width() const;

    void resized() override;

private:
    std::array<juce::TextButton, count> buttons_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RollToolbar)
};
