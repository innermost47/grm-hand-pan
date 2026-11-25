// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The first sound: a D Kurd handpan played up and down its scale at the reference velocity, then the ding soft and
// loud, then a chord. Usage: render_scale [out.wav] [--lap]
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/model.hpp"
#include "../core/offline_renderer.hpp"
#include "../core/wav_writer.hpp"

using namespace grmh;

int main(int argc, char** argv)
{
    std::string path = "handpan_d_kurd.wav";
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--lap") == 0)
            settings.on_lap = true;
        else
            path = argv[i];

    constexpr int SAMPLE_RATE = 48000;
    const Model model = Model::solve(settings, SAMPLE_RATE);
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

    std::vector<float> samples = OfflineRenderer::render(model.sound(), strikes, t + 6.0);
    float peak = 0.0f;
    for (const float s : samples)
        peak = std::max(peak, std::fabs(s));
    std::printf("peak %.1f dBFS (full scale %.0f dB SPL at %.0f m)\n", 20 * std::log10(peak),
                20 * std::log10(FULL_SCALE_PRESSURE / 20e-6), LISTENING_DISTANCE);
    WavWriter::write(path, samples, SAMPLE_RATE);
    std::printf("wrote %s (%.1f s)\n", path.c_str(), samples.size() / double(SAMPLE_RATE));
    return 0;
}
