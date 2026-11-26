// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "HandPanProcessor.h"

#include "../constants/audio.hpp"
#include "../constants/plugin.hpp"
#include "../constants/scales.hpp"
#include "../core/pattern_compiler.hpp"
#include "HandPanEditor.h"
#include "state/SessionState.h"

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
    compile_library();
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
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{plugin::PARAM_MIDI_MODE, 1}, plugin::PARAM_MIDI_MODE_NAME,
        juce::StringArray(plugin::MIDI_MODE_CHOICES[0], plugin::MIDI_MODE_CHOICES[1]), plugin::MIDI_MODE_NOTES));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{plugin::PARAM_SONG, 1},
                                                          plugin::PARAM_SONG_NAME, false));
    return layout;
}

void HandPanProcessor::set_pattern(const Pattern& pattern)
{
    library_.pattern() = pattern;
    compile_library();
}

void HandPanProcessor::select_pattern(SlotId id)
{
    library_.select(id);
    compile_library();
}

void HandPanProcessor::set_slot(SlotId id, const PatternSlot& slot)
{
    if (!PatternLibrary::valid(id))
        return;
    library_.slot(id) = slot;
    compile_library();
}

void HandPanProcessor::set_hands(const PlayerHands& hands)
{
    hands_ = hands;
    compile_library();
}

void HandPanProcessor::compile_library()
{
    ++pattern_version_;
    auto* compiled = new CompiledLibrary;
    compiled->patterns.reserve((size_t)(BANKS * PATTERNS_PER_BANK));
    for (int b = 0; b < BANKS; ++b)
        for (int s = 0; s < PATTERNS_PER_BANK; ++s)
            compiled->patterns.push_back(compile(library_.slot({b, s}).pattern));
    compiled->edited = library_.current().bank * PATTERNS_PER_BANK + library_.current().slot;
    compiled->song = compiled_song();
    libraries_.post(compiled);
}

CompiledPattern HandPanProcessor::compile(const Pattern& p) const
{
    return {PatternCompiler::compile(p, notes_, hands_), p.length_beats(), p.steps_per_bar() * p.step_beats()};
}

CompiledPattern HandPanProcessor::compiled_song() const
{
    return ArrangementCompiler::compile(arrangement_, library_, notes_, hands_);
}

double HandPanProcessor::played_tempo() const
{
    const double shown = shown_tempo();
    return shown > 0.0 ? shown : parameters_.getRawParameterValue(plugin::PARAM_TEMPO)->load();
}

float HandPanProcessor::output_gain() const
{
    return volume_gain(parameters_.getRawParameterValue(plugin::PARAM_VOLUME)->load());
}

void HandPanProcessor::set_arrangement(const Arrangement& arrangement)
{
    arrangement_ = arrangement;
    compile_library();
}

std::vector<BlockSpan> HandPanProcessor::song_spans() const { return ArrangementCompiler::spans(arrangement_, library_); }

double HandPanProcessor::pattern_origin(double beat) const
{
    if (!song_mode())
        return 0.0;
    const auto spans = song_spans();
    const double place = ArrangementCompiler::song_place(arrangement_, spans, beat);
    const int block = ArrangementCompiler::block_at(spans, place);
    return block >= 0 ? beat - (place - spans[(size_t)block].start) : 0.0;
}

// what plays along the song: the arrangement (Song on), the patterns the keys hold (keys play patterns), or the
// pattern being edited
void HandPanProcessor::gather_strikes(const ClockBlock& clock, int samples, bool song, bool keys_play_patterns)
{
    if (playing_library_ == nullptr)
    {
        block_strikes_.clear();
        return;
    }
    if (keys_play_patterns)
    {
        player_.gather(*playing_library_, clock, samples, block_strikes_);
        return;
    }
    const CompiledPattern& played =
        song ? playing_library_->song : playing_library_->patterns[(size_t)playing_library_->edited];
    PatternSequencer sequencer;
    sequencer.set(&played.strikes, played.length_beats, played.loop);
    block_strikes_.gather(sequencer, clock, samples);
}

bool HandPanProcessor::patterns_mode() const
{
    return (int)parameters_.getRawParameterValue(plugin::PARAM_MIDI_MODE)->load() == plugin::MIDI_MODE_PATTERNS;
}

bool HandPanProcessor::song_mode() const { return parameters_.getRawParameterValue(plugin::PARAM_SONG)->load() > 0.5f; }

// the keys of the patterns' mode: a key asks for its pattern from the next bar line, let go it stops it (a key
// pressed late in a block counts from the block's start: a bar line just before it in the same block may start it)
void HandPanProcessor::read_pattern_keys(const juce::MidiBuffer& midi)
{
    for (const auto event : midi)
    {
        const auto message = event.getMessage();
        const int index = message.getNoteNumber() - plugin::PATTERN_KEY_FIRST;
        if (index < 0 || index >= BANKS * PATTERNS_PER_BANK)
            continue;
        if (message.isNoteOn())
            player_.request(index);
        else if (message.isNoteOff())
            player_.release(index);
    }
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
    playing_library_ = libraries_.take();

    const double own_tempo = parameters_.getRawParameterValue(plugin::PARAM_TEMPO)->load();
    const ClockBlock clock = transport_.advance(host_position(), own_tempo, sample_rate_, samples);
    shown_tempo_.store(clock.tempo);
    const bool song = song_mode(), keys_play_patterns = patterns_mode() && !song;
    if (keys_play_patterns)
        read_pattern_keys(midi);
    else
        player_.stop();
    gather_strikes(clock, samples, song, keys_play_patterns);
    shown_pattern_.store(keys_play_patterns ? player_.playing() : PatternPlayer::NONE);
    play_auditions();

    int position = 0, next = 0;
    for (const auto event : midi)
    {
        const auto message = event.getMessage();
        if (!message.isNoteOn() || keys_play_patterns)
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
    juce::ValueTree state = parameters_.copyState();
    SessionState::write(state, library_, arrangement_, hands_);
    if (const auto xml = state.createXml())
        copyXmlToBinary(*xml, data);
}

void HandPanProcessor::setStateInformation(const void* data, int size)
{
    const auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr || !xml->hasTagName(parameters_.state.getType()))
        return;
    juce::ValueTree state = juce::ValueTree::fromXml(*xml);
    SessionState::read(state, library_, arrangement_, hands_); // and takes its own part out
    parameters_.replaceState(state);
    compile_library();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HandPanProcessor(); }
