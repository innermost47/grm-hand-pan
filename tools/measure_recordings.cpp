// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Measures recorded handpan notes against the model, with the same meter: for each note, the levels of its octave,
// compound fifth and strongest untuned mode against the fundamental; then the same for the model's reference tap.
// Usage: measure_recordings <folder of WAV files named after their note, e.g. D3.wav> <note> [<note>...]
#include <cstdio>
#include <string>

#include "../core/mode_levels.hpp"
#include "../core/instrument.hpp"
#include "../core/note_names.hpp"
#include "../core/offline_renderer.hpp"
#include "../core/wav_reader.hpp"

using namespace grmh;

namespace
{
void print(const char* label, const ModeLevels& l)
{
    std::printf("  %-9s f1 %7.1f Hz | octave %6.1f dB | fifth %6.1f dB | untuned %6.1f dB (floor %6.1f) | attack %6.1f dB\n",
                label, l.fundamental_frequency, l.octave, l.fifth, l.untuned, l.untuned_band_floor, l.attack);
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: measure_recordings <folder> <note> [<note>...]\n");
        return 1;
    }
    const std::string folder = argv[1];
    for (int i = 2; i < argc; ++i)
    {
        const std::string name = argv[i];
        const int note = midi_of_name(name);
        const double nominal = Model::note_frequency(note);
        std::printf("%s (MIDI %d, %.1f Hz)\n", name.c_str(), note, nominal);

        const WavData recording = WavReader::read(folder + "/" + name + ".wav");
        print("recording", ModeLevelMeter::measure(recording.samples, recording.sample_rate, nominal));

        Settings settings;
        settings.notes = {note};
        const Model model = solve_instrument(settings, recording.sample_rate);
        const auto rendered = OfflineRenderer::render(model.sound(), {{0.0, note, MIDI_VELOCITY_REFERENCE}}, 1.0);
        print("model", ModeLevelMeter::measure(rendered, recording.sample_rate, nominal));
    }
    return 0;
}
