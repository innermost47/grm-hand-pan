// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll's patterns as they are played: the grid of a signature, a circle as a velocity, the hands (each
// side by its hand, the middle in turn, two fingers at once, the player's choice), the weak hand, flams, and the loop
// along the song, the same whatever the host's blocks, heard from the side of the fields struck.
#include <cmath>
#include <cstdio>
#include <numeric>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/instrument.hpp"
#include "../core/pattern_compiler.hpp"
#include "../core/pattern_sequencer.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
// the D Kurd's fields: D3 (middle), A3 (front), Bb3 (left), C4 (right), D4 (left), E4 (right), F4 (left), G4 (right),
// A4 (back)
const std::vector<int> NOTES(D_KURD.begin(), D_KURD.end());
constexpr int DING = 0, BB3 = 2, D4 = 4, E4 = 5, F4 = 6;

PatternHit hit(int step, int field, double size = HIT_SIZE_DEFAULT, HandChoice hand = HandChoice::automatic)
{
    PatternHit h;
    h.step = step;
    h.field = field;
    h.size = size;
    h.hand = hand;
    return h;
}

std::vector<Hand> hands_of(const std::vector<PatternHit>& hits, Handedness handedness = Handedness::right_handed)
{
    Pattern p;
    p.hits = hits;
    return HandAssignment::assign(p, NOTES, handedness);
}

void check_grid(test::TestReport& report)
{
    Pattern four;
    report.expect(four.step_count() == 16 && std::fabs(four.step_beats() - 0.25) < 1e-12 &&
                      std::fabs(four.length_beats() - 4.0) < 1e-12,
                  "a bar of 4/4 in sixteenths: 16 steps of a quarter of a beat");
    Pattern six;
    six.signature = {6, 8};
    six.steps_per_beat = 2;
    six.bars = 2;
    report.expect(six.steps_per_bar() == 12 && std::fabs(six.length_beats() - 6.0) < 1e-12,
                  "two bars of 6/8 in sixteenths: 12 steps a bar, 6 quarter notes");
}

void check_velocity(test::TestReport& report)
{
    report.expect(PatternCompiler::hit_velocity(0.0) == 1 && PatternCompiler::hit_velocity(1.0) == 127 &&
                      PatternCompiler::hit_velocity(0.5) == 64 && PatternCompiler::hit_velocity(2.0) == 127,
                  "a circle from the smallest to the biggest is a velocity from 1 to 127");
    PlayerHands player;
    report.expect(PatternCompiler::hand_velocity(64, Hand::left, player) == 64, "by default both hands strike alike");
    player.weak_hand_speed = 0.5;
    report.expect(PatternCompiler::hand_velocity(64, Hand::left, player) == 32 &&
                      PatternCompiler::hand_velocity(64, Hand::right, player) == 64,
                  "a right-hander's left finger at half the speed: an octave of speed lower, 32 velocity steps");
    player.handedness = Handedness::left_handed;
    report.expect(PatternCompiler::hand_velocity(64, Hand::right, player) == 32 &&
                      PatternCompiler::hand_velocity(64, Hand::left, player) == 64,
                  "a left-hander's weak hand is the right one");
}

void check_hands(test::TestReport& report)
{
    const auto sides = hands_of({hit(0, E4), hit(1, D4), hit(2, BB3)});
    report.expect(sides[0] == Hand::right && sides[1] == Hand::left && sides[2] == Hand::left,
                  "each side by its own hand");

    const auto ding = hands_of({hit(0, DING), hit(1, DING), hit(2, DING), hit(3, DING)});
    report.expect(ding[0] == Hand::right && ding[1] == Hand::left && ding[2] == Hand::right && ding[3] == Hand::left,
                  "the ding in turn, the strong hand first (right-hander)");
    const auto lefty = hands_of({hit(0, DING), hit(1, DING)}, Handedness::left_handed);
    report.expect(lefty[0] == Hand::left && lefty[1] == Hand::right, "a left-hander leads with the left");

    const auto after = hands_of({hit(0, D4), hit(1, DING)});
    report.expect(after[1] == Hand::right, "the ding by the hand that did not play last");

    const auto together = hands_of({hit(0, D4), hit(0, F4)});
    report.expect(together[0] == Hand::left && together[1] == Hand::right,
                  "two fields of the left side at once: the one nearer the middle goes to the free hand");

    const auto chosen = hands_of({hit(0, E4, 0.5, HandChoice::left), hit(0, D4)});
    report.expect(chosen[0] == Hand::left && chosen[1] == Hand::right,
                  "the player's choice holds, the other finger takes the free hand");
}

void check_flam(test::TestReport& report)
{
    Pattern p;
    p.hits = {hit(4, E4, 0.3), hit(4, E4, 0.8)};
    const auto strikes = PatternCompiler::compile(p, NOTES, PlayerHands{});
    const bool shape = strikes.size() == 2 && strikes[0].velocity == PatternCompiler::hit_velocity(0.8) &&
                       strikes[0].offset == 0.0 && std::fabs(strikes[1].offset + FLAM_GAP) < 1e-12 &&
                       strikes[1].velocity == PatternCompiler::hit_velocity(0.3) && strikes[0].beat == 1.0 &&
                       strikes[1].beat == 1.0;
    report.expect(shape, "two hits on one field at one step: a flam, the louder on the beat, the softer a gap before");
    report.expect(strikes.size() == 2 && strikes[0].hand == Hand::right && strikes[1].hand == Hand::left,
                  "a flam is played by both hands, the grace by the free one");

    Pattern outside;
    outside.hits = {hit(0, 12), hit(16, DING), hit(15, DING)};
    report.expect(PatternCompiler::compile(outside, NOTES, PlayerHands{}).size() == 1,
                  "a hit on a field the instrument lacks, or past the pattern's end, is not played");
}

void check_loop(test::TestReport& report)
{
    Pattern p;
    p.hits = {hit(0, DING, 0.9), hit(0, DING, 0.4), hit(8, E4)};
    const auto strikes = PatternCompiler::compile(p, NOTES, PlayerHands{});
    PatternSequencer sequencer;
    sequencer.set(&strikes, p.length_beats());
    constexpr double TEMPO = 120.0;
    const double grace = FLAM_GAP * TEMPO / 60.0;

    std::vector<double> whole;
    sequencer.strikes_in(0.0, 8.0, TEMPO, [&](double beat, const ScheduledStrike&) { whole.push_back(beat); });
    std::sort(whole.begin(), whole.end());
    const std::vector<double> expected{0.0, 2.0, 4.0 - grace, 4.0, 6.0, 8.0 - grace};
    bool same = whole.size() == expected.size();
    for (size_t i = 0; same && i < whole.size(); ++i)
        same = std::fabs(whole[i] - expected[i]) < 1e-9;
    report.expect(same, "two loops: the grace of the first beat closes the loop before, none before the song starts");

    std::vector<double> blocks;
    for (double from = 0.0; from < 8.0; from += 0.37)
        sequencer.strikes_in(from, std::min(8.0, from + 0.37), TEMPO,
                             [&](double beat, const ScheduledStrike&) { blocks.push_back(beat); });
    std::sort(blocks.begin(), blocks.end());
    bool alike = blocks.size() == whole.size();
    for (size_t i = 0; alike && i < blocks.size(); ++i)
        alike = std::fabs(blocks[i] - whole[i]) < 1e-12;
    report.expect(alike, "the same strikes whatever the host's blocks");

    const auto timed = sequencer.timeline(2, TEMPO);
    report.expect(timed.size() == 6 && timed.front().time == 0.0 && std::fabs(timed[1].time - FLAM_GAP) < 1e-12,
                  "offline, the first grace opens the rendering and its main stroke follows a gap later");
}

double energy(const std::vector<float>& s)
{
    return std::accumulate(s.begin(), s.end(), 0.0, [](double e, float x) { return e + double(x) * x; });
}

void check_heard(test::TestReport& report)
{
    Settings settings;
    settings.notes = NOTES;
    const Model model = solve_instrument(settings, 48000.0);
    Pattern p;
    p.hits = {hit(0, E4, 0.7), hit(8, E4, 0.4)};
    const auto strikes = PatternCompiler::compile(p, NOTES, PlayerHands{});
    PatternSequencer sequencer;
    sequencer.set(&strikes, p.length_beats());
    const auto sound = OfflineRenderer::render_stereo(model.sound(), sequencer.timeline(1, 100.0), 3.0);
    const double right_over_left = 10.0 * std::log10(energy(sound[1]) / energy(sound[0]));
    std::printf("  a pattern on E4: right over left %+.1f dB\n", right_over_left);
    report.expect(energy(sound[0]) > 0.0 && right_over_left > 3.0,
                  "a pattern played on E4 sounds, from the right, where E4 sits");
}
} // namespace

int main()
{
    test::TestReport report("pattern");
    check_grid(report);
    check_velocity(report);
    check_hands(report);
    check_flam(report);
    check_loop(report);
    check_heard(report);
    return report.finish();
}
