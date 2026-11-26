// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The whole sound path without a host: a model, a list of strikes in time, rendered by the real-time engine in
// blocks, as a host would call it.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "engine.hpp"

namespace grmh
{
// one strike in time
struct TimedStrike
{
    double time; // s
    int note;    // MIDI
    int velocity; // MIDI, 1-127
};

class OfflineRenderer
{
public:
    static constexpr int BLOCK = 256;

    // `seconds` of sound of `strikes` (sorted by time or not) on `sound`
    static std::vector<float> render(const SoundModel& sound, std::vector<TimedStrike> strikes, double seconds)
    {
        std::vector<float> out(frames(sound, seconds), 0.0f);
        play(sound, std::move(strikes), out.size(),
             [&](Engine& engine, size_t at, int count) { engine.render(out.data() + at, count); });
        return out;
    }

    // the same in two channels, left first
    static std::vector<std::vector<float>> render_stereo(const SoundModel& sound, std::vector<TimedStrike> strikes,
                                                         double seconds)
    {
        std::vector<std::vector<float>> out(2, std::vector<float>(frames(sound, seconds), 0.0f));
        play(sound, std::move(strikes), out[0].size(), [&](Engine& engine, size_t at, int count)
             { engine.render_stereo(out[0].data() + at, out[1].data() + at, count); });
        return out;
    }

private:
    static size_t frames(const SoundModel& sound, double seconds)
    {
        return static_cast<size_t>(std::ceil(seconds * sound.sample_rate));
    }

    // strikes due at each sample, then a render up to the next strike or the end of the block, as a host calls
    template <class Render>
    static void play(const SoundModel& sound, std::vector<TimedStrike> strikes, size_t total, Render render)
    {
        std::sort(strikes.begin(), strikes.end(),
                  [](const TimedStrike& a, const TimedStrike& b) { return a.time < b.time; });
        Engine engine;
        engine.set_model(&sound);
        size_t next = 0, position = 0;
        while (position < total)
        {
            while (next < strikes.size() && sample_of(strikes[next].time, sound) <= position)
            {
                engine.strike(strikes[next].note, strikes[next].velocity);
                ++next;
            }
            size_t end = std::min(total, position + BLOCK);
            if (next < strikes.size())
                end = std::min(end, std::max(position + 1, sample_of(strikes[next].time, sound)));
            render(engine, position, static_cast<int>(end - position));
            position = end;
        }
    }

    static size_t sample_of(double time, const SoundModel& sound)
    {
        return static_cast<size_t>(std::max(0.0, std::round(time * sound.sample_rate)));
    }
};
} // namespace grmh
