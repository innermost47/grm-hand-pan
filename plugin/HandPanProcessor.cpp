// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "HandPanProcessor.h"

#include "../constants/audio.hpp"
#include "../constants/plugin.hpp"
#include "../constants/scales.hpp"
#include "../core/pattern_compiler.hpp"
#include "HandPanEditor.h"

using namespace grmh;

namespace
{
float volume_gain(float decibels) { return juce::Decibels::decibelsToGain(decibels, plugin::VOLUME_MIN_DB); }
} // namespace

HandPanProcessor::HandPanProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "GRMHandPan", create_layout()), solver_(parameters_, models_),
      notes_(D_KURD.begin(), D_KURD.end())
{
    compile_pattern();
}

HandPanProcessor::~HandPanProcessor() { solver_.stopThread(plugin::SOLVER_STOP_TIMEOUT_MS); }

juce::AudioProcessorValueTreeState::ParameterLayout HandPanProcessor::create_layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{plugin::PARAM_ON_LAP, 1},
                                                          plugin::PARAM_ON_LAP_NAME, plugin::ON_LAP_DEFAULT));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{plugin::PARAM_STEREO_WIDTH, 1}, plugin::PARAM_STEREO_WIDTH_NAME,
        juce::NormalisableRange<float>(0.0f, 1.0f, (float)plugin::STEREO_WIDTH_STEP), (float)STEREO_WIDTH_DEFAULT));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{plugin::PARAM_VOLUME, 1}, plugin::PARAM_VOLUME_NAME,
        juce::NormalisableRange<float>(plugin::VOLUME_MIN_DB, plugin::VOLUME_MAX_DB, 0.1f), plugin::VOLUME_DEFAULT_DB,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{plugin::PARAM_TEMPO, 1}, plugin::PARAM_TEMPO_NAME,
        juce::NormalisableRange<float>((float)TEMPO_MIN, (float)TEMPO_MAX, 1.0f), (float)TEMPO_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("BPM")));
    return layout;
}

void HandPanProcessor::set_pattern(const Pattern& pattern)
{
    pattern_ = pattern;
    compile_pattern();
}

void HandPanProcessor::set_hands(const PlayerHands& hands)
{
    hands_ = hands;
    compile_pattern();
}

void HandPanProcessor::compile_pattern()
{
    ++pattern_version_;
    patterns_.post(new CompiledPattern{PatternCompiler::compile(pattern_, notes_, hands_), pattern_.length_beats()});
}

void HandPanProcessor::audition(int note, int velocity)
{
    const auto scope = auditions_.write(1);
    if (scope.blockSize1 > 0)
        audition_strikes_[(size_t)scope.startIndex1] = {0, note, velocity};
}

void HandPanProcessor::play_auditions()
{
    const auto scope = auditions_.read(auditions_.getNumReady());
    for (int i = 0; i < scope.blockSize1; ++i)
        engine_.strike(audition_strikes_[(size_t)(scope.startIndex1 + i)].note,
                       audition_strikes_[(size_t)(scope.startIndex1 + i)].velocity);
    for (int i = 0; i < scope.blockSize2; ++i)
        engine_.strike(audition_strikes_[(size_t)(scope.startIndex2 + i)].note,
                       audition_strikes_[(size_t)(scope.startIndex2 + i)].velocity);
}

void HandPanProcessor::prepareToPlay(double sample_rate, int)
{
    sample_rate_ = sample_rate;
    solver_.set_sample_rate(sample_rate);
    gain_.reset(sample_rate, plugin::VOLUME_RAMP_SECONDS);
    gain_.setCurrentAndTargetValue(volume_gain(parameters_.getRawParameterValue(plugin::PARAM_VOLUME)->load()));
    if (!solver_.isThreadRunning())
        solver_.startThread(juce::Thread::Priority::low);
}

bool HandPanProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

double HandPanProcessor::getTailLengthSeconds() const { return plugin::TAIL_SECONDS; }

HostPosition HandPanProcessor::host_position() const
{
    HostPosition host;
    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
        {
            host.available = position->getBpm().hasValue();
            host.playing = position->getIsPlaying();
            host.beat = position->getPpqPosition().orFallback(0.0);
            host.tempo = position->getBpm().orFallback(0.0);
        }
    return host;
}

void HandPanProcessor::render(juce::AudioBuffer<float>& buffer, int start, int count)
{
    if (count <= 0)
        return;
    if (buffer.getNumChannels() >= 2)
        engine_.render_stereo(buffer.getWritePointer(0, start), buffer.getWritePointer(1, start), count);
    else
        engine_.render(buffer.getWritePointer(0, start), count);
}

void HandPanProcessor::play_pattern_until(juce::AudioBuffer<float>& buffer, int& position, int& next, int until)
{
    for (; next < block_strikes_.size() && block_strikes_[next].sample < until; ++next)
    {
        const BlockStrike& s = block_strikes_[next];
        render(buffer, position, s.sample - position);
        position = s.sample;
        engine_.strike(s.note, s.velocity);
    }
}

void HandPanProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals no_denormals;
    const int samples = buffer.getNumSamples();
    // a newer model from the solver, taken between two blocks; the fields keep ringing through it
    const SoundModel* fresh = models_.take();
    if (fresh != playing_)
    {
        playing_ = fresh;
        engine_.set_model(playing_);
    }
    const CompiledPattern* compiled = patterns_.take();
    if (compiled != playing_pattern_)
    {
        playing_pattern_ = compiled;
        sequencer_.set(&compiled->strikes, compiled->length_beats);
    }

    const double own_tempo = parameters_.getRawParameterValue(plugin::PARAM_TEMPO)->load();
    const ClockBlock clock = transport_.advance(host_position(), own_tempo, sample_rate_, samples);
    shown_tempo_.store(clock.tempo);
    block_strikes_.gather(sequencer_, clock, samples);
    play_auditions();

    int position = 0, next = 0;
    for (const auto event : midi)
    {
        const auto message = event.getMessage();
        if (!message.isNoteOn())
            continue;
        const int at = juce::jlimit(0, samples, event.samplePosition);
        play_pattern_until(buffer, position, next, at + 1); // a pattern strike at the same sample goes first
        render(buffer, position, at - position);
        position = at;
        engine_.strike(message.getNoteNumber(), message.getVelocity());
    }
    play_pattern_until(buffer, position, next, samples);
    render(buffer, position, samples - position);
    for (int c = 2; c < buffer.getNumChannels(); ++c)
        buffer.clear(c, 0, samples);

    gain_.setTargetValue(volume_gain(parameters_.getRawParameterValue(plugin::PARAM_VOLUME)->load()));
    gain_.applyGain(buffer, samples);
}

juce::AudioProcessorEditor* HandPanProcessor::createEditor() { return new HandPanEditor(*this); }

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
