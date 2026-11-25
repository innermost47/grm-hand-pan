// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The cavity network against acoustics [KF00] and PANArt's measurements [WMR08, PB]: the speed of sound, the
// Helmholtz and radiation formulas written out, the volume of the published vessel, and the lap lowering the air
// mode by lengthening the neck.
#include <cmath>
#include <cstdio>

#include "../core/cavity_network.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
void check_free_cavity(test::TestReport& report, const Cavity& cavity)
{
    std::printf("  air mode %.1f Hz, V %.1f L (+/- %.0f %%), S/L' %.1f mm, Q %.0f (+/- %.0f %%), T60 %.2f s\n",
                cavity.frequency.value, 1e3 * cavity.volume.value, 100 * cavity.volume.spread,
                1e3 * cavity.neck_conductance.value, cavity.quality_factor.value, 100 * cavity.quality_factor.spread,
                cavity.t60.value);

    report.expect_near(cavity.speed_of_sound, 343.0, 0.005, "the speed of sound in air at 20 degrees C [KF00]");

    // fH = c / (2 pi) sqrt(S / (V L')), written out
    const double c = cavity.speed_of_sound, V = cavity.volume.value, K = cavity.neck_conductance.value;
    report.expect_near(c / (2 * PI) * std::sqrt(K / V), CAVITY_RESONANCE_INTEGRAL_HANG.value, 1e-6,
                       "the Helmholtz formula gives the measured air mode");

    // the volume of two spherical caps on the ding shell, a shape the bounds must hold
    const double a = DING_SHELL_DIAMETER / 2, cap = HANG_HEIGHT / 2;
    const double caps = 2 * PI * cap * (3 * a * a + cap * cap) / 6;
    report.expect(caps >= cavity.volume.low() && caps <= cavity.volume.high(),
                  "two spherical caps lie within one sigma of the volume");
    const double base = DING_SHELL_DIAMETER * DING_SHELL_DIAMETER * HANG_HEIGHT;
    report.expect(V > CAVITY_SHAPE_CONES * base && V < CAVITY_SHAPE_ELLIPSOIDS * base,
                  "the volume lies between two cones and two half ellipsoids");

    // the neck radiates between a flanged and an unflanged opening [KF00]
    const double radiation = std::sqrt(V / (K * K * K));
    report.expect(cavity.quality_factor.value > NECK_RADIATION_FLANGED * radiation &&
                      cavity.quality_factor.value < NECK_RADIATION_UNFLANGED * radiation,
                  "Q lies between the flanged and unflanged radiation of the neck");
    report.expect_near(cavity.t60.value,
                       std::log(1000.0) * cavity.quality_factor.value / (PI * cavity.frequency.value), 1e-6,
                       "T60 is the time the energy of a mode of that Q takes to fall 60 dB");
}

void check_lap(test::TestReport& report, const Cavity& free, const Cavity& lap)
{
    report.expect_near(lap.volume.value, free.volume.value, 1e-6, "the lap does not change the cavity");
    report.expect_near(lap.neck_conductance.value / free.neck_conductance.value,
                       std::pow(CAVITY_RESONANCE_ON_LAP.value / CAVITY_RESONANCE_INTEGRAL_HANG.value, 2.0), 1e-6,
                       "on the lap, the air mode falls as the square root of the neck's conductance");
    report.expect(lap.neck_conductance.value < free.neck_conductance.value, "the knees lengthen the neck [MR07]");
    const double d2 = 440.0 * std::pow(2.0, (CAVITY_RESONANCE_TARGET_NOTE_MIDI - 69.0) / 12.0);
    report.expect_near(lap.frequency.value, d2, 0.01, "on the lap, the air mode reaches the ding's octave (D2) [PB]");
}
} // namespace

int main()
{
    test::TestReport report("cavity");
    const Cavity free = solve_cavity(CAVITY_RESONANCE_INTEGRAL_HANG);
    const Cavity lap = solve_cavity(CAVITY_RESONANCE_ON_LAP);
    check_free_cavity(report, free);
    check_lap(report, free, lap);
    return report.finish();
}
