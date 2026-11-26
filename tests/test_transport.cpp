// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The song's clock: the host's while it plays, the plugin's own otherwise (play, pause, stop back to the start, at its
// own tempo); the pattern's strikes of a block at their sample, in time order; a beat read as bar.beat.step.
#include <cmath>
#include <cstdio>

#include "../constants/scales.hpp"
#include "../core/block_strikes.hpp"
#include "../core/pattern_compiler.hpp"
#include "../core/song_position.hpp"
#include "../core/transport.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
constexpr double RATE = 48000.0;
constexpr int BLOCK = 512;

void check_own_clock(test::TestReport& report)
{
    Transport t;
    const HostPosition none;
    ClockBlock b = t.advance(none, 120.0, RATE, BLOCK);
    report.expect(!b.playing && b.start_beat == 0.0, "the plugin's clock waits for Play");
    t.play();
    b = t.advance(none, 120.0, RATE, BLOCK);
    const ClockBlock next = t.advance(none, 120.0, RATE, BLOCK);
    report.expect(b.playing && b.start_beat == 0.0 && std::fabs(next.start_beat - BLOCK * 2.0 / RATE) < 1e-12,
                  "played at 120 BPM, it moves two beats a second");
    t.pause();
    const ClockBlock paused = t.advance(none, 120.0, RATE, BLOCK);
    const ClockBlock still = t.advance(none, 120.0, RATE, BLOCK);
    report.expect(!paused.playing && paused.start_beat == still.start_beat && paused.start_beat > 0.0,
                  "paused, it stays where it was");
    t.stop();
    report.expect(t.advance(none, 120.0, RATE, BLOCK).start_beat == 0.0 && !t.shown_playing(),
                  "stopped, it goes back to the start");
}

void check_host_clock(test::TestReport& report)
{
    Transport t;
    HostPosition host{true, true, 37.5, 84.0};
    const ClockBlock b = t.advance(host, 120.0, RATE, BLOCK);
    report.expect(b.playing && b.start_beat == 37.5 && b.tempo == 84.0 && t.shown_beat() == 37.5,
                  "while the host plays, its position and its tempo");
    host.playing = false;
    const ClockBlock idle = t.advance(host, 120.0, RATE, BLOCK);
    report.expect(!idle.playing && idle.tempo == 84.0, "the host stopped: silence, at the host's tempo");
}

void check_block_strikes(test::TestReport& report)
{
    Pattern p;
    PatternHit late, early;
    late.step = 4; // beat 1
    late.field = 5;
    early.step = 0;
    early.field = 0;
    p.hits = {late, early};
    const std::vector<int> notes(D_KURD.begin(), D_KURD.end());
    const std::vector<ScheduledStrike> strikes = PatternCompiler::compile(p, notes, PlayerHands{});
    PatternSequencer sequencer;
    sequencer.set(&strikes, p.length_beats());

    ClockBlock block{true, 0.0, 120.0, 120.0 / 60.0 / RATE};
    const int samples = (int)RATE; // one second: two beats
    BlockStrikes in_block;
    in_block.gather(sequencer, block, samples);
    report.expect(in_block.size() == 2 && in_block[0].sample == 0 && in_block[0].note == D_KURD[0] &&
                      in_block[1].sample == (int)(RATE / 2.0) && in_block[1].note == D_KURD[5],
                  "a block holds the pattern's strikes at their sample, in time order");
    block.playing = false;
    in_block.gather(sequencer, block, samples);
    report.expect(in_block.size() == 0, "nothing while the clock is still");
}

void check_position(test::TestReport& report)
{
    Pattern four;
    const SongPosition start = song_position(0.0, four);
    const SongPosition later = song_position(4.0 * 31 + 0.25, four); // bar 32, beat 1, second sixteenth
    report.expect(start.bar == 1 && start.beat == 1 && start.step == 1, "the song starts at 1.1.1");
    report.expect(later.bar == 32 && later.beat == 1 && later.step == 2, "a beat read in 4/4: 32.1.2");
    Pattern six;
    six.signature = {6, 8};
    six.steps_per_beat = 2;
    const SongPosition in_six = song_position(3.0 + 1.25, six); // a bar of 6/8 is 3 quarter notes
    report.expect(in_six.bar == 2 && in_six.beat == 3 && in_six.step == 2, "a beat read in 6/8: 2.3.2");
}
} // namespace

int main()
{
    test::TestReport report("transport");
    check_own_clock(report);
    check_host_clock(report);
    check_block_strikes(report);
    check_position(report);
    return report.finish();
}
