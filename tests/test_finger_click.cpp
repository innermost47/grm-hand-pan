// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The finger on the steel: each pulse lasts its contact and carries the strike's impulse, its pressure is Newton's
// over a baffle, heard above the field's modes only; a harder strike clicks shorter, louder and brighter, and a fast
// roll loses no strike.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/instrument.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
constexpr double RATE = 48000.0;

// the force of one pulse the listener hears, sample by sample, until it ends (no residual: the whole force)
std::vector<double> pulse(int length, double peak, const FieldSound* field = nullptr)
{
    FingerContacts contacts;
    contacts.press(length, peak, field ? &field->click_residual : nullptr, field ? field->click_tail : 0);
    std::vector<double> force;
    while (!contacts.idle())
        force.push_back(contacts.step());
    return force;
}

double level(const std::vector<double>& force, double frequency)
{
    double re = 0.0, im = 0.0;
    for (size_t n = 0; n < force.size(); ++n)
    {
        re += force[n] * std::cos(2.0 * PI * frequency * n / RATE);
        im -= force[n] * std::sin(2.0 * PI * frequency * n / RATE);
    }
    return 10.0 * std::log10(re * re + im * im + 1e-30);
}

double peak_of(const std::vector<double>& force)
{
    double p = 0.0;
    for (const double f : force)
        p = std::max(p, std::fabs(f));
    return p;
}

void check_pulse(test::TestReport& report, const ContactSound& contact)
{
    const int v = MIDI_VELOCITY_REFERENCE;
    const Strike& strike = strike_table()[(size_t)v];
    const auto force = pulse(contact.length[(size_t)v], contact.peak_force[(size_t)v]);
    double impulse = 0.0;
    for (const double f : force)
        impulse += f / RATE;
    const double rounding = 1.0 / (RATE * strike.contact_time.value); // the contact is a whole number of samples
    report.expect((int)force.size() == contact.length[(size_t)v], "the pulp pushes for exactly its contact");
    report.expect_near(force.size() / RATE, strike.contact_time.value, rounding, "the contact the strike network gives");
    report.expect_near(peak_of(force), strike.peak_force.value, 1e-3, "the pulse peaks at the strike's force");
    report.expect_near(impulse, strike.impulse.value, rounding, "the pulse carries the strike's impulse");
}

void check_click(test::TestReport& report, const SoundModel& sound, const Model& model)
{
    // p = rho0 F / (2 pi r rho_s), written out, times the click's weight the recordings ask for
    const double rho0 = ATMOSPHERIC_PRESSURE * M_AIR / (R_GAS * ROOM_TEMPERATURE);
    const double rho_s = STEEL_DENSITY.value * SHELL_THICKNESS.value;
    report.expect_near(sound.contact.click_gain * FULL_SCALE_PRESSURE,
                       participation().click.value * rho0 / (2.0 * PI * LISTENING_DISTANCE * rho_s), 1e-9,
                       "the sheet's volume accelerates by F / rho_s and radiates over a baffle");

    // below the field's modes, the click adds nothing the modes do not already play
    const size_t d4 = 4;
    const FieldSound& field = sound.fields[d4];
    const double f1 = model.fields()[d4].modes[0].frequency.value;
    const int v = MIDI_VELOCITY_REFERENCE;
    const auto whole = pulse(sound.contact.length[v], sound.contact.peak_force[v]);
    const auto residual = pulse(sound.contact.length[v], sound.contact.peak_force[v], &field);
    report.expect(level(whole, f1) - level(residual, f1) > 30.0,
                  "at the fundamental, the click is left to the modes (more than 30 dB down)");

    const auto spl = [&](int velocity)
    {
        const auto r = pulse(sound.contact.length[(size_t)velocity], sound.contact.peak_force[(size_t)velocity], &field);
        return 20.0 * std::log10(sound.contact.click_gain * peak_of(r) * FULL_SCALE_PRESSURE / 20e-6);
    };
    std::printf("  D4 click at 1 m, above the modes: %.1f dB SPL soft (20), %.1f reference (64), %.1f loud (120)\n",
                spl(20), spl(v), spl(120));

    // brighter: the tone grows as the impulse, the click above the modes far faster (a shorter, harder push)
    const int soft = 20, loud = 120;
    const double tone_growth = 20.0 * std::log10(strike_table()[loud].impulse.value / strike_table()[soft].impulse.value);
    std::printf("  contact %.1f ms soft, %.1f ms loud; from soft to loud the tone grows %.1f dB, the click %.1f dB\n",
                1e3 * sound.contact.length[soft] / RATE, 1e3 * sound.contact.length[loud] / RATE, tone_growth,
                spl(loud) - spl(soft));
    report.expect(sound.contact.length[loud] < sound.contact.length[soft], "a harder strike clicks shorter");
    report.expect(spl(loud) - spl(soft) > tone_growth + 10.0,
                  "a harder strike is brighter: its click grows far faster than its tone");
}

void check_roll(test::TestReport& report, const SoundModel& sound)
{
    // a roll faster than any hand: forty strikes a millisecond apart, all heard, none lost to a full pool
    const FieldSound& field = sound.fields[4];
    FingerContacts contacts;
    for (int i = 0; i < 40; ++i)
    {
        contacts.press(sound.contact.length[100], sound.contact.peak_force[100], &field.click_residual,
                       field.click_tail);
        for (int n = 0; n < (int)(RATE / 1000); ++n)
            contacts.step();
    }
    report.expect(!contacts.idle(), "the last strikes of a fast roll still push");
}
} // namespace

int main()
{
    test::TestReport report("finger click");
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    const Model model = solve_instrument(settings, RATE);
    check_pulse(report, model.sound().contact);
    check_click(report, model.sound(), model);
    check_roll(report, model.sound());
    return report.finish();
}
