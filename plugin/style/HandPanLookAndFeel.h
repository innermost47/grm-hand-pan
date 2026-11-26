// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// How the plugin's widgets look: flat sumi surfaces, hairlines, washi text, one colour at a time for what matters
// (constants/theme.hpp, constants/sizes.hpp). Every widget of the plugin draws through it: no colour nor size of its
// own.
#pragma once

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../JuceHeader.h"

class HandPanLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HandPanLookAndFeel();

    static juce::Colour colour(std::uint32_t argb) { return juce::Colour(argb); }

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position, float start_angle,
                          float end_angle, juce::Slider& slider) override;

    // a value dragged up and down on a field (the tempo): the field, its value written on it
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float position, float min_position,
                          float max_position, juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& background, bool hovered,
                              bool down) override;
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool hovered, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int height) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool hovered, bool down) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool down, int button_x, int button_y, int button_w,
                      int button_h, juce::ComboBox& box) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool separator, bool active,
                           bool highlighted, bool ticked, bool has_sub_menu, const juce::String& text,
                           const juce::String& shortcut, const juce::Drawable* icon,
                           const juce::Colour* text_colour) override;
    juce::Font getPopupMenuFont() override;

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& bar, int x, int y, int width, int height, bool vertical,
                       int thumb_start, int thumb_size, bool over, bool down) override;

    juce::Font getLabelFont(juce::Label& label) override;

    // a heading's text: medium, spaced letters (a panel's title, a knob's letter)
    static juce::Font heading_font(float height);

private:
    // a flat surface with a hairline: what a button, a field and a knob stand on
    static void draw_surface(juce::Graphics& g, juce::Rectangle<float> area, float corner, juce::Colour border,
                             bool hovered, bool down);
};
