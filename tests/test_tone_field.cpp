// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The tone field networks against the literature: the plate's frequency parameters, the loss law against the
// measured decays [A15], the tuned modes against the measured fields [MR07, WMR08].
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

#include "../core/handpan_networks.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
double midi_to_hz(double midi) { return 440.0 * std::pow(2.0, (midi - 69.0) / 12.0); }

// the first root of f on [lo, hi], by bisection
double first_root(const std::function<double(double)>& f, double lo, double hi)
{
    for (int i = 0; i < 200; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        (f(lo) * f(mid) <= 0.0 ? hi : lo) = mid;
    }
    return 0.5 * (lo + hi);
}

// the frequency equations of a circular plate [L69]: clamped J0 I1 + I0 J1 = 0; simply supported
// J1/J0 + I1/I0 = 2 lambda / (1 - nu)
void check_plate_parameters(test::TestReport& report)
{
    const auto clamped = [](double x)
    { return std::cyl_bessel_j(0.0, x) * std::cyl_bessel_i(1.0, x) + std::cyl_bessel_i(0.0, x) * std::cyl_bessel_j(1.0, x); };
    const double nu = STEEL_POISSON_RATIO.value;
    const auto simply_supported = [nu](double x)
    {
        return std::cyl_bessel_j(1.0, x) / std::cyl_bessel_j(0.0, x) +
               std::cyl_bessel_i(1.0, x) / std::cyl_bessel_i(0.0, x) - 2.0 * x / (1.0 - nu);
    };
    const double lc = first_root(clamped, 2.5, 3.8), ls = first_root(simply_supported, 1.5, 2.3);
    report.expect_near(lc * lc, PLATE_LAMBDA2_CLAMPED, 1e-4, "clamped plate: lambda^2 solves its frequency equation");
    report.expect_near(ls * ls, PLATE_LAMBDA2_SIMPLY_SUPPORTED, 1e-3,
                       "simply supported plate (nu = 0.3): lambda^2 solves its frequency equation");
}

void check_loss_law(test::TestReport& report)
{
    const LossLaw& law = loss_law();
    for (int m = 0; m < TUNED_MODES; ++m)
        std::printf("  T60 at %.0f Hz, mode %d: %.2f s (+/- %.0f %%)\n", law.reference_frequency, m + 1,
                    law.t60_at_reference[(size_t)m].value, 100 * law.t60_at_reference[(size_t)m].spread);
    std::printf("  slope %.3f +/- %.3f\n", law.slope, law.slope_sigma);

    // the 84 decays give the sign at about 1.7 sigma only; the register means below are the real check
    report.expect(law.slope < 0.0, "higher fields ring shorter (slope < 0)");
    report.expect(law.t60_at_reference[1].value > law.t60_at_reference[0].value,
                  "the octave rings longer than the fundamental");

    // the law against the register means of [A15] Table 4.2: the mean predicted T60 of the three modes of the fields
    // measured in a register, within the register's standard deviation
    const double register_bounds[3][2] = {{midi_to_hz(47), midi_to_hz(59)}, {midi_to_hz(60), midi_to_hz(64)},
                                          {midi_to_hz(65), midi_to_hz(72)}};
    for (size_t g = 0; g < DECAY_BY_REGISTER.size(); ++g)
    {
        double sum = 0.0;
        int count = 0;
        for (const auto& m : DECAY_MEASUREMENTS)
        {
            const double f1 = m.frequency / m.mode;
            if (f1 < register_bounds[g][0] * 0.98 || f1 > register_bounds[g][1] * 1.02)
                continue;
            sum += law.t60_at_reference[(size_t)m.mode - 1].value *
                   std::pow(f1 / law.reference_frequency, law.slope);
            ++count;
        }
        const auto& group = DECAY_BY_REGISTER[g];
        report.expect_near(sum / count, group.mean, group.standard_deviation / group.mean,
                           std::string("register ") + group.group + ": mean T60 of the law");
    }
}

void check_tone_fields(test::TestReport& report)
{
    for (const auto& measured : MEASURED_TONE_FIELDS)
    {
        const ToneField field = solve_tone_field(measured.fundamental);
        const std::string name = measured.note;
        const double measured_modes[TUNED_MODES] = {measured.fundamental, measured.octave, measured.compound_fifth};
        for (int m = 1; m < TUNED_MODES; ++m)
        {
            const Measured& f = field.modes[(size_t)m].frequency;
            report.expect_near(f.value, measured_modes[m], 3.0 * f.spread,
                               name + ": mode " + std::to_string(m + 1) + " within 3 sigma of the measurement");
        }
    }

    const ToneField d3 = solve_tone_field(midi_to_hz(50)), d4 = solve_tone_field(midi_to_hz(62));
    std::printf("  D3 field: radius %.1f cm (+/- %.0f %%); D4 field: radius %.1f cm\n",
                100 * d3.effective_radius.value, 100 * d3.effective_radius.spread, 100 * d4.effective_radius.value);
    report.expect_near(d3.effective_radius.value / d4.effective_radius.value, std::sqrt(2.0), 1e-6,
                       "an octave lower, the plate is sqrt(2) wider");
    report.expect(2.0 * d3.effective_radius.value < DING_SHELL_DIAMETER, "the D3 field fits in the ding shell");
    report.expect_near(d3.modes[0].frequency.value, midi_to_hz(50), 1e-6, "the field rings at its note");

    for (const ToneField* field : {&d3, &d4})
        for (const Mode& mode : field->modes)
            report.expect(mode.t60.value >= DECAY_ALL.min && mode.t60.value <= DECAY_ALL.max,
                          "a mode's T60 lies within the measured range (" + std::to_string(mode.t60.value) + " s)");
}

void check_untuned_modes(test::TestReport& report)
{
    const ToneField g3 = solve_tone_field(189.0);
    report.expect(g3.modes.size() == (size_t)FIELD_MODES, "a field plays its tuned and untuned modes");
    report.expect_near(g3.modes[3].frequency.value, 1418.0, 3.0 * g3.modes[3].frequency.spread,
                       "the G3 field's (2,1)a where [MR07] measured it");
    report.expect_near(g3.modes[4].frequency.value, 1543.0, 3.0 * g3.modes[4].frequency.spread,
                       "the G3 field's (2,1)b where [MR07] measured it");
    report.expect(g3.modes[2].multipole_order == 1 && g3.modes[3].multipole_order == 2,
                  "the compound fifth is a dipole, the (2,1) modes quadrupoles [MR07]");

    // the untuned modes' T60 extrapolate the loss law above the tuned modes: [A15] measured peaks at four times their
    // field's fundamental (0.6-1.3 kHz); each must lie within two field spreads of what the law gives that field
    const double field_spread_dex = DECAY_ALL.standard_deviation / DECAY_ALL.mean / grm::LN10;
    for (const auto& peak : HIGH_PEAK_DECAYS)
    {
        const ToneField field = solve_tone_field(peak.frequency / peak.mode);
        const double predicted = field.modes[3].t60.value;
        report.expect(std::fabs(std::log10(peak.t60 / predicted)) < 2.0 * field_spread_dex,
                      "a peak at " + std::to_string((int)peak.frequency) + " Hz decays as the law says (" +
                          std::to_string(peak.t60) + " s measured, " + std::to_string(predicted) + " s)");
    }
}
} // namespace

int main()
{
    test::TestReport report("tone field");
    check_plate_parameters(report);
    check_loss_law(report);
    check_tone_fields(report);
    check_untuned_modes(report);
    return report.finish();
}
