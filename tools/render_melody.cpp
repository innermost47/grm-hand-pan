// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A short piece for the D Kurd handpan, to hear the instrument played rather than probed: the ding as a pulse, rolls,
// flams, arpeggios and a sung phrase, at 84 BPM in sixteen bars. Usage: render_melody [out.wav] [--lap]
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "../constants/scales.hpp"
#include "../core/instrument.hpp"
#include "../core/wav_writer.hpp"
#include "score.hpp"

using namespace grmh;

namespace
{
// the D Kurd's fields
constexpr int D3 = D_KURD[0], A3 = D_KURD[1], Bb3 = D_KURD[2], C4 = D_KURD[3], D4 = D_KURD[4], E4 = D_KURD[5],
              F4 = D_KURD[6], G4 = D_KURD[7], A4 = D_KURD[8];

// bars 1-4: the ding opens each bar, an answer of eighths, a ghost ding before the next bar
void opening(Score& s)
{
    const int answers[4][5] = {{A3, D4, F4, E4, D4}, {C4, E4, G4, F4, E4}, {Bb3, D4, F4, A4, G4}, {A3, C4, E4, D4, A3}};
    for (int bar = 0; bar < 4; ++bar)
    {
        const double b = 4.0 * bar;
        s.strike(b, D3, 84);
        for (int i = 0; i < 5; ++i)
            s.strike(b + 1.0 + 0.5 * i, answers[bar][i], 58 + 4 * (i % 2 == 0));
        s.strike(b + 3.5, D3, 34);
    }
}

// bars 5-8: rolls between two fields, a fast roll falling into a descending arpeggio, a flam
void rolls(Score& s)
{
    s.strike(16.0, D3, 88);
    s.roll(17.0, 2.0, 4, {D4, F4}, 40, 92); // sixteenths, crescendo
    s.strike(19.0, D3, 70);
    s.strike(20.0, D3, 86);
    s.roll(21.0, 2.0, 4, {E4, G4}, 44, 96);
    s.strike(23.0, A3, 70);
    s.strike(24.0, D3, 90);
    s.roll(25.0, 1.0, 8, {A4, F4}, 50, 100); // thirty-seconds
    const int down[8] = {A4, G4, F4, E4, D4, C4, Bb3, A3};
    for (int i = 0; i < 8; ++i)
        s.strike(26.0 + 0.25 * i, down[i], 82 - 4 * i);
    s.strike(28.0, D3, 92);
    s.flam(29.0, D4, 80);
    s.flam(30.0, F4, 74);
    s.strike(31.0, A3, 60);
}

// bars 9-12: a phrase sung over a pulse of the ding on beats 1 and 3
void phrase(Score& s)
{
    for (int bar = 0; bar < 4; ++bar)
    {
        s.strike(32.0 + 4.0 * bar, D3, 72);
        s.strike(34.0 + 4.0 * bar, D3, 48);
    }
    const struct
    {
        double beat;
        int note;
    } melody[] = {{32.0, A4}, {33.0, G4}, {33.5, F4}, {34.5, E4}, {36.0, D4}, {37.0, E4}, {37.5, F4},
                  {38.0, E4}, {40.0, F4}, {40.5, E4}, {41.0, D4}, {41.5, C4}, {42.5, D4}, {44.0, A3},
                  {45.0, C4}, {45.5, D4}, {46.0, E4}, {47.0, D4}};
    for (const auto& m : melody)
        s.strike(m.beat + 0.02, m.note, 76); // just after the ding, as a singer's breath
}

// bars 13-16: a roll on the ding itself, the ding with its fifth, an arpeggio up, a last chord left to ring
void ending(Score& s)
{
    s.roll(48.0, 4.0, 4, {D3}, 30, 96);       // both hands on the ding, growing
    s.roll(52.0, 3.0, 4, {D3, A3}, 60, 100);
    s.strike(55.0, D4, 90);
    const int up[] = {D3, A3, D4, F4, A4, G4, E4, C4};
    for (int i = 0; i < 8; ++i)
        s.strike(56.0 + 0.25 * i, up[i], 70 + 3 * i);
    s.flam(58.0, D3, 100);
    s.strike(60.0, D3, 104);
    s.strike(60.0, D4, 84);
    s.strike(60.0, A4, 76);
}
} // namespace

int main(int argc, char** argv)
{
    std::string path = "handpan_melody.wav";
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--lap") == 0)
            settings.on_lap = true;
        else
            path = argv[i];

    constexpr int SAMPLE_RATE = 48000;
    Score score(84.0, 0.008, 6, 2611);
    opening(score);
    rolls(score);
    phrase(score);
    ending(score);

    const Model model = solve_instrument(settings, SAMPLE_RATE);
    std::vector<float> samples = OfflineRenderer::render(model.sound(), score.play(), score.seconds(61.0) + 6.0);
    float peak = 0.0f;
    for (const float s : samples)
        peak = std::max(peak, std::fabs(s));
    std::printf("peak %.1f dBFS\n", 20 * std::log10(peak));
    WavWriter::write(path, samples, SAMPLE_RATE);
    std::printf("wrote %s (%.1f s)\n", path.c_str(), samples.size() / double(SAMPLE_RATE));
    return 0;
}
