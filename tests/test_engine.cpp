// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The sound path, rendered offline: the partials of a struck field at the model's frequencies, their decay at the
// model's T60, the cavity's air mode answering the ding, a level in the range of the instrument, and the mailbox
// that hands models to the audio thread.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/mailbox.hpp"
#include "../core/model.hpp"
#include "../core/offline_renderer.hpp"
#include "support/spectrum_probe.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
constexpr double RATE = 48000.0;

Settings d_kurd()
{
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    return settings;
}

void check_partials(test::TestReport& report, const Model& model, size_t field_index)
{
    const ToneField& field = model.fields()[field_index];
    const int note = model.sound().fields[field_index].note;
    const auto samples = OfflineRenderer::render(model.sound(), {{0.0, note, MIDI_VELOCITY_REFERENCE}}, 4.0);
    const test::SpectrumProbe probe(samples, RATE);
    const std::string name = "note " + std::to_string(note);

    for (size_t m = 0; m < field.modes.size(); ++m)
    {
        const double f = field.modes[m].frequency.value;
        // between two partials, a quarter of the way to the next harmonic of the fundamental
        const double between = f + 0.25 * field.modes[0].frequency.value;
        const double contrast = probe.level(f, 0.1, 1.0) - probe.level(between, 0.1, 1.0);
        report.expect(contrast > 20.0, name + ": partial " + std::to_string(m + 1) + " stands at the model's " +
                                           std::to_string((int)f) + " Hz (" + std::to_string((int)contrast) + " dB)");
    }

    // the fundamental falls 60 dB in its T60: half of it in half of it
    const double t60 = field.modes[0].t60.value, f1 = field.modes[0].frequency.value;
    const double drop = probe.level(f1, 0.1, 0.4) - probe.level(f1, 0.1 + t60 / 2, 0.4);
    report.expect_near(drop, 30.0, 0.1, name + ": the fundamental decays at the model's T60");
}

void check_cavity_and_level(test::TestReport& report, const Model& model)
{
    const int ding = model.sound().fields[0].note;
    const auto samples = OfflineRenderer::render(model.sound(), {{0.0, ding, MIDI_VELOCITY_REFERENCE}}, 4.0);
    const test::SpectrumProbe probe(samples, RATE);
    report.expect(probe.finite(), "the sound is finite");

    const double air = model.cavity().frequency.value;
    const double contrast = probe.level(air, 0.2, 2.0) - probe.level(0.7 * air, 0.2, 2.0);
    std::printf("  air mode at %.1f Hz: %.1f dB above its surroundings\n", air, contrast);
    report.expect(contrast > 10.0, "the ding sets the cavity's air mode ringing");

    // the pressure at 1 m of the reference tap on the ding, against the level PANArt gives its instruments ("a
    // volume of just over 70 dB", [PB]): within the range of a voice, 60-90 dB SPL
    const double spl = 20.0 * std::log10(probe.peak() * FULL_SCALE_PRESSURE / 20e-6);
    std::printf("  reference tap on the ding: %.1f dB SPL peak at %.0f m (%.1f dBFS)\n", spl, LISTENING_DISTANCE,
                20.0 * std::log10(probe.peak()));
    report.expect(spl > 60.0 && spl < 90.0, "a tap sounds as loud as a handpan [PB]");
    report.expect(probe.peak() < 1.0f, "a reference tap does not clip");

    const auto loud = OfflineRenderer::render(model.sound(), {{0.0, ding, 120}}, 1.0);
    const auto soft = OfflineRenderer::render(model.sound(), {{0.0, ding, 20}}, 1.0);
    report.expect(test::SpectrumProbe(loud, RATE).peak() > test::SpectrumProbe(soft, RATE).peak(),
                  "a harder strike is louder");

    const auto stranger = OfflineRenderer::render(model.sound(), {{0.0, 61, 100}}, 0.5);
    report.expect(test::SpectrumProbe(stranger, RATE).peak() == 0.0f, "a note no field is tuned to is silent");
}

// counts the objects alive, to see which the mailbox frees
struct Counted
{
    static int alive;
    Counted() { ++alive; }
    ~Counted() { --alive; }
};
int Counted::alive = 0;

void check_mailbox(test::TestReport& report)
{
    {
        Mailbox<Counted> box;
        report.expect(box.take() == nullptr, "an empty mailbox gives nothing");
        auto* first = new Counted;
        box.post(first);
        report.expect(box.take() == first, "the audio thread takes what the solver posted");
        auto* second = new Counted;
        box.post(second);
        report.expect(box.take() == second && Counted::alive == 2,
                      "a newer object replaces the current one, which waits in the trash");
        box.collect();
        report.expect(Counted::alive == 1, "the solver frees the trash, never the audio thread");
        box.post(new Counted);
        box.post(new Counted);
        report.expect(Counted::alive == 2, "an object posted and never taken is freed by the next post");
    }
    report.expect(Counted::alive == 0, "the mailbox frees everything it holds");
}
} // namespace

int main()
{
    test::TestReport report("engine");
    const Model model = Model::solve(d_kurd(), RATE);
    check_partials(report, model, 0);                       // the ding, D3
    check_partials(report, model, model.fields().size() - 1); // the highest field, A4
    check_cavity_and_level(report, model);
    check_mailbox(report);

    Engine idle;
    float block[64];
    idle.render(block, 64);
    report.expect(block[0] == 0.0f && block[63] == 0.0f, "an engine without a model is silent");
    return report.finish();
}
