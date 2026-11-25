// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The radiation of a mode against the compact multipole limits [KF00] and the incoherent limit of its parts.
#include <cmath>
#include <cstdio>

#include "../core/handpan_networks.hpp"
#include "../core/radiation.hpp"
#include "support/test_report.hpp"

using namespace grmh;

int main()
{
    test::TestReport report("radiation");
    const double a = 0.075, d = part_spacing(a), small = 0.01 / d; // k such that kd = 0.01

    report.expect_near(radiation_factor(0, 10.0, a), 1.0, 1e-12, "a monopole radiates as itself");
    // dipole of two half discs: (A / 2) kd cos(theta) against A, rms of cos over the sphere 1 / sqrt(3)
    report.expect_near(radiation_factor(1, small, a), 0.01 / (2.0 * std::sqrt(3.0)), 1e-3,
                       "a compact dipole radiates kd / (2 sqrt 3) of a monopole");
    // lateral quadrupole of four quarter discs: (A / 4) (kd)^2 ux uy, rms of ux uy over the sphere 1 / sqrt(15)
    report.expect_near(radiation_factor(2, small, a), 1e-4 / (4.0 * std::sqrt(15.0)), 1e-3,
                       "a compact quadrupole radiates (kd)^2 / (4 sqrt 15) of a monopole");
    // far from compact (kd = 40, ten times what a handpan reaches), within the 1 / kd the limit is approached by
    report.expect_near(radiation_factor(1, 40.0 / d, a), 1.0 / std::sqrt(2.0), 0.05,
                       "far from compact, the two halves radiate on their own");
    report.expect_near(radiation_factor(2, 40.0 / d, a), 0.5, 0.05,
                       "far from compact, the four quarters radiate on their own");

    // the compound fifth of a D4 field, a dipole [MR07]: weaker than a monopole, less so higher up
    const ToneField d4 = solve_tone_field(293.66), a4 = solve_tone_field(440.0);
    const auto fifth = [](const ToneField& field)
    {
        const double k = 2.0 * PI * field.modes[2].frequency.value / 343.0;
        return 20.0 * std::log10(radiation_factor(field.modes[2].multipole_order, k, field.effective_radius.value));
    };
    std::printf("  compound fifth, as a dipole: D4 %.1f dB, A4 %.1f dB against a monopole\n", fifth(d4), fifth(a4));
    report.expect(fifth(d4) < -3.0, "the compound fifth radiates less than a monopole [MR07]");
    report.expect(fifth(a4) > fifth(d4), "the higher the note, the more its fifth radiates (kd grows as sqrt(f))");
    return report.finish();
}
