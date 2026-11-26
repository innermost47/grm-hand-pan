// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin: a D Kurd handpan played from MIDI. Each note-on strikes the field tuned to it, at its velocity, to the
// sample; note-offs are ignored (a handpan rings until it fades). The sound model comes from the solver thread through
// a lock-free mailbox; the audio thread never allocates, locks or solves.
#pragma once

#include "../core/engine.hpp"
#include "../core/mailbox.hpp"
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout create_layout();
    void render(juce::AudioBuffer<float>& buffer, int start, int count);

    juce::AudioProcessorValueTreeState parameters_;
    grmh::Mailbox<grmh::SoundModel> models_; // declared before the solver: outlives it
    SolverThread solver_;
    grmh::Engine engine_;
    const grmh::SoundModel* playing_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HandPanProcessor)
};
