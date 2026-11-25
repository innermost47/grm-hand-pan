// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The strike network against the finger's measurements [HH97, J03, K06] and the spectrum of a half-sine pulse.
#include <cmath>
#include <cstdio>

#include "../core/handpan_networks.hpp"
#include "../core/strike_network.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
void check_reference_tap(test::TestReport& report)
{
    const Strike s = solve_strike(TAP_VELOCITY_REFERENCE);
    std::printf("  tap at %.2f m/s: contact %.2f ms (+/- %.0f %%), impulse %.2f mN s, peak %.1f N, pulp %.0f N/m, e %.2f\n",
                s.velocity, 1e3 * s.contact_time.value, 100 * s.contact_time.spread, 1e3 * s.impulse.value,
                s.peak_force.value, s.pulp_stiffness.value, s.restitution.value);

    report.expect(s.contact_time.value > PULP_CONTACT_MIN && s.contact_time.value < PULP_CONTACT_MAX,
                  "the contact lies within the pulp's measured transients [J03]");
    // the restitution is weighed against the other measurements: it must agree with [J03] within its uncertainty
    report.expect_near(s.restitution.value, std::sqrt(1.0 - PULP_ENERGY_DISSIPATED), s.restitution.spread,
                       "the pulp gives back the 19 % of the energy it does not dissipate [J03]");
    report.expect_near(s.pulp_stiffness.value, s.pulp_hardening.value * s.peak_force.value, 1e-6,
                       "the pulp is as stiff as it is loaded, k = b F [D16, HH97]");
    // a hardening pulp: tc = pi sqrt(m / k), k = b F, F = pi J / (2 tc), J = m v (1 + e) give tc = 2 pi / (b v (1 + e))
    report.expect_near(s.contact_time.value,
                       2.0 * PI / (s.pulp_hardening.value * s.velocity * (1.0 + s.restitution.value)), 1e-6,
                       "the contact of a hardening pulp, written out");
    const double step = std::log(PULP_FORCE_RATIO) / s.pulp_hardening.value;
    report.expect(step > PULP_DEFLECTION_STEP_MIN * 0.9 && step < PULP_DEFLECTION_STEP_MAX * 1.1,
                  "the pulp hardening agrees with the deflections measured by MRI [D16]");
    // the forces [HH97] measured the finger at are a range of experiments, not a law: the tap's peak force, within its
    // uncertainty, must meet that range
    const double force_high = s.peak_force.value * std::pow(10.0, s.peak_force.sigma_dex());
    const double force_low = s.peak_force.value / std::pow(10.0, s.peak_force.sigma_dex());
    std::printf("  peak force %.2f N, one sigma %.2f-%.2f N\n", s.peak_force.value, force_low, force_high);
    report.expect(force_high >= FINGERTIP_FORCE_MIN && force_low <= FINGERTIP_FORCE_MAX,
                  "a tap's peak force meets the fingertip forces measured by [HH97]");

    const Strike twice = solve_strike(2.0 * TAP_VELOCITY_REFERENCE);
    report.expect_near(twice.impulse.value / s.impulse.value, 2.0, 1e-6, "twice as fast, twice the impulse");
    report.expect_near(twice.contact_time.value / s.contact_time.value, 0.5, 1e-6,
                       "twice as fast, the pulp hardens and the contact halves");
}

void check_spectrum(test::TestReport& report)
{
    const double tc = 2e-3;
    report.expect_near(strike_spectrum(0.0, tc), 1.0, 1e-12, "the pulse's spectrum starts at its impulse");
    report.expect(strike_spectrum(1.5 / tc, tc) < 1e-9, "a half-sine pulse has its first zero at 1.5 / tc");
    report.expect_near(strike_spectrum(0.5 / tc, tc), PI / 4.0, 1e-9, "and is continuous at 2 f tc = 1");
    bool falling = true;
    for (double f = 10.0; f < 1.5 / tc; f += 10.0)
        falling = falling && strike_spectrum(f, tc) <= strike_spectrum(f - 10.0, tc) + 1e-12;
    report.expect(falling, "below its first zero, the pulse's spectrum only falls");

    // a long contact silences the high modes: the octave and compound fifth of a D4 field against its fundamental,
    // for strikes whose contact is known within the reference tap's uncertainty
    const ToneField d4 = solve_tone_field(440.0 * std::pow(2.0, (62.0 - 69.0) / 12.0));
    const double spread = solve_strike(TAP_VELOCITY_REFERENCE).contact_time.spread;
    const auto level = [&](size_t mode, double contact)
    {
        const Measured tc{contact, spread};
        return 20.0 * std::log10(expected_strike_spectrum(d4.modes[mode].frequency.value, tc) /
                                 expected_strike_spectrum(d4.modes[0].frequency.value, tc));
    };
    std::printf("  D4, octave / fifth against the fundamental: %.1f / %.1f dB at 1 ms, %.1f / %.1f dB at 5 ms\n",
                level(1, PULP_CONTACT_MIN), level(2, PULP_CONTACT_MIN), level(1, PULP_CONTACT_MAX),
                level(2, PULP_CONTACT_MAX));
    report.expect(level(1, PULP_CONTACT_MAX) < level(1, PULP_CONTACT_MIN) &&
                      level(2, PULP_CONTACT_MAX) < level(2, PULP_CONTACT_MIN),
                  "a long contact silences the octave and the fifth");
    report.expect(level(1, PULP_CONTACT_MIN) < 0.0 && level(2, PULP_CONTACT_MIN) < level(1, PULP_CONTACT_MIN),
                  "the higher the mode, the less a strike excites it");
    report.expect_near(expected_strike_spectrum(0.0, {tc, spread}), 1.0, 1e-9,
                       "averaged over its length, the pulse still starts at its impulse");

    // a harder strike is brighter [WMR08]: the octave of a D4 field, soft and loud
    const auto octave = [&](double velocity)
    {
        const Measured contact = solve_strike(velocity).contact_time;
        return 20.0 * std::log10(expected_strike_spectrum(d4.modes[1].frequency.value, contact) /
                                 expected_strike_spectrum(d4.modes[0].frequency.value, contact));
    };
    const double soft = octave(0.5 * TAP_VELOCITY_REFERENCE), loud = octave(2.0 * TAP_VELOCITY_REFERENCE);
    std::printf("  D4 octave against the fundamental: %.1f dB soft, %.1f dB loud\n", soft, loud);
    report.expect(loud > soft, "a harder strike is brighter [WMR08]");
}
} // namespace

int main()
{
    test::TestReport report("strike");
    check_reference_tap(report);
    check_spectrum(report);
    return report.finish();
}
