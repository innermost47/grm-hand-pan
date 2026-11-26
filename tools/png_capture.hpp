// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A component drawn into a PNG file, for Anthony's eye (shared by the capture tools).
#pragma once

#include <cstdio>

#include "../plugin/JuceHeader.h"

namespace grmh::tools
{
constexpr float CAPTURE_SCALE = 2.0f; // a sharp image on a high-density screen

// the PNG at `path` (relative to the working directory, `fallback` when empty); the program's exit code
inline int capture_png(juce::Component& component, const char* path, const char* fallback)
{
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(path != nullptr ? path : fallback);
    out.getParentDirectory().createDirectory();
    const juce::Image image = component.createComponentSnapshot(component.getLocalBounds(), true, CAPTURE_SCALE);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    if (!stream.openedOk() || !juce::PNGImageFormat().writeImageToStream(image, stream))
        return 1;
    std::printf("%s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
} // namespace grmh::tools
