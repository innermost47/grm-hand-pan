// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin's editor as a host opens it, saved as a PNG for Anthony to look at. Usage: render_editor [out.png]
#include "../plugin/HandPanProcessor.h"
#include "png_capture.hpp"

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    HandPanProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    return grmh::tools::capture_png(*editor, argc > 1 ? argv[1] : nullptr, "renders/editor.png");
}
