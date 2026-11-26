// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What tells the player to wait: a sakura arc turning and a word ("Solving the instrument"), while the networks solve
// (the first calibrations, the instrument put on the lap...). Hidden the rest of the time; it turns at the screen's
// rhythm (VBlankAttachment).
#pragma once

#include "../JuceHeader.h"

class LoadingIndicator : public juce::Component
{
public:
    LoadingIndicator();

    void set_active(bool active);
    bool active() const { return active_; } // for tests

    void paint(juce::Graphics& g) override;

private:
    bool active_ = false;
    double started_ms_ = 0.0;
    juce::VBlankAttachment vblank_{this, [this] { if (active_) repaint(); }};
};
