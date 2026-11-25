// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The participation of the modes against the recorded notes [MUMT22]: the calibrated instrument, measured with the
// meter of the recordings, sounds its octave, fifth and untuned modes as loud as the recordings, within the spread
// from field to field [A15].
#include <cmath>
#include <cstdio>
#include <string>

#include "../core/instrument.hpp"
#include "support/test_report.hpp"

using namespace grmh;

int main()
{
    test::TestReport report("participation");
    const Participation& p = participation();
    const auto db = [](const Measured& m) { return 20.0 * std::log10(m.value); };
    std::printf("  participation: octave %.1f dB, fifth %.1f dB, untuned %.1f dB (+/- %.1f dB each)\n", db(p.octave),
                db(p.fifth), db(p.untuned), 20.0 * p.untuned.sigma_dex());
    report.expect(db(p.untuned) < -20.0 && db(p.untuned) + 20.0 * p.untuned.sigma_dex() < 0.0,
                  "a strike at the sweet spot hardly excites the untuned modes (recordings)");

    double gaps[3] = {0.0, 0.0, 0.0};
    for (const RecordedNote& r : RECORDED_NOTES)
    {
        Settings settings;
        settings.notes = {r.note};
        const Model model = solve_instrument(settings, CALIBRATION_SAMPLE_RATE);
        const auto rendered = OfflineRenderer::render(model.sound(), {{0.0, r.note, MIDI_VELOCITY_REFERENCE}}, 1.0);
        const ModeLevels l = ModeLevelMeter::measure(rendered, CALIBRATION_SAMPLE_RATE, Model::note_frequency(r.note));
        const double note_gaps[3] = {r.octave - l.octave, r.fifth - l.fifth, r.untuned - l.untuned};
        std::printf("  %s: recording - model = %5.1f / %5.1f / %5.1f dB\n", r.name, note_gaps[0], note_gaps[1],
                    note_gaps[2]);
        for (int k = 0; k < 3; ++k)
        {
            gaps[k] += note_gaps[k] / RECORDED_NOTES.size();
            report.expect(std::fabs(note_gaps[k]) < 2.5 * FIELD_LEVEL_SPREAD_DB,
                          std::string(r.name) + ": within the spread from field to field (" +
                              std::to_string((int)note_gaps[k]) + " dB)");
        }
    }
    const char* kinds[3] = {"octave", "fifth", "untuned modes"};
    for (int k = 0; k < 3; ++k)
        report.expect(std::fabs(gaps[k]) < 1.0, std::string("on average, the ") + kinds[k] +
                                                    " sound as loud as recorded (" + std::to_string(gaps[k]) + " dB)");
    return report.finish();
}
