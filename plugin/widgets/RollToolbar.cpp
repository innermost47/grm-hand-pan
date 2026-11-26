// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "RollToolbar.h"

#include "../../constants/sizes.hpp"

using namespace grmh::ui;

RollToolbar::RollToolbar()
{
    const char* names[count] = {"Select", "Copy", "Paste", "Clone", "Clear", "Export"};
    const char* tips[count] = {"Select every hit (Ctrl+A)", "Copy the selection (Ctrl+C)",
                               "Paste where the roll was last clicked (Ctrl+V)",
                               "The selection again, right after itself (Ctrl+D)",
                               "Remove the selection, or the whole pattern when nothing is selected",
                               "Write the pattern to a WAV file"};
    for (int i = 0; i < count; ++i)
    {
        auto& b = buttons_[(size_t)i];
        b.setButtonText(names[i]);
        b.setTooltip(tips[i]);
        b.setWantsKeyboardFocus(false);
        b.setMouseClickGrabsKeyboardFocus(false);
        b.onClick = [this, i]
        {
            if (on_command)
                on_command((Command)i);
        };
        addAndMakeVisible(b);
    }
}

int RollToolbar::preferred_width() const { return count * DIMENSIONS.toolbar_button + (count - 1) * SPACING.sm; }

// the buttons share the width they are given, never wider than toolbar_button
void RollToolbar::resized()
{
    auto area = getLocalBounds();
    const int width = std::min(DIMENSIONS.toolbar_button, (area.getWidth() - (count - 1) * SPACING.sm) / count);
    for (auto& b : buttons_)
    {
        b.setBounds(area.removeFromLeft(width).withSizeKeepingCentre(width, DIMENSIONS.button_height));
        area.removeFromLeft(SPACING.sm);
    }
}
