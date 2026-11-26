// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The design system holds: its typefaces are embedded and found by the look and feel, the text reads on every surface
// (WCAG 2.1 contrast: 4.5 for text, 3 for what is not text), the two hands' colours stand apart from the surfaces and
// from each other, and vermilion marks only what plays.
#include <cmath>
#include <cstdio>

#include "../constants/theme.hpp"
#include "../plugin/style/HandPanLookAndFeel.h"
#include "../plugin/style/Typefaces.h"
#include "support/test_report.hpp"

using namespace grmh;
using namespace grmh::ui;

namespace
{
// WCAG 2.1 relative luminance and contrast ratio
double luminance(std::uint32_t argb)
{
    const auto channel = [](std::uint32_t v)
    {
        const double c = double(v & 0xffu) / 255.0;
        return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(argb >> 16) + 0.7152 * channel(argb >> 8) + 0.0722 * channel(argb);
}

double contrast(std::uint32_t a, std::uint32_t b)
{
    const double la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

void check_typefaces(test::TestReport& report)
{
    const Typefaces& faces = Typefaces::get();
    report.expect(faces.gothic(Typefaces::Weight::light) != nullptr && faces.gothic(Typefaces::Weight::regular) != nullptr &&
                      faces.gothic(Typefaces::Weight::medium) != nullptr && faces.mincho() != nullptr,
                  "the four embedded typefaces load");
    report.expect(faces.gothic(Typefaces::Weight::regular)->getName() == "Zen Kaku Gothic New" &&
                      faces.mincho()->getName() == "Shippori Mincho",
                  "Zen Kaku Gothic New for the interface, Shippori Mincho for the name");

    HandPanLookAndFeel look;
    juce::Font title = faces.title(20.0f);
    juce::Font plain(juce::FontOptions().withHeight(13.0f));
    report.expect(look.getTypefaceForFont(title) == faces.mincho() &&
                      look.getTypefaceForFont(plain) == faces.gothic(Typefaces::Weight::regular),
                  "the look and feel finds the title's typeface, and gives the interface's to any other font");
    const auto regular = faces.gothic(Typefaces::Weight::regular);
    report.expect(regular->getNominalGlyphForCodepoint(0x266d).has_value() &&
                      regular->getNominalGlyphForCodepoint(0x266f).has_value(),
                  "flats and sharps (B\xe2\x99\xad" "3, F\xe2\x99\xaf" "4) are drawn by the interface's typeface");
}

void check_contrast(test::TestReport& report)
{
    bool text_reads = true, dim_reads = true;
    for (const std::uint32_t surface : {COLORS.window, COLORS.panel, COLORS.raised})
    {
        text_reads = text_reads && contrast(COLORS.text, surface) >= 7.0;
        dim_reads = dim_reads && contrast(COLORS.text_dim, surface) >= 4.5;
    }
    std::printf("  text on a surface %.1f:1 at least, secondary text %.1f:1\n", contrast(COLORS.text, COLORS.raised),
                contrast(COLORS.text_dim, COLORS.raised));
    report.expect(text_reads, "text reads on every surface (7:1, WCAG AAA)");
    report.expect(dim_reads, "secondary text reads on every surface (4.5:1, WCAG AA)");

    bool marks = true;
    for (const std::uint32_t mark : {COLORS.accent, COLORS.right_hand, COLORS.left_hand, COLORS.playing})
        for (const std::uint32_t surface : {COLORS.window, COLORS.panel, COLORS.raised})
            marks = marks && contrast(mark, surface) >= 3.0;
    report.expect(marks, "the hands, the accent and what plays stand out of every surface (3:1)");
    report.expect(contrast(COLORS.window, COLORS.right_hand) >= 4.5 && contrast(COLORS.window, COLORS.left_hand) >= 4.5,
                  "a note's name reads in ink on either hand's colour");
}

void check_hands(test::TestReport& report)
{
    const juce::Colour right(COLORS.right_hand), left(COLORS.left_hand), playing(COLORS.playing);
    const auto hue_gap = [](juce::Colour a, juce::Colour b)
    {
        const float d = std::fabs(a.getHue() - b.getHue());
        return std::min(d, 1.0f - d) * 360.0f;
    };
    std::printf("  hues apart: hands %.0f deg, right hand and playing %.0f deg\n", hue_gap(right, left),
                hue_gap(right, playing));
    report.expect(hue_gap(right, left) >= 90.0f, "the two hands are told apart by their hue");
    report.expect(hue_gap(playing, left) >= 90.0f && playing.getSaturation() > right.getSaturation() + 0.2f,
                  "what plays is told apart from both hands: vermilion, more saturated than the cherry blossom");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    test::TestReport report("design system");
    check_typefaces(report);
    check_contrast(report);
    check_hands(report);
    return report.finish();
}
