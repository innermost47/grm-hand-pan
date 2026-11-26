// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The design system on one page, drawn by the plugin's look and feel, saved as a PNG for Anthony to look at: the
// palette, the type scale, the knobs, the buttons, a combo box, a switch, the colours of the two hands and of what
// plays. Usage: render_design_system [out.png]
#include <BinaryData.h>

#include "../constants/sizes.hpp"
#include "../constants/theme.hpp"
#include "../plugin/style/HandPanLookAndFeel.h"
#include "../plugin/style/Typefaces.h"

using namespace grmh::ui;

namespace
{
constexpr int WIDTH = 1100, HEIGHT = 560, MARGIN = 40, SCALE = 2;

juce::String utf8(const char* text) { return juce::String(juce::CharPointer_UTF8(text)); }

class DesignSystemSheet : public juce::Component
{
public:
    DesignSystemSheet()
    {
        setLookAndFeel(&look_);
        const char* letters[] = {"V", "W", "K"};
        const double values[] = {0.8, 0.6, 0.0};
        for (size_t i = 0; i < knobs_.size(); ++i)
        {
            auto& knob = knobs_[i];
            knob.setName(letters[i]);
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            knob.setRange(0.0, 1.0);
            knob.setValue(values[i], juce::dontSendNotification);
            addAndMakeVisible(knob);
        }
        const char* names[] = {"Copy", "Paste", "Clone", "Select"};
        for (size_t i = 0; i < buttons_.size(); ++i)
        {
            buttons_[i].setButtonText(names[i]);
            buttons_[i].setToggleState(i == 3, juce::dontSendNotification);
            addAndMakeVisible(buttons_[i]);
        }
        scale_.addItemList({"D Kurd", "D Celtic minor", "C Hijaz"}, 1);
        scale_.setSelectedId(1, juce::dontSendNotification);
        addAndMakeVisible(scale_);
        left_handed_.setButtonText("Left-handed");
        on_lap_.setButtonText("On the lap");
        on_lap_.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(left_handed_);
        addAndMakeVisible(on_lap_);
        setSize(WIDTH, HEIGHT);
    }

    ~DesignSystemSheet() override { setLookAndFeel(nullptr); }

    void resized() override
    {
        const int top = 260;
        for (size_t i = 0; i < knobs_.size(); ++i)
            knobs_[i].setBounds(MARGIN + (int)i * (DIMENSIONS.knob + SPACING.xl), top, DIMENSIONS.knob, DIMENSIONS.knob);
        for (size_t i = 0; i < buttons_.size(); ++i)
            buttons_[i].setBounds(MARGIN + (int)i * (84 + SPACING.md), top + 90, 84, DIMENSIONS.button_height);
        scale_.setBounds(420, top + 4, 240, DIMENSIONS.field_height);
        left_handed_.setBounds(420, top + 56, 200, DIMENSIONS.button_height);
        on_lap_.setBounds(420, top + 90, 200, DIMENSIONS.button_height);
    }

    void paint(juce::Graphics& g) override
    {
        const Typefaces& faces = Typefaces::get();
        g.fillAll(HandPanLookAndFeel::colour(COLORS.window));

        g.setColour(HandPanLookAndFeel::colour(COLORS.text));
        g.setFont(faces.title(TYPOGRAPHY.display * 1.4f));
        g.drawText("GRM Hand Pan", MARGIN, 28, 500, 40, juce::Justification::centredLeft);
        g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
        g.setFont(faces.interface(TYPOGRAPHY.body, Typefaces::Weight::light));
        g.drawText(utf8("Design system  \xe2\x80\x94  sumi, washi, sakura, hanada, shu"), MARGIN, 70, 600, 20,
                   juce::Justification::centredLeft);

        paint_palette(g, 112);
        paint_type_scale(g);
        paint_hands(g, 420);
    }

private:
    void paint_palette(juce::Graphics& g, int top)
    {
        const struct
        {
            std::uint32_t argb;
            const char* name;
            const char* use;
        } swatches[] = {{PALETTE.sumi_deep, "sumi deep", "window"},  {PALETTE.sumi, "sumi", "panel"},
                        {PALETTE.sumi_raised, "sumi raised", "surface"}, {PALETTE.sumi_line, "sumi line", "hairline"},
                        {PALETTE.washi, "washi", "text"},            {PALETTE.washi_dim, "washi dim", "secondary"},
                        {PALETTE.stone, "stone", "muted"},           {PALETTE.sakura, "sakura", "right hand"},
                        {PALETTE.hanada, "hanada", "left hand"},     {PALETTE.shu, "shu", "playing"}};
        const int size = 64, gap = 30;
        int x = MARGIN;
        for (const auto& s : swatches)
        {
            const auto chip = juce::Rectangle<float>((float)x, (float)top, (float)size, (float)size);
            g.setColour(HandPanLookAndFeel::colour(s.argb));
            g.fillRoundedRectangle(chip, BORDERS.corner);
            g.setColour(HandPanLookAndFeel::colour(COLORS.line));
            g.drawRoundedRectangle(chip, BORDERS.corner, BORDERS.hairline);
            g.setColour(HandPanLookAndFeel::colour(COLORS.text));
            g.setFont(Typefaces::get().interface(TYPOGRAPHY.small));
            g.drawText(s.name, x - 6, top + size + 6, size + 12, 16, juce::Justification::centred);
            g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
            g.setFont(Typefaces::get().interface(TYPOGRAPHY.caption, Typefaces::Weight::light));
            g.drawText(s.use, x - 6, top + size + 22, size + 12, 14, juce::Justification::centred);
            x += size + gap;
        }
    }

    void paint_type_scale(juce::Graphics& g)
    {
        const Typefaces& faces = Typefaces::get();
        const int x = 720;
        int y = 250;
        g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
        g.setFont(HandPanLookAndFeel::heading_font(TYPOGRAPHY.small));
        g.drawText("SEQUENCER", x, y, 300, 20, juce::Justification::centredLeft);
        y += 28;
        const struct
        {
            float height;
            Typefaces::Weight weight;
            const char* text;
        } lines[] = {{TYPOGRAPHY.heading, Typefaces::Weight::medium, "Pattern 1  \xc2\xb7  Verse"},
                     {TYPOGRAPHY.body, Typefaces::Weight::regular, "D3  A3  B\xe2\x99\xad" "3  C4  D4  E4  F4  G4  A4"},
                     {TYPOGRAPHY.label, Typefaces::Weight::regular, "32.1.0     4/4     84 BPM"},
                     {TYPOGRAPHY.small, Typefaces::Weight::light, "Two hits on one note: a flam"},
                     {TYPOGRAPHY.caption, Typefaces::Weight::light, "bar 3 of 8"}};
        for (const auto& line : lines)
        {
            g.setColour(HandPanLookAndFeel::colour(COLORS.text));
            g.setFont(faces.interface(line.height, line.weight));
            g.drawText(utf8(line.text), x, y, 340, (int)line.height + 8, juce::Justification::centredLeft);
            y += (int)line.height + 14;
        }
    }

    // the two hands' colours on note circles, and the LED of the bar that plays
    void paint_hands(juce::Graphics& g, int top)
    {
        const auto circle = [&](float cx, float diameter, std::uint32_t fill, std::uint32_t ring, const char* note,
                                const char* caption)
        {
            g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
            g.setFont(Typefaces::get().interface(TYPOGRAPHY.caption, Typefaces::Weight::light));
            g.drawText(caption, juce::Rectangle<float>(70.0f, 14.0f).withCentre({cx, (float)top + 92.0f}),
                       juce::Justification::centred);
            const auto area = juce::Rectangle<float>(diameter, diameter).withCentre({cx, (float)top + 50.0f});
            g.setColour(HandPanLookAndFeel::colour(fill).withAlpha(fill == COLORS.raised ? 1.0f : 0.9f));
            g.fillEllipse(area);
            g.setColour(HandPanLookAndFeel::colour(ring));
            g.drawEllipse(area, BORDERS.ring);
            g.setColour(HandPanLookAndFeel::colour(fill == COLORS.raised ? COLORS.text_dim : COLORS.window));
            g.setFont(Typefaces::get().interface(TYPOGRAPHY.label, Typefaces::Weight::medium));
            g.drawText(note, area, juce::Justification::centred);
        };
        circle(MARGIN + 30.0f, 52.0f, COLORS.raised, COLORS.line, "D3", "empty");
        circle(MARGIN + 100.0f, 52.0f, COLORS.left_hand, COLORS.left_hand_deep, "D4", "left hand");
        circle(MARGIN + 170.0f, 52.0f, COLORS.right_hand, COLORS.right_hand_deep, "E4", "right hand");
        circle(MARGIN + 240.0f, 34.0f, COLORS.right_hand, COLORS.right_hand_deep, "", "softer");

        const float led_x = MARGIN + 320.0f;
        for (int i = 0; i < 4; ++i)
        {
            const float d = i == 1 ? 12.0f : 8.0f;
            const auto led = juce::Rectangle<float>(d, d).withCentre({led_x + i * 26.0f, (float)top + 50.0f});
            g.setColour(HandPanLookAndFeel::colour(i == 1 ? COLORS.playing : COLORS.line));
            g.fillEllipse(led);
        }
        g.setColour(HandPanLookAndFeel::colour(COLORS.text_dim));
        g.setFont(Typefaces::get().interface(TYPOGRAPHY.caption, Typefaces::Weight::light));
        g.drawText("bars: the one playing", (int)led_x - 6, top + 85, 200, 14, juce::Justification::centredLeft);
    }

    HandPanLookAndFeel look_;
    std::array<juce::Slider, 3> knobs_;
    std::array<juce::TextButton, 4> buttons_;
    juce::ComboBox scale_;
    juce::ToggleButton left_handed_, on_lap_;
};
} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1]
                                                                                           : "renders/design_system.png");
    out.getParentDirectory().createDirectory();
    DesignSystemSheet sheet;
    const juce::Image image = sheet.createComponentSnapshot(sheet.getLocalBounds(), true, (float)SCALE);
    out.deleteFile();
    juce::FileOutputStream stream(out);
    if (!stream.openedOk() || !juce::PNGImageFormat().writeImageToStream(image, stream))
        return 1;
    std::printf("%s\n", out.getFullPathName().toRawUTF8());
    return 0;
}
