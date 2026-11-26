// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "SolverThread.h"

#include <cmath>
#include <exception>

#include "../constants/plugin.hpp"
#include "../constants/scales.hpp"
#include "../core/instrument.hpp"
#include "../core/layout.hpp"

SolverThread::SolverThread(juce::AudioProcessorValueTreeState& parameters, grmh::Mailbox<grmh::SoundModel>& models)
    : juce::Thread("GRM Hand Pan solver"), parameters_(parameters), models_(models)
{
}

SolverThread::~SolverThread() { stopThread(grmh::plugin::SOLVER_STOP_TIMEOUT_MS); }

grmh::Settings SolverThread::read_settings() const
{
    grmh::Settings settings;
    settings.notes.assign(grmh::D_KURD.begin(), grmh::D_KURD.end());
    settings.on_lap = parameters_.getRawParameterValue(grmh::plugin::PARAM_ON_LAP)->load() > 0.5f;
    const double width = parameters_.getRawParameterValue(grmh::plugin::PARAM_STEREO_WIDTH)->load();
    settings.stereo_width = std::round(width / grmh::plugin::STEREO_WIDTH_STEP) * grmh::plugin::STEREO_WIDTH_STEP;
    return settings;
}

void SolverThread::post(const grmh::SoundModel& sound)
{
    models_.post(new grmh::SoundModel(sound));
    {
        const std::lock_guard<std::mutex> lock(shown_lock_);
        shown_ = std::make_shared<const grmh::SoundModel>(sound);
    }
    ready_.store(true);
}

void SolverThread::run()
{
    while (!threadShouldExit())
    {
        models_.collect();
        const double rate = sample_rate_.load();
        const grmh::Settings settings = read_settings();
        // the instrument these settings ask for was already solved, or tried and failed: not again
        const bool attempted = rate == last_rate_ && settings.on_lap == last_settings_.on_lap &&
                               settings.instrument_seed == last_settings_.instrument_seed;
        try
        {
            if (rate > 0.0 && !attempted)
            {
                last_settings_ = settings;
                last_rate_ = rate;
                last_model_ = std::make_unique<grmh::Model>(grmh::solve_instrument(settings, rate));
                post(last_model_->sound());
            }
            else if (attempted && last_model_ != nullptr && settings.stereo_width != last_settings_.stereo_width)
            {
                // where the fields sound needs no network: a patched copy
                grmh::SoundModel sound = last_model_->sound();
                grmh::apply_layout(sound, settings.notes, settings.stereo_width);
                last_settings_.stereo_width = settings.stereo_width;
                post(sound);
            }
        }
        catch (const std::exception& e)
        {
            // the audio keeps the last model it took; these settings are not tried again
            juce::Logger::writeToLog(juce::String("GRM Hand Pan: the instrument could not be solved: ") + e.what());
            last_model_.reset(); // nothing of an older instrument to patch
        }
        wait(grmh::plugin::SOLVER_PERIOD_MS);
    }
}
