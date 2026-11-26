// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Where the fields sit and sound: the D Kurd laid out as Anthony plays it (the ding in the middle, A3 in front, then
// left, right, up the scale, A4 at the back), the stereo gains at constant power, and a field on the right heard on the
// right.
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/instrument.hpp"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
double energy(const std::vector<float>& s)
{
    return std::accumulate(s.begin(), s.end(), 0.0, [](double e, float x) { return e + double(x) * x; });
}

void check_positions(test::TestReport& report)
{
    const std::vector<int> notes(D_KURD.begin(), D_KURD.end());
    const auto positions = field_positions(notes);
    // D3, A3, Bb3, C4, D4, E4, F4, G4, A4
    const double expected[9] = {0.0, 0.0, -std::sqrt(0.5), std::sqrt(0.5), -1.0, 1.0, -std::sqrt(0.5),
                                std::sqrt(0.5), 0.0};
    bool all = true;
    for (size_t i = 0; i < notes.size(); ++i)
        all = all && std::fabs(positions[i] - expected[i]) < 1e-9;
    report.expect(all, "the ding in the middle, A3 in front, then left, right up the scale, A4 at the back");

    const std::vector<int> shuffled{50, 69, 60, 57, 65, 58, 67, 62, 64};
    const auto again = field_positions(shuffled);
    report.expect(std::fabs(again[1]) < 1e-9 && std::fabs(again[3]) < 1e-9 && again[5] < 0.0,
                  "the layout follows the pitches, not the order the notes are given in");
}

void check_gains(test::TestReport& report)
{
    bool constant_power = true;
    for (double x = -1.0; x <= 1.0; x += 0.25)
        for (double w = 0.0; w <= 1.0; w += 0.25)
        {
            double l = 0.0, r = 0.0;
            stereo_gains(x, w, l, r);
            constant_power = constant_power && std::fabs(l * l + r * r - 2.0) < 1e-12;
        }
    report.expect(constant_power, "every position at every width carries the same power");
    double l = 0.0, r = 0.0;
    stereo_gains(1.0, 0.0, l, r);
    report.expect(std::fabs(l - 1.0) < 1e-12 && std::fabs(r - 1.0) < 1e-12, "no width, no side: mono");
    stereo_gains(0.0, 1.0, l, r);
    report.expect(std::fabs(l - 1.0) < 1e-12 && std::fabs(r - 1.0) < 1e-12,
                  "a field in the middle is as loud in each channel as in the mono render");
    stereo_gains(1.0, 1.0, l, r);
    report.expect(std::fabs(l) < 1e-12, "at full width, a field on the side is in one channel only");
}

void check_render(test::TestReport& report)
{
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    const Model model = solve_instrument(settings, 48000.0);
    const auto heard = [&](int note)
    {
        const auto s = OfflineRenderer::render_stereo(model.sound(), {{0.0, note, 90}}, 1.0);
        return 10.0 * std::log10(energy(s[1]) / energy(s[0])); // right over left, dB
    };
    const double e4 = heard(D_KURD[5]), d4 = heard(D_KURD[4]), ding = heard(D_KURD[0]);
    std::printf("  right over left: E4 %+.1f dB, D4 %+.1f dB, ding %+.1f dB (width %.1f)\n", e4, d4, ding,
                settings.stereo_width);
    report.expect(e4 > 3.0 && d4 < -3.0, "E4 on the right sounds right, D4 on the left sounds left");
    report.expect(std::fabs(ding) < 1.0, "the ding sounds in the middle");
}
} // namespace

int main()
{
    test::TestReport report("layout");
    check_positions(report);
    check_gains(report);
    check_render(report);
    return report.finish();
}
