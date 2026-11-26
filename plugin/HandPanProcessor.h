// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin: a D Kurd handpan played from MIDI and from its own roll. Each note-on strikes the field tuned to it, at
// its velocity, to the sample; note-offs are ignored (a handpan rings until it fades). The roll's pattern plays along
// the song (the host's clock, or the plugin's own), its strikes to the sample too. The sound model comes from the
// solver thread, the compiled pattern from the interface, both through lock-free mailboxes; the audio thread never
// allocates, locks or solves.
#pragma once

#include "../core/block_strikes.hpp"
#include "../core/engine.hpp"
#include "../core/mailbox.hpp"
#include "../core/pattern_sequencer.hpp"
#include "../core/transport.hpp"
#include "JuceHeader.h"
#include "SolverThread.h"

class HandPanProcessor : public juce::AudioProcessor
{
public:
    HandPanProcessor();
    ~HandPanProcessor() override;

    void prepareToPlay(double sample_rate, int block_size) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& data) override;
    void setStateInformation(const void* data, int size) override;

    // true once the instrument is solved and playing (for hosts and tests)
    bool instrument_ready() const { return solver_.ready(); }

    juce::AudioProcessorValueTreeState& parameters() { return parameters_; }
    grmh::Transport& transport() { return transport_; }
    bool standalone() const { return wrapperType == wrapperType_Standalone; }
    // the tempo of the last block (the host's, or the plugin's own)
    double shown_tempo() const { return shown_tempo_.load(); }

    // message thread: the roll's pattern, and a new one (compiled here, played from the next block)
    const grmh::Pattern& pattern() const { return pattern_; }
    void set_pattern(const grmh::Pattern& pattern);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout create_layout();
    grmh::HostPosition host_position() const;
    void render(juce::AudioBuffer<float>& buffer, int start, int count);
    // the pattern's strikes before sample `until`, each after rendering up to it; `position` follows
    void play_pattern_until(juce::AudioBuffer<float>& buffer, int& position, int& next, int until);

    juce::AudioProcessorValueTreeState parameters_;
    grmh::Mailbox<grmh::SoundModel> models_; // declared before the solver: outlives it
    SolverThread solver_;
    grmh::Engine engine_;
    const grmh::SoundModel* playing_ = nullptr;

    grmh::Pattern pattern_;                          // message thread
    grmh::Mailbox<grmh::CompiledPattern> patterns_;  // message thread -> audio thread
    const grmh::CompiledPattern* playing_pattern_ = nullptr;
    grmh::PatternSequencer sequencer_;
    grmh::BlockStrikes block_strikes_;
    grmh::Transport transport_;
    std::atomic<double> shown_tempo_{0.0};
    double sample_rate_ = 0.0;
    juce::SmoothedValue<float> gain_{1.0f}; // the volume, gliding

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HandPanProcessor)
};
