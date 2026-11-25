// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The constants agree with the measurements they come from: the declared spreads cover the published data.
#include <cmath>
#include <string>

#include "../constants/core.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
// the root mean square of the relative deviation of measured ratios from the nominal one
template <class Ratio>
double rms_deviation(Ratio ratio_of, double nominal)
{
    double sum = 0.0;
    for (const auto& field : MEASURED_TONE_FIELDS)
    {
        const double deviation = ratio_of(field) / nominal - 1.0;
        sum += deviation * deviation;
    }
    return std::sqrt(sum / MEASURED_TONE_FIELDS.size());
}

void check_mode_ratios(test::TestReport& report)
{
    const auto octave = [](const MeasuredToneField& f) { return f.octave / f.fundamental; };
    const auto fifth = [](const MeasuredToneField& f) { return f.compound_fifth / f.fundamental; };
    report.expect(rms_deviation(octave, OCTAVE_RATIO) <= MODE_RATIO_SPREAD,
                  "the octave ratio spread covers the measured tone fields");
    report.expect(rms_deviation(fifth, COMPOUND_FIFTH_RATIO) <= MODE_RATIO_SPREAD,
                  "the compound fifth ratio spread covers the measured tone fields");
    for (const auto& field : MEASURED_TONE_FIELDS)
    {
        const std::string name = field.note;
        report.expect_near(octave(field), OCTAVE_RATIO, 3 * MODE_RATIO_SPREAD, name + ": octave within 3 sigma");
        report.expect_near(fifth(field), COMPOUND_FIFTH_RATIO, 3 * MODE_RATIO_SPREAD,
                           name + ": compound fifth within 3 sigma");
    }
    for (const double ratio : UNTUNED_MODE_RATIOS)
        report.expect(ratio > COMPOUND_FIFTH_RATIO * (1 + 3 * MODE_RATIO_SPREAD) && ratio < 16.0,
                      "an untuned mode lies above the tuned ones, around the third octave");
}

void check_decay(test::TestReport& report)
{
    for (const auto& group : DECAY_BY_REGISTER)
    {
        const std::string name = group.group;
        report.expect(group.min <= group.mean && group.mean <= group.max, name + ": mean T60 within its range");
        report.expect(DECAY_ALL.min <= group.min && group.max <= DECAY_ALL.max,
                      name + ": range within the range of all instruments");
    }
    report.expect(DECAY_BY_REGISTER[0].mean >= DECAY_BY_REGISTER[1].mean &&
                      DECAY_BY_REGISTER[1].mean >= DECAY_BY_REGISTER[2].mean,
                  "higher registers decay faster");
}

void check_material_and_shell(test::TestReport& report)
{
    report.expect(SHELL_THICKNESS.low() <= SHELL_THICKNESS_MIN * 1.0001 &&
                      SHELL_THICKNESS.high() >= SHELL_THICKNESS_MAX * 0.9999,
                  "the thickness spread covers the published range 0.91-0.95 mm");
    report.expect(STEEL_DENSITY.low() <= 7850.0 && STEEL_DENSITY.high() >= 8000.0 * 0.9999,
                  "the density spread covers EN 1991-1-1 (7850-8000 kg/m3)");
    report.expect(STEEL_YOUNG_MODULUS.low() <= 205.0e9 && STEEL_YOUNG_MODULUS.high() >= 210.0e9,
                  "the Young's modulus spread covers 205-210 GPa");
    report.expect_near(STEEL_POISSON_RATIO.value, 0.30, 1e-12, "Poisson's ratio of EN 1993-1-1");
}

void check_cavity(test::TestReport& report)
{
    const double f2 = 440.0 * std::pow(2.0, (41.0 - 69.0) / 12.0);
    report.expect_near(CAVITY_RESONANCE_INTEGRAL_HANG.value, f2, 0.03, "the measured cavity resonance is near F2");
    const double d2 = 440.0 * std::pow(2.0, (CAVITY_RESONANCE_TARGET_NOTE_MIDI - 69.0) / 12.0);
    report.expect_near(d2, 73.42, 1e-3, "the D2 target of PANArt");
}

void check_spreads(test::TestReport& report)
{
    for (const Measured* m : {&STEEL_YOUNG_MODULUS, &STEEL_POISSON_RATIO, &STEEL_DENSITY, &SHELL_THICKNESS,
                              &CAVITY_RESONANCE_INTEGRAL_HANG})
        report.expect(m->value > 0.0 && m->spread > 0.0 && m->spread < 0.5 && std::isfinite(m->sigma_dex()),
                      "a measured value is positive with a finite, honest spread");
}
} // namespace

int main()
{
    test::TestReport report("constants");
    check_mode_ratios(report);
    check_decay(report);
    check_material_and_shell(report);
    check_cavity(report);
    check_spreads(report);
    return report.finish();
}
