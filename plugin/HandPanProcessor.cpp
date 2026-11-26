// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "HandPanProcessor.h"

#include "../constants/audio.hpp"
#include "../constants/plugin.hpp"

using namespace grmh;

HandPanProcessor::HandPanProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "GRMHandPan", create_layout()), solver_(parameters_, models_)
{
}

HandPanProcessor::~HandPanProcessor() { solver_.stopThread(plugin::SOLVER_STOP_TIMEOUT_MS); }

juce::AudioProcessorValueTreeState::ParameterLayout HandPanProcessor::create_layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{plugin::PARAM_ON_LAP, 1},
                                                          plugin::PARAM_ON_LAP_NAME, false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{plugin::PARAM_STEREO_WIDTH, 1}, plugin::PARAM_STEREO_WIDTH_NAME,
        juce::NormalisableRange<float>(0.0f, 1.0f, (float)plugin::STEREO_WIDTH_STEP), (float)STEREO_WIDTH_DEFAULT));
    return layout;
}

void HandPanProcessor::prepareToPlay(double sample_rate, int)
{
    solver_.set_sample_rate(sample_rate);
    if (!solver_.isThreadRunning())
        solver_.startThread(juce::Thread::Priority::low);
}

bool HandPanProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

double HandPanProcessor::getTailLengthSeconds() const { return plugin::TAIL_SECONDS; }

void HandPanProcessor::render(juce::AudioBuffer<float>& buffer, int start, int count)
{
    if (count <= 0)
        return;
    if (buffer.getNumChannels() >= 2)
        engine_.render_stereo(buffer.getWritePointer(0, start), buffer.getWritePointer(1, start), count);
    else
        engine_.render(buffer.getWritePointer(0, start), count);
}

void HandPanProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals no_denormals;
    // a newer model from the solver, taken between two blocks; the fields keep ringing through it
    const SoundModel* fresh = models_.take();
    if (fresh != playing_)
    {
        playing_ = fresh;
        engine_.set_model(playing_);
    }

    int position = 0;
    for (const auto event : midi)
    {
        const auto message = event.getMessage();
        if (!message.isNoteOn())
            continue;
        const int at = juce::jlimit(0, buffer.getNumSamples(), event.samplePosition);
        render(buffer, position, at - position);
        position = at;
        engine_.strike(message.getNoteNumber(), message.getVelocity());
    }
    render(buffer, position, buffer.getNumSamples() - position);
    for (int c = 2; c < buffer.getNumChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* HandPanProcessor::createEditor() { return new juce::GenericAudioProcessorEditor(*this); }

void HandPanProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if (const auto xml = parameters_.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void HandPanProcessor::setStateInformation(const void* data, int size)
{
    if (const auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters_.state.getType()))
            parameters_.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HandPanProcessor(); }
