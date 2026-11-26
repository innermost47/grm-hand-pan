// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "HandPanLookAndFeel.h"

#include "Typefaces.h"

using namespace grmh::ui;

HandPanLookAndFeel::HandPanLookAndFeel()
{
    const auto text = colour(COLORS.text), panel = colour(COLORS.panel), raised = colour(COLORS.raised);
    setColour(juce::ResizableWindow::backgroundColourId, colour(COLORS.window));
    setColour(juce::TextButton::buttonColourId, raised);
    setColour(juce::TextButton::buttonOnColourId, raised);
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, colour(COLORS.accent));
    setColour(juce::ComboBox::backgroundColourId, raised);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::outlineColourId, colour(COLORS.line));
    setColour(juce::ComboBox::arrowColourId, colour(COLORS.text_dim));
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, raised);
    setColour(juce::PopupMenu::highlightedTextColourId, colour(COLORS.accent));
    setColour(juce::Label::textColourId, text);
    setColour(juce::Slider::rotarySliderFillColourId, colour(COLORS.accent));
    setColour(juce::Slider::rotarySliderOutlineColourId, colour(COLORS.line));
    setColour(juce::Slider::thumbColourId, text);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ToggleButton::textColourId, text);
    setColour(juce::ToggleButton::tickColourId, colour(COLORS.accent));
    setColour(juce::ScrollBar::thumbColourId, colour(COLORS.line));
    setColour(juce::TooltipWindow::backgroundColourId, panel);
    setColour(juce::TooltipWindow::textColourId, text);
    setColour(juce::TooltipWindow::outlineColourId, colour(COLORS.line));
}

juce::Typeface::Ptr HandPanLookAndFeel::getTypefaceForFont(const juce::Font& font)
{
    const Typefaces& faces = Typefaces::get();
    if (font.getTypefaceName() == faces.mincho()->getName())
        return faces.mincho();
    for (const auto weight : {Typefaces::Weight::light, Typefaces::Weight::medium})
        if (font.getTypefaceName() == faces.gothic(weight)->getName() &&
            font.getTypefaceStyle() == faces.gothic(weight)->getStyle())
            return faces.gothic(weight);
    return faces.gothic(font.isBold() ? Typefaces::Weight::medium : Typefaces::Weight::regular);
}

juce::Font HandPanLookAndFeel::heading_font(float height)
{
    return Typefaces::get().interface(height, Typefaces::Weight::medium).withExtraKerningFactor(TYPOGRAPHY.tracking);
}

void HandPanLookAndFeel::draw_surface(juce::Graphics& g, juce::Rectangle<float> area, float corner, juce::Colour border,
                                      bool hovered, bool down)
{
    g.setColour(colour(COLORS.raised));
    g.fillRoundedRectangle(area, corner);
    if (hovered || down)
    {
        g.setColour(colour(COLORS.text).withAlpha(down ? OPACITY.pressed : OPACITY.hover));
        g.fillRoundedRectangle(area, corner);
    }
    g.setColour(border);
    g.drawRoundedRectangle(area, corner, BORDERS.hairline);
}

// a disc on a ring: the ring's arc shows the value in the accent colour, a short ink stroke on the disc points to it
// (the knob's name is written under it: widgets/Knob)
void HandPanLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position,
                                          float start_angle, float end_angle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    const float diameter = std::min(bounds.getWidth(), bounds.getHeight()) - 2.0f * BORDERS.ring;
    const auto ring = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
    const float radius = diameter * 0.5f;
    const float angle = start_angle + position * (end_angle - start_angle);
    const bool enabled = slider.isEnabled();

    juce::Path track;
    track.addCentredArc(ring.getCentreX(), ring.getCentreY(), radius, radius, 0.0f, start_angle, end_angle, true);
    g.setColour(colour(COLORS.line));
    g.strokePath(track, juce::PathStrokeType(BORDERS.ring, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc(ring.getCentreX(), ring.getCentreY(), radius, radius, 0.0f, start_angle, angle, true);
    g.setColour(colour(COLORS.accent).withMultipliedAlpha(enabled ? 1.0f : OPACITY.disabled));
    g.strokePath(value, juce::PathStrokeType(BORDERS.ring, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto disc = ring.reduced((float)SPACING.sm + BORDERS.ring);
    g.setColour(colour(COLORS.raised));
    g.fillEllipse(disc);
    g.setColour(colour(COLORS.line));
    g.drawEllipse(disc, BORDERS.hairline);

    const float r = disc.getWidth() * 0.5f;
    const auto towards = [&](float share)
    {
        return disc.getCentre().getPointOnCircumference(r * share, angle);
    };
    g.setColour(colour(COLORS.text).withMultipliedAlpha(enabled ? 1.0f : OPACITY.disabled));
    g.drawLine(juce::Line<float>(towards(0.3f), towards(0.9f)), BORDERS.ring);
}

void HandPanLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float position,
                                          float min_position, float max_position, juce::Slider::SliderStyle style,
                                          juce::Slider& slider)
{
    if (!slider.isBar())
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, position, min_position, max_position, style, slider);
        return;
    }
    const auto area = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(BORDERS.hairline * 0.5f);
    draw_surface(g, area, BORDERS.corner, colour(COLORS.line), slider.isMouseOverOrDragging(), slider.isMouseButtonDown());
}

void HandPanLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                              bool hovered, bool down)
{
    const auto area = button.getLocalBounds().toFloat().reduced(BORDERS.hairline * 0.5f);
    const auto border = button.getToggleState() ? colour(COLORS.accent) : colour(COLORS.line);
    draw_surface(g, area, BORDERS.corner, border, hovered && button.isEnabled(), down);
}

juce::Font HandPanLookAndFeel::getTextButtonFont(juce::TextButton&, int) { return heading_font(TYPOGRAPHY.small); }

void HandPanLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const auto text = button.getToggleState() ? colour(COLORS.accent) : colour(COLORS.text);
    g.setColour(text.withMultipliedAlpha(button.isEnabled() ? 1.0f : OPACITY.disabled));
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.drawText(button.getButtonText().toUpperCase(), button.getLocalBounds(), juce::Justification::centred, false);
}

// a small switch: a pill, its dot on the right and in the accent colour when on; the text after it
void HandPanLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool hovered, bool)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const float height = (float)DIMENSIONS.button_height * 0.6f;
    const auto pill = juce::Rectangle<float>(height * 1.8f, height).withY(bounds.getCentreY() - height * 0.5f);
    const bool on = button.getToggleState();
    g.setColour(colour(COLORS.raised));
    g.fillRoundedRectangle(pill, height * 0.5f);
    g.setColour(on ? colour(COLORS.accent) : colour(COLORS.line).brighter(hovered ? 0.3f : 0.0f));
    g.drawRoundedRectangle(pill, height * 0.5f, BORDERS.hairline);
    const float dot = height - 2.0f * (float)SPACING.sm;
    const auto knob = juce::Rectangle<float>(dot, dot).withCentre(
        {on ? pill.getRight() - height * 0.5f : pill.getX() + height * 0.5f, pill.getCentreY()});
    g.setColour(on ? colour(COLORS.accent) : colour(COLORS.text_dim));
    g.fillEllipse(knob);

    g.setColour(colour(COLORS.text).withMultipliedAlpha(button.isEnabled() ? 1.0f : OPACITY.disabled));
    g.setFont(Typefaces::get().interface(TYPOGRAPHY.label));
    g.drawText(button.getButtonText(), bounds.withLeft(pill.getRight() + (float)SPACING.md),
               juce::Justification::centredLeft, true);
}

void HandPanLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool down, int, int, int, int,
                                      juce::ComboBox& box)
{
    const auto area = juce::Rectangle<int>(width, height).toFloat().reduced(BORDERS.hairline * 0.5f);
    draw_surface(g, area, BORDERS.corner, colour(COLORS.line), box.isMouseOver(true), down);

    const float size = (float)height * 0.18f;
    const auto centre = juce::Point<float>(area.getRight() - (float)height * 0.5f, area.getCentreY());
    juce::Path chevron;
    chevron.startNewSubPath(centre.x - size, centre.y - size * 0.5f);
    chevron.lineTo(centre.x, centre.y + size * 0.5f);
    chevron.lineTo(centre.x + size, centre.y - size * 0.5f);
    g.setColour(colour(COLORS.text_dim));
    g.strokePath(chevron, juce::PathStrokeType(BORDERS.ring, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font HandPanLookAndFeel::getComboBoxFont(juce::ComboBox&) { return Typefaces::get().interface(TYPOGRAPHY.body); }

void HandPanLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(SPACING.lg, 0, box.getWidth() - box.getHeight() - SPACING.lg, box.getHeight());
    label.setFont(getComboBoxFont(box));
}

void HandPanLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(colour(COLORS.panel));
    g.setColour(colour(COLORS.line));
    g.drawRect(juce::Rectangle<int>(width, height).toFloat(), BORDERS.hairline);
}

void HandPanLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool separator,
                                           bool active, bool highlighted, bool ticked, bool, const juce::String& text,
                                           const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (separator)
    {
        g.setColour(colour(COLORS.line));
        g.fillRect(area.withSizeKeepingCentre(area.getWidth() - 2 * SPACING.lg, 1));
        return;
    }
    if (highlighted && active)
    {
        g.setColour(colour(COLORS.raised));
        g.fillRect(area);
    }
    const auto text_area = area.reduced(SPACING.lg, 0);
    g.setColour(ticked || (highlighted && active) ? colour(COLORS.accent)
                                                  : colour(COLORS.text).withMultipliedAlpha(active ? 1.0f : OPACITY.disabled));
    g.setFont(getPopupMenuFont());
    g.drawText(text, text_area, juce::Justification::centredLeft, true);
}

juce::Font HandPanLookAndFeel::getPopupMenuFont() { return Typefaces::get().interface(TYPOGRAPHY.body); }

void HandPanLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                       bool vertical, int thumb_start, int thumb_size, bool over, bool down)
{
    const auto track = juce::Rectangle<int>(x, y, width, height).toFloat();
    const auto thumb = vertical ? track.withY((float)thumb_start).withHeight((float)thumb_size).reduced(width * 0.3f, 0.0f)
                                : track.withX((float)thumb_start).withWidth((float)thumb_size).reduced(0.0f, height * 0.3f);
    g.setColour(colour(over || down ? COLORS.muted : COLORS.line));
    g.fillRoundedRectangle(thumb, std::min(thumb.getWidth(), thumb.getHeight()) * 0.5f);
}

juce::Font HandPanLookAndFeel::getLabelFont(juce::Label&) { return Typefaces::get().interface(TYPOGRAPHY.body); }
