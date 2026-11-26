// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The solver thread: watches the settings, solves the instrument when they change (the calibrations once, the first
// time) and hands the sound model to the audio thread through the mailbox. A change of stereo width only patches a
// copy of the last model. Never touches the audio buffers.
#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "../core/mailbox.hpp"
#include "../core/model.hpp"
#include "JuceHeader.h"

class SolverThread : public juce::Thread
{
public:
    SolverThread(juce::AudioProcessorValueTreeState& parameters, grmh::Mailbox<grmh::SoundModel>& models);
    ~SolverThread() override;

    // the host's sample rate, from prepareToPlay
    void set_sample_rate(double sample_rate) { sample_rate_.store(sample_rate); }

    // true once a first model has been posted
    bool ready() const { return ready_.load(); }

    // any thread but the audio one: the last model posted (null before the first), to render offline
    std::shared_ptr<const grmh::SoundModel> shown_model() const
    {
        const std::lock_guard<std::mutex> lock(shown_lock_);
        return shown_;
    }

    void run() override;

private:
    grmh::Settings read_settings() const;
    void post(const grmh::SoundModel& sound);

    juce::AudioProcessorValueTreeState& parameters_;
    grmh::Mailbox<grmh::SoundModel>& models_;
    std::atomic<double> sample_rate_{0.0};
    std::atomic<bool> ready_{false};
    mutable std::mutex shown_lock_; // never taken by the audio thread
    std::shared_ptr<const grmh::SoundModel> shown_;

    std::unique_ptr<grmh::Model> last_model_;
    grmh::Settings last_settings_;
    double last_rate_ = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolverThread)
};
