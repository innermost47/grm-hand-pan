// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The first sound: a D Kurd handpan played up and down its scale at the reference velocity, then the ding soft and
// loud, then a chord. Usage: render_scale [out.wav] [--lap]
#include <cstdio>
#include <string>
#include <vector>

#include "../core/instrument.hpp"
#include "../core/offline_renderer.hpp"
#include "render_support.hpp"

using namespace grmh;

int main(int argc, char** argv)
{
    std::string path;
    const Settings settings = d_kurd_from_arguments(argc, argv, path, "handpan_d_kurd.wav");

    constexpr int SAMPLE_RATE = 48000;
    const Model model = solve_instrument(settings, SAMPLE_RATE);
    for (size_t i = 0; i < model.fields().size(); ++i)
    {
        const ToneField& field = model.fields()[i];
        std::printf("note %d: %.1f / %.1f / %.1f Hz, T60 %.1f / %.1f / %.1f s, radius %.1f cm\n", settings.notes[i],
                    field.modes[0].frequency.value, field.modes[1].frequency.value, field.modes[2].frequency.value,
                    field.modes[0].t60.value, field.modes[1].t60.value, field.modes[2].t60.value,
                    100 * field.effective_radius.value);
    }
    std::printf("cavity: %.1f Hz, Q %.0f\n", model.cavity().frequency.value, model.cavity().quality_factor.value);

    std::vector<TimedStrike> strikes;
    double t = 0.5;
    for (const int note : settings.notes) // up the scale
        strikes.push_back({t += 0.5, note, MIDI_VELOCITY_REFERENCE});
    for (auto it = settings.notes.rbegin(); it != settings.notes.rend(); ++it) // and down
        strikes.push_back({t += 0.35, *it, MIDI_VELOCITY_REFERENCE});
    t += 2.5;
    for (const int velocity : {24, 64, 110}) // the ding, soft to loud
        strikes.push_back({t += 3.0, settings.notes.front(), velocity});
    t += 3.5;
    for (const size_t i : {0, 3, 5, 7}) // a chord: D3 C4 E4 G4
        strikes.push_back({t, settings.notes[i], 80});

    write_rendering(path, OfflineRenderer::render_stereo(model.sound(), strikes, t + 6.0), SAMPLE_RATE);
    return 0;
}
