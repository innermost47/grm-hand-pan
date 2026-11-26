// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The song's clock. While the host plays, its position and tempo; otherwise the plugin's own clock, which the
// interface plays, pauses and stops (back to the start), at the plugin's own tempo. Any thread asks; the audio thread
// advances it, block after block, and leaves where it is for the interface to show.
#pragma once

#include <atomic>

#include "../models/transport.hpp"

namespace grmh
{
class Transport
{
public:
    // any thread: the plugin's own clock
    void play() { requested_.store(Request::play); }
    void pause() { requested_.store(Request::pause); }
    void stop() { requested_.store(Request::stop); }

    // any thread: what the interface shows (the beat at the last block's start, and whether it plays)
    double shown_beat() const { return shown_beat_.load(); }
    bool shown_playing() const { return shown_playing_.load(); }

    // audio thread: the block of `samples` that starts now, at `sample_rate`; `own_tempo` when the host gives none
    ClockBlock advance(const HostPosition& host, double own_tempo, double sample_rate, int samples)
    {
        apply_request();
        ClockBlock block;
        const bool host_plays = host.available && host.playing;
        block.tempo = host.available && host.tempo > 0.0 ? host.tempo : own_tempo;
        block.playing = host_plays || own_playing_;
        block.start_beat = host_plays ? host.beat : own_beat_;
        block.beats_per_sample = sample_rate > 0.0 ? block.tempo / 60.0 / sample_rate : 0.0;
        if (!host_plays && own_playing_)
            own_beat_ += block.beats_per_sample * samples;
        shown_beat_.store(block.start_beat);
        shown_playing_.store(block.playing);
        return block;
    }

private:
    enum class Request
    {
        none,
        play,
        pause,
        stop
    };

    void apply_request()
    {
        switch (requested_.exchange(Request::none))
        {
        case Request::play:
            own_playing_ = true;
            break;
        case Request::pause:
            own_playing_ = false;
            break;
        case Request::stop:
            own_playing_ = false;
            own_beat_ = 0.0;
            break;
        case Request::none:
            break;
        }
    }

    std::atomic<Request> requested_{Request::none};
    std::atomic<double> shown_beat_{0.0};
    std::atomic<bool> shown_playing_{false};
    bool own_playing_ = false; // audio thread
    double own_beat_ = 0.0;    // audio thread
};
} // namespace grmh
