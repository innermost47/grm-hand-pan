// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The fields singing together, against [A15]: the exchange keeps the energy, the coupling solved from the energy the
// neighbours add and the onset of a coupled mode, and, measured by none of them, the beating of the coupled pairs.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/instrument.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
constexpr double RATE = 48000.0;

void check_exchange(test::TestReport& report)
{
    // two lossless modes that do not turn: only the exchange moves their energy
    FieldSound still;
    for (auto& mode : still.modes)
    {
        mode.rotation_re = 1.0;
        mode.strike_velocity.fill(1.0);
    }
    FieldVoice a, b;
    a.set_sound(&still);
    b.set_sound(&still);
    a.strike(64);
    const double angle = 1e-3;
    const int quarter = static_cast<int>(std::round(0.5 * PI / angle));
    double drift = 0.0;
    for (int i = 0; i < quarter; ++i)
    {
        FieldVoice::exchange(a, 0, b, 0, std::cos(angle), std::sin(angle));
        const double energy = a.amplitude(0) * a.amplitude(0) + b.amplitude(0) * b.amplitude(0);
        drift = std::max(drift, std::fabs(energy - 1.0));
    }
    report.expect(drift < 1e-9, "the exchange keeps the energy of the pair");
    report.expect(a.amplitude(0) < 1e-3 && std::fabs(b.amplitude(0) - 1.0) < 1e-6,
                  "after a quarter of the exchange, all the energy has changed field");
}

void check_coupling(test::TestReport& report)
{
    const Measured g = coupling_rate();
    const Measured measured = measured_sympathy_gain_db();
    const double gain = SympathyProbe().energy_gain(g.value);
    std::printf("  g %.2f 1/s (+/- %.0f %%): full handover in %.0f ms, D4 energy gain %.2f dB (measured %.1f +/- %.1f)\n",
                g.value, 100 * g.spread, 1e3 * PI / (2 * g.value), gain, measured.value, measured.spread);
    report.expect(std::fabs(gain - measured.value) < measured.spread,
                  "the neighbours add the energy [A15] measured, within its spread (Table 4.7)");
    const double handover = PI / (2.0 * g.value);
    report.expect(handover > SYMPATHY_COUPLED_ONSET - 2 * SYMPATHY_ONSET_RESOLUTION &&
                      handover < SYMPATHY_COUPLED_ONSET + SYMPATHY_ONSET_RESOLUTION,
                  "a coupled mode peaks when [A15] saw it (Table 4.4)");
}

void check_instrument(test::TestReport& report)
{
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    const Model model = solve_instrument(settings, RATE);
    const SoundModel& sound = model.sound();
    std::printf("  D Kurd: %d coupled pairs (at most %d)\n", sound.coupling_count, MAX_COUPLINGS);
    report.expect(sound.coupling_count > 0 && sound.coupling_count < MAX_COUPLINGS,
                  "a D Kurd's coupled pairs fit the audio thread");

    // the beating of the coupled pairs [A15] measured (Table 4.3: 3.1-8.6 Hz), tuned modes in near unison (a
    // fundamental against a neighbour's octave): their two normal modes split by sqrt((2g)^2 + dw^2)
    const double g = coupling_rate().value;
    std::vector<double> beats;
    for (int i = 0; i < sound.coupling_count; ++i)
    {
        const Coupling& c = sound.couplings[(size_t)i];
        const double fa = model.fields()[(size_t)c.field_a].modes[(size_t)c.mode_a].frequency.value;
        const double fb = model.fields()[(size_t)c.field_b].modes[(size_t)c.mode_b].frequency.value;
        if (c.mode_a >= TUNED_MODES || c.mode_b >= TUNED_MODES || std::fabs(fa - fb) > 0.5 * (fa + fb) * 0.03)
            continue; // not a unison of tuned modes: a neighbour that sings, but does not beat
        const double dw = 2.0 * PI * (fa - fb);
        beats.push_back(std::sqrt(4.0 * g * g + dw * dw) / (2.0 * PI));
    }
    if (!report.expect(!beats.empty(), "a D Kurd has tuned unisons that sing together"))
        return;
    std::sort(beats.begin(), beats.end());
    const double median = beats[beats.size() / 2];
    std::printf("  beating of the %zu tuned unisons: %.1f to %.1f Hz, median %.1f Hz\n", beats.size(), beats.front(),
                beats.back(), median);
    report.expect(median > 3.1 && median < 8.6, "the tuned unisons beat as [A15] heard them (Table 4.3)");
    report.expect(beats.front() >= g / PI - 1e-9, "no pair beats slower than the coupling alone allows (g / pi)");

    // struck alone, the ding's octave sets the D4 ringing
    Engine engine;
    engine.set_model(&sound);
    engine.strike(D_KURD[0], MIDI_VELOCITY_REFERENCE);
    std::vector<float> block(64);
    double loudest = 0.0, at = 0.0;
    for (int n = 0; n < (int)(RATE * 0.6) / 64; ++n)
    {
        engine.render(block.data(), 64);
        const double a = engine.amplitude(4, 0); // D4's fundamental
        if (a > loudest)
            loudest = a, at = (n + 1) * 64 / RATE;
    }
    std::printf("  D3 struck: the D4 fundamental peaks at %.0f ms\n", 1e3 * at);
    report.expect(loudest > 0.0 && at > 0.03, "the ding sets its octave's field ringing, over time");

    // the beatings of [A15], the coupling's own splitting taken out, leave about 1 % of mistuning
    std::printf("  mistuning: %.2f %% rms (%.2f %% before the coupling is taken out)\n", 100 * instrument_mistuning(),
                100 * mistuning_spread(0.0));
    report.expect(instrument_mistuning() < mistuning_spread(0.0) && instrument_mistuning() > 0.005,
                  "an instrument is mistuned as [A15]'s beatings say, once the coupling is taken out");
}
} // namespace

int main()
{
    test::TestReport report("sympathy");
    check_exchange(report);
    check_coupling(report);
    check_instrument(report);
    return report.finish();
}
