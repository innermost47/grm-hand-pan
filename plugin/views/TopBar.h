// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The top of the editor (design/maquette.png): on the left the volume (V), the stereo width (W) and the instrument on
// the lap; in the middle the transport (play, pause, stop: the host's own transport when it plays), where the song is
// (bar.beat.step), the pattern's signature and the tempo (the plugin's own in the standalone, the host's otherwise);
// on the right the scale. What moves is refreshed at the screen's rhythm (VBlankAttachment).
#pragma once

#include "../HandPanProcessor.h"
#include "../JuceHeader.h"
#include "../widgets/Knob.h"
#include "../widgets/TransportButton.h"

class TopBar : public juce::Component
{
public:
    explicit TopBar(HandPanProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void refresh(); // at every frame
    void choose_signature();

    HandPanProcessor& processor_;
    Knob volume_{"V"}, width_{"W"};
    juce::ToggleButton on_lap_;
    TransportButton play_{TransportButton::Sign::play, "Play"};
    TransportButton pause_{TransportButton::Sign::pause, "Pause"};
    TransportButton stop_{TransportButton::Sign::stop, "Stop: back to the start"};
    juce::Label position_, tempo_shown_;
    juce::Slider tempo_{juce::Slider::LinearBarVertical, juce::Slider::TextBoxLeft};
    juce::ComboBox signature_, scale_;
    std::unique_ptr<SliderAttachment> volume_attachment_, width_attachment_, tempo_attachment_;
    std::unique_ptr<ButtonAttachment> on_lap_attachment_;
    juce::VBlankAttachment vblank_{this, [this] { refresh(); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};
