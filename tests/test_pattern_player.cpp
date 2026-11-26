// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Patterns asked for along the song (Anthony, 26/11/2026): a pattern starts on the next bar line, from its first
// step; another one asked for takes over on the bar line after; let go, it stops at once; nothing plays while the
// clock is still; the same whatever the host's blocks.
#include <cmath>
#include <vector>

#include "../core/pattern_player.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
constexpr double RATE = 48000.0, TEMPO = 120.0;
constexpr int BLOCK = 512;

// two one-bar patterns of 4/4: the first strikes note 50 on its first step, the second note 64
CompiledLibrary library()
{
    CompiledLibrary lib;
    for (int note : {50, 64})
    {
        CompiledPattern p;
        ScheduledStrike s;
        s.note = note;
        s.velocity = 90;
        p.strikes = {s};
        p.length_beats = 4.0;
        p.bar_beats = 4.0;
        lib.patterns.push_back(p);
    }
    return lib;
}

struct Heard
{
    double beat;
    int note;
};

// a host calling the player block after block: the song goes on from where the last call left it
struct Song
{
    double beat = 0.0;
    int block = BLOCK;
    bool playing = true;

    // the strikes up to beat `to` (blocks of `block` samples: the last one may end past it)
    std::vector<Heard> play_to(PatternPlayer& player, const CompiledLibrary& lib, double to)
    {
        std::vector<Heard> heard;
        BlockStrikes strikes;
        const double bps = TEMPO / 60.0 / RATE;
        for (; beat < to - 1e-12; beat += block * bps)
        {
            player.gather(lib, ClockBlock{playing, beat, TEMPO, bps}, block, strikes);
            for (int i = 0; i < strikes.size(); ++i)
                heard.push_back({beat + strikes[i].sample * bps, strikes[i].note});
        }
        return heard;
    }
};

bool near(double a, double b) { return std::fabs(a - b) < 2.0 * TEMPO / 60.0 / RATE; }

void check_requests(test::TestReport& report)
{
    const CompiledLibrary lib = library();
    PatternPlayer player;
    Song song;
    report.expect(song.play_to(player, lib, 1.0).empty(), "nothing asked for: silence");
    player.request(1);
    const auto first = song.play_to(player, lib, 6.0);
    report.expect(first.size() == 1 && near(first[0].beat, 4.0) && first[0].note == 64,
                  "asked for at beat 1: the second pattern starts on the next bar line, beat 4");
    player.request(0);
    const auto second = song.play_to(player, lib, 13.0);
    report.expect(second.size() == 2 && second[0].note == 50 && near(second[0].beat, 8.0) && second[1].note == 50 &&
                      near(second[1].beat, 12.0),
                  "the first asked for at beat 6: it takes over on the bar line, beat 8");
    report.expect(player.playing() == 0, "then the first plays");
    player.release(0);
    report.expect(song.play_to(player, lib, 20.0).empty() && player.playing() == PatternPlayer::NONE,
                  "let go: it stops at once");
}

void check_edges(test::TestReport& report)
{
    const CompiledLibrary lib = library();
    PatternPlayer still;
    Song stopped;
    stopped.playing = false;
    still.request(0);
    report.expect(stopped.play_to(still, lib, 8.0).empty() && still.requested() == 0,
                  "the clock still: nothing plays, the pattern still awaited");
    Song from_line;
    from_line.beat = 4.0;
    const auto on_line = from_line.play_to(still, lib, 4.5);
    report.expect(on_line.size() == 1 && near(on_line[0].beat, 4.0), "asked for on a bar line: it starts there");

    PatternPlayer a, b;
    a.request(1);
    b.request(1);
    Song big{0.5, 4096}, small{0.5, 37};
    auto heard_a = big.play_to(a, lib, 11.0), heard_b = small.play_to(b, lib, 11.0);
    bool same = heard_a.size() == heard_b.size() && heard_a.size() == 2;
    for (size_t i = 0; same && i < heard_a.size(); ++i)
        same = near(heard_a[i].beat, heard_b[i].beat) && heard_a[i].note == heard_b[i].note;
    report.expect(same, "the same strikes whatever the host's blocks (a bar line in the middle of a block)");

    PatternPlayer let_go;
    Song song;
    let_go.request(1);
    let_go.release(1);
    report.expect(song.play_to(let_go, lib, 8.0).empty(), "let go before its bar line: it never starts");
}
} // namespace

int main()
{
    test::TestReport report("pattern player");
    check_requests(report);
    check_edges(report);
    return report.finish();
}
