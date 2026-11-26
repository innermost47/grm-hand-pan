// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A compiled pattern played in a loop along the song: the strikes that fall in a stretch of beats (a host's block),
// at a tempo. It reads the strikes, never allocates: it can run in the audio thread. A flam's grace before the
// pattern's first beat is played at the end of the loop before.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "../models/pattern.hpp"
#include "offline_renderer.hpp"

namespace grmh
{
class PatternSequencer
{
public:
    // plays these strikes from now on, looped every `length_beats` quarter notes, or once when `loop` is false (they
    // outlive their use here)
    void set(const std::vector<ScheduledStrike>* strikes, double length_beats, bool loop = true)
    {
        strikes_ = strikes;
        length_ = length_beats;
        loop_ = loop;
    }

    // every strike in [from, to) of the song (quarter notes, the pattern starting at 0), at `tempo` BPM:
    // emit(beat, strike), in the order of the pattern's strikes, not of time
    template <class Emit>
    void strikes_in(double from, double to, double tempo, Emit emit) const
    {
        if (strikes_ == nullptr || length_ <= 0.0 || tempo <= 0.0 || to <= from)
            return;
        for (const ScheduledStrike& s : *strikes_)
        {
            const double at = s.beat + s.offset * tempo / 60.0;
            if (!loop_)
            {
                if (at >= from && at < to)
                    emit(at, s);
                continue;
            }
            for (double k = std::ceil((from - at) / length_); at + k * length_ < to; k += 1.0)
                emit(at + k * length_, s);
        }
    }

    // `loops` loops at `tempo` (once when it does not loop), in seconds from the earliest grace (for the offline
    // renderer)
    std::vector<TimedStrike> timeline(int loops, double tempo) const
    {
        std::vector<TimedStrike> timed;
        if (strikes_ == nullptr)
            return timed;
        double lead = 0.0; // beats: the earliest grace before the first beat
        for (const ScheduledStrike& s : *strikes_)
            lead = std::max(lead, -(s.beat + s.offset * tempo / 60.0));
        const double end = loop_ ? loops * length_ - lead : length_; // played once: to its end
        strikes_in(-lead, end, tempo, [&](double beat, const ScheduledStrike& s)
                   { timed.push_back({(beat + lead) * 60.0 / tempo, s.note, s.velocity}); });
        std::sort(timed.begin(), timed.end(), [](const TimedStrike& a, const TimedStrike& b) { return a.time < b.time; });
        return timed;
    }

private:
    const std::vector<ScheduledStrike>* strikes_ = nullptr;
    double length_ = 0.0;
    bool loop_ = true;
};
} // namespace grmh
