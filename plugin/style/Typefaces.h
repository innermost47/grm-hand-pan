// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The interface's typefaces, embedded (assets/fonts, SIL Open Font License, reduced to the Latin glyphs): Zen Kaku
// Gothic New for the interface, light, regular and medium; Shippori Mincho for the plugin's name.
#pragma once

#include "../JuceHeader.h"

class Typefaces
{
public:
    enum class Weight
    {
        light,
        regular,
        medium
    };

    static const Typefaces& get(); // loaded once

    juce::Typeface::Ptr gothic(Weight weight) const;
    juce::Typeface::Ptr mincho() const { return mincho_; }

    // a font of the interface, `height` points
    juce::Font interface(float height, Weight weight = Weight::regular) const;
    // the plugin's name, `height` points
    juce::Font title(float height) const;

private:
    Typefaces();

    juce::Typeface::Ptr light_, regular_, medium_, mincho_;
};
