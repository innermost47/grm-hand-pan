// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A transport button drawn as its sign: play (a triangle), pause (two bars), stop (a square). Lit in the accent colour
// while its state holds (playing, paused).
#pragma once

#include "../JuceHeader.h"

class TransportButton : public juce::Button
{
public:
    enum class Sign
    {
        play,
        pause,
        stop
    };

    TransportButton(Sign sign, const juce::String& tooltip);

    void set_lit(bool lit);

protected:
    void paintButton(juce::Graphics& g, bool hovered, bool down) override;

private:
    juce::Path sign_path(juce::Rectangle<float> area) const;

    Sign sign_;
    bool lit_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportButton)
};
