// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "Typefaces.h"

#include <BinaryData.h>

namespace
{
juce::Typeface::Ptr load(const char* data, int size) { return juce::Typeface::createSystemTypefaceFor(data, (size_t)size); }
} // namespace

Typefaces::Typefaces()
    : light_(load(BinaryData::zenkakugothiclight_ttf, BinaryData::zenkakugothiclight_ttfSize)),
      regular_(load(BinaryData::zenkakugothicregular_ttf, BinaryData::zenkakugothicregular_ttfSize)),
      medium_(load(BinaryData::zenkakugothicmedium_ttf, BinaryData::zenkakugothicmedium_ttfSize)),
      mincho_(load(BinaryData::shipporiminchoregular_ttf, BinaryData::shipporiminchoregular_ttfSize))
{
}

const Typefaces& Typefaces::get()
{
    static const Typefaces typefaces;
    return typefaces;
}

juce::Typeface::Ptr Typefaces::gothic(Weight weight) const
{
    switch (weight)
    {
    case Weight::light:
        return light_;
    case Weight::medium:
        return medium_;
    case Weight::regular:
        break;
    }
    return regular_;
}

juce::Font Typefaces::interface(float height, Weight weight) const
{
    return juce::Font(juce::FontOptions(gothic(weight)).withHeight(height));
}

juce::Font Typefaces::title(float height) const { return juce::Font(juce::FontOptions(mincho_).withHeight(height)); }
