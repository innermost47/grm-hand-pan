// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The editor as a host opens it: its size, its widgets tied to the parameters (a knob moved is the parameter moved,
// the parameter moved is the knob moved), the signature chosen is the pattern's, the transport's buttons drive the
// plugin's clock.
#include <cstdio>

#include "../constants/plugin.hpp"
#include "../constants/sizes.hpp"
#include "../plugin/HandPanEditor.h"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
template <class T>
T* find_child(juce::Component& parent, const std::function<bool(T&)>& matches)
{
    for (auto* child : parent.getChildren())
    {
        if (auto* t = dynamic_cast<T*>(child); t != nullptr && matches(*t))
            return t;
        if (auto* deeper = find_child<T>(*child, matches))
            return deeper;
    }
    return nullptr;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    test::TestReport report("editor");
    HandPanProcessor processor;
    processor.setPlayConfigDetails(0, 2, 48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    report.expect(editor != nullptr && editor->getWidth() == ui::DIMENSIONS.editor_width &&
                      editor->getHeight() == ui::DIMENSIONS.editor_height,
                  "the editor opens at its size");

    auto* volume = find_child<juce::Slider>(*editor, [](juce::Slider& s) { return s.getName() == "V"; });
    auto* width = find_child<juce::Slider>(*editor, [](juce::Slider& s) { return s.getName() == "W"; });
    report.expect(volume != nullptr && width != nullptr, "the V and W knobs are there");
    if (volume != nullptr && width != nullptr)
    {
        volume->setValue(-12.0, juce::sendNotificationSync);
        const float volume_db = processor.parameters().getRawParameterValue(plugin::PARAM_VOLUME)->load();
        report.expect(std::fabs(volume_db + 12.0f) < 0.05f, "V moved is the volume moved");
        // on the message thread, an attachment follows its parameter at once
        processor.parameters().getParameter(plugin::PARAM_STEREO_WIDTH)->setValueNotifyingHost(0.25f);
        report.expect(std::fabs(width->getValue() - 0.25) < 0.011, "the width moved by the host is W moved");
    }

    auto* lap = find_child<juce::ToggleButton>(*editor, [](juce::ToggleButton& b)
                                               { return b.getButtonText() == plugin::PARAM_ON_LAP_NAME; });
    report.expect(lap != nullptr && lap->getToggleState(), "the switch 'On the lap', on by default");

    auto* signature = find_child<juce::ComboBox>(*editor, [](juce::ComboBox& c) { return c.getText() == "4/4"; });
    report.expect(signature != nullptr, "the signature shows the pattern's: 4/4");
    if (signature != nullptr)
    {
        signature->setText("6/8", juce::sendNotificationSync);
        const Pattern& p = processor.pattern();
        report.expect(p.signature.numerator == 6 && p.signature.denominator == 8 && p.steps_per_bar() == 12,
                      "6/8 chosen: the pattern is in 6/8, twelve sixteenths a bar");
    }

    auto* play = find_child<TransportButton>(*editor, [](TransportButton& b) { return b.getName() == "Play"; });
    report.expect(play != nullptr, "the Play button is there");
    if (play != nullptr)
    {
        play->onClick(); // what a click runs (a click itself is delivered later, by the message loop)
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        processor.prepareToPlay(48000.0, 512);
        processor.processBlock(buffer, midi);
        processor.processBlock(buffer, midi);
        report.expect(processor.transport().shown_playing() && processor.transport().shown_beat() > 0.0,
                      "Play clicked: the plugin's clock runs");
    }
    editor.reset();
    return report.finish();
}
