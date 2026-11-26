// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A pattern as it is played, solved off the audio thread: each hit on its beat, struck by its hand, at the velocity
// of its circle and of that hand; the hits that share a field and a step are a flam, the loudest on the beat, each
// softer one a FLAM_GAP before the next.
#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "../constants/audio.hpp"
#include "../models/pattern.hpp"
#include "hand_assignment.hpp"

namespace grmh
{
class PatternCompiler
{
public:
    // the MIDI velocity of a hit's circle (0..1)
    static int hit_velocity(double size)
    {
        const double share = std::clamp(size, 0.0, 1.0);
        return HIT_VELOCITY_MIN + (int)std::lround(share * (HIT_VELOCITY_MAX - HIT_VELOCITY_MIN));
    }

    // the velocity struck by `hand`: the weak hand's fingertip is slower, and a MIDI velocity is a fingertip speed
    // (an octave of speed every MIDI_VELOCITY_STEPS_PER_DOUBLING steps, constants/audio.hpp)
    static int hand_velocity(int velocity, Hand hand, const PlayerHands& player)
    {
        if (hand == strong_hand(player.handedness) || player.weak_hand_speed <= 0.0)
            return velocity;
        const double shift = MIDI_VELOCITY_STEPS_PER_DOUBLING * std::log2(player.weak_hand_speed);
        return std::clamp(velocity + (int)std::lround(shift), HIT_VELOCITY_MIN, HIT_VELOCITY_MAX);
    }

    // the strikes of `pattern` on an instrument of these notes (the ding first), by beat; a hit on a field the
    // instrument does not have, or past the pattern's end, is not played
    static std::vector<ScheduledStrike> compile(const Pattern& pattern, const std::vector<int>& notes,
                                                const PlayerHands& player)
    {
        const std::vector<Hand> hands = HandAssignment::assign(pattern, notes, player.handedness);
        const auto& hits = pattern.hits;
        std::vector<size_t> order;
        for (size_t i = 0; i < hits.size(); ++i)
            if (hits[i].field >= 0 && hits[i].field < (int)notes.size() && hits[i].step >= 0 &&
                hits[i].step < pattern.step_count())
                order.push_back(i);
        std::stable_sort(order.begin(), order.end(),
                         [&](size_t a, size_t b)
                         {
                             if (hits[a].step != hits[b].step)
                                 return hits[a].step < hits[b].step;
                             if (hits[a].field != hits[b].field)
                                 return hits[a].field < hits[b].field;
                             return hits[a].size > hits[b].size; // the loudest first: on the beat
                         });

        std::vector<ScheduledStrike> strikes;
        strikes.reserve(order.size());
        for (size_t k = 0; k < order.size(); ++k)
        {
            const PatternHit& hit = hits[order[k]];
            int grace = 0; // hits before this one on the same field and step
            while (grace < (int)k && same_place(hits[order[k - 1 - (size_t)grace]], hit))
                ++grace;
            ScheduledStrike s;
            s.beat = hit.step * pattern.step_beats();
            s.offset = -grace * FLAM_GAP;
            s.note = notes[(size_t)hit.field];
            s.hand = hands[order[k]];
            s.velocity = hand_velocity(hit_velocity(hit.size), s.hand, player);
            strikes.push_back(s);
        }
        return strikes;
    }

private:
    static bool same_place(const PatternHit& a, const PatternHit& b) { return a.step == b.step && a.field == b.field; }
};
} // namespace grmh
