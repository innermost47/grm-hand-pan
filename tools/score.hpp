// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A score written in beats, played as timed strikes: single strikes, rolls (alternating hands, with a crescendo),
// flams. A human player is never exactly on time nor exactly as loud: each strike moves a few milliseconds and a few
// velocity steps, the same way at every run (a fixed seed).
#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

#include "../core/offline_renderer.hpp"

namespace grmh
{
class Score
{
public:
    Score(double tempo_bpm, double timing_jitter_s, int velocity_jitter, std::uint32_t seed)
        : beat_(60.0 / tempo_bpm), timing_jitter_(timing_jitter_s), velocity_jitter_(velocity_jitter), random_(seed)
    {
    }

    // one strike at a beat (from the start of the piece)
    void strike(double beat, int note, int velocity) { strikes_.push_back({beat * beat_, note, velocity}); }

    // `notes` struck in turn, `per_beat` strikes a beat, for `beats` beats, the velocity going from `from` to `to`
    void roll(double beat, double beats, int per_beat, const std::vector<int>& notes, int from, int to)
    {
        const int count = static_cast<int>(beats * per_beat);
        for (int i = 0; i < count; ++i)
        {
            const double share = count > 1 ? double(i) / (count - 1) : 0.0;
            strike(beat + double(i) / per_beat, notes[(size_t)i % notes.size()],
                   static_cast<int>(from + share * (to - from)));
        }
    }

    // two fingers landing one after the other, the first softer: a flam
    void flam(double beat, int note, int velocity)
    {
        strikes_.push_back({beat * beat_ - FLAM_GAP, note, velocity / 2});
        strike(beat, note, velocity);
    }

    double seconds(double beats) const { return beats * beat_; }

    // the strikes, as a player would play them
    std::vector<TimedStrike> play()
    {
        std::normal_distribution<double> timing(0.0, timing_jitter_);
        std::uniform_int_distribution<int> loudness(-velocity_jitter_, velocity_jitter_);
        std::vector<TimedStrike> played = strikes_;
        for (TimedStrike& s : played)
        {
            s.time = std::max(0.0, s.time + timing(random_));
            s.velocity = std::clamp(s.velocity + loudness(random_), 1, 127);
        }
        return played;
    }

private:
    static constexpr double FLAM_GAP = 0.03; // s between the two fingers of a flam

    double beat_;
    double timing_jitter_;
    int velocity_jitter_;
    std::mt19937 random_;
    std::vector<TimedStrike> strikes_;
};
} // namespace grmh
