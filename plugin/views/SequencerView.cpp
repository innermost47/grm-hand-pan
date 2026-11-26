// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "SequencerView.h"

#include <cmath>

#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../../core/hand_assignment.hpp"
#include "../../core/pattern_compiler.hpp"
#include "../../core/pattern_editor.hpp"
#include "../../core/note_names.hpp"
#include "../style/HandPanLookAndFeel.h"
#include "../style/Typefaces.h"

using namespace grmh;
using namespace grmh::ui;

namespace
{
juce::Colour colour(std::uint32_t argb) { return HandPanLookAndFeel::colour(argb); }

juce::Rectangle<float> circle(RollPoint centre, double radius)
{
    const float d = (float)(2.0 * radius);
    return juce::Rectangle<float>(d, d).withCentre({(float)centre.x, (float)centre.y});
}
} // namespace

SequencerView::SequencerView(HandPanProcessor& processor) : processor_(processor)
{
    left_handed_.setButtonText("Left-handed");
    left_handed_.setTooltip("Mirror the roll and lead with the left hand");
    left_handed_.onClick = [this]
    {
        PlayerHands hands = processor_.hands();
        hands.handedness = left_handed_.getToggleState() ? Handedness::left_handed : Handedness::right_handed;
        processor_.set_hands(hands);
    };
    addAndMakeVisible(left_handed_);
    setWantsKeyboardFocus(true);
    reload();
}

RollPoint SequencerView::roll_point(const juce::MouseEvent& event) const
{
    return {event.position.x - (double)roll_area_.getX(), event.position.y - (double)roll_area_.getY()};
}

int SequencerView::column_of(int field) const
{
    return field >= 0 && field < (int)columns_.size() ? columns_[(size_t)field] : 0;
}

int SequencerView::hit_under(RollPoint point) const
{
    const RollGeometry geo = geometry();
    const StepRange range = geo.visible(processor_.pattern().step_count());
    const auto& hits = processor_.pattern().hits;
    for (int i = (int)hits.size() - 1; i >= 0; --i) // the last drawn is on top
    {
        const PatternHit& h = hits[(size_t)i];
        if (h.step < range.first || h.step > range.last)
            continue;
        const int column = column_of(h.field);
        const RollPoint c = geo.hit_centre(h.step, column, h.x, h.y, h.size);
        if (std::hypot(point.x - c.x, point.y - c.y) <= geo.hit_radius(column, h.size))
            return i;
    }
    return -1;
}

bool SequencerView::field_under(RollPoint point, int& step, int& field) const
{
    const RollGeometry geo = geometry();
    const StepRange range = geo.visible(processor_.pattern().step_count());
    for (int s = range.first; s <= range.last; ++s)
        for (size_t f = 0; f < columns_.size(); ++f)
            if (geo.on_field(s, columns_[f], point))
            {
                step = s;
                field = (int)f;
                return true;
            }
    return false;
}

void SequencerView::edit(const std::function<void(Pattern&)>& change)
{
    Pattern pattern = processor_.pattern();
    change(pattern);
    processor_.set_pattern(pattern);
    reload();
}

void SequencerView::hear(const PatternHit& hit)
{
    if (processor_.transport().shown_playing() || hit.field < 0 || hit.field >= (int)processor_.notes().size())
        return;
    processor_.audition(processor_.notes()[(size_t)hit.field], PatternCompiler::hit_velocity(hit.size));
}

void SequencerView::show_hand_menu(int hit)
{
    const HandChoice current = processor_.pattern().hits[(size_t)hit].hand;
    juce::PopupMenu menu;
    menu.addItem(1, "Hand: automatic", true, current == HandChoice::automatic);
    menu.addItem(2, "Left hand", true, current == HandChoice::left);
    menu.addItem(3, "Right hand", true, current == HandChoice::right);
    menu.addSeparator();
    menu.addItem(4, "Delete");
    juce::Component::SafePointer<SequencerView> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(),
                       [self, hit](int chosen)
                       {
                           if (self == nullptr || chosen == 0)
                               return;
                           if (chosen == 4)
                           {
                               self->edit([hit](Pattern& p) { PatternEditor::remove(p, hit); });
                               self->selected_ = -1;
                               return;
                           }
                           const HandChoice hands[] = {HandChoice::automatic, HandChoice::left, HandChoice::right};
                           self->edit([hit, &hands, chosen](Pattern& p)
                                      { PatternEditor::set_hand(p, hit, hands[chosen - 1]); });
                       });
}

void SequencerView::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    drag_ = Drag::none;
    const RollPoint point = roll_point(event);
    const int hit = hit_under(point);
    if (hit >= 0)
    {
        selected_ = hit;
        repaint();
        if (event.mods.isPopupMenu())
        {
            show_hand_menu(hit);
            return;
        }
        drag_ = event.mods.isShiftDown() ? Drag::size : Drag::move;
        drag_start_y_ = event.position.y;
        drag_start_size_ = processor_.pattern().hits[(size_t)hit].size;
        return;
    }
    int step = 0, field = 0;
    if (event.mods.isPopupMenu() || !field_under(point, step, field))
    {
        selected_ = -1;
        repaint();
        return;
    }
    PatternHit added;
    added.step = step;
    added.field = field;
    added.size = HIT_SIZE_DEFAULT;
    const RollPoint place = geometry().place_on_field(step, column_of(field), point, added.size);
    added.x = place.x;
    added.y = place.y;
    int index = -1;
    edit([&](Pattern& p) { index = PatternEditor::add(p, added); });
    if (index < 0)
        return; // two hits at this step already: two hands
    selected_ = index;
    drag_ = Drag::size;
    drag_start_y_ = event.position.y;
    drag_start_size_ = added.size;
    hear(added);
}

void SequencerView::mouseDrag(const juce::MouseEvent& event)
{
    if (drag_ == Drag::none || selected_ < 0 || selected_ >= (int)processor_.pattern().hits.size())
        return;
    if (drag_ == Drag::size)
    {
        const double size = drag_start_size_ + (drag_start_y_ - event.position.y) / HIT_SIZE_DRAG_PIXELS;
        edit([&](Pattern& p) { PatternEditor::resize(p, selected_, size); });
        return;
    }
    const PatternHit& hit = processor_.pattern().hits[(size_t)selected_];
    const RollPoint place = geometry().place_on_field(hit.step, column_of(hit.field), roll_point(event), hit.size);
    edit([&](Pattern& p) { PatternEditor::move(p, selected_, place.x, place.y); });
}

void SequencerView::mouseUp(const juce::MouseEvent&)
{
    if (drag_ == Drag::size && selected_ >= 0 && selected_ < (int)processor_.pattern().hits.size() &&
        processor_.pattern().hits[(size_t)selected_].size != drag_start_size_)
        hear(processor_.pattern().hits[(size_t)selected_]); // its new loudness
    drag_ = Drag::none;
}

void SequencerView::mouseDoubleClick(const juce::MouseEvent& event)
{
    const int hit = hit_under(roll_point(event));
    if (hit < 0)
        return;
    edit([hit](Pattern& p) { PatternEditor::remove(p, hit); });
    selected_ = -1;
    drag_ = Drag::none;
}

bool SequencerView::keyPressed(const juce::KeyPress& key)
{
    if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && selected_ >= 0)
    {
        const int hit = selected_;
        edit([hit](Pattern& p) { PatternEditor::remove(p, hit); });
        selected_ = -1;
        return true;
    }
    return false;
}

void SequencerView::reload()
{
    const auto handedness = processor_.hands().handedness;
    left_handed_.setToggleState(handedness == Handedness::left_handed, juce::dontSendNotification);
    columns_ = RollLayout::columns(processor_.notes(), handedness);
    hit_hands_ = HandAssignment::assign(processor_.pattern(), processor_.notes(), handedness);
    glow_.assign(processor_.pattern().hits.size(), 0.0f);
    if (selected_ >= (int)processor_.pattern().hits.size())
        selected_ = -1;
    shown_version_ = processor_.pattern_version();
    scroll_to(bottom_);
    repaint();
}

RollGeometry SequencerView::geometry() const
{
    const Pattern& p = processor_.pattern();
    return RollGeometry(roll_area_.getWidth(), roll_area_.getHeight(), p.steps_per_bar(), RollLayout::reach(columns_),
                        bottom_);
}

void SequencerView::scroll_to(double bottom)
{
    const Pattern& p = processor_.pattern();
    const double clamped = std::clamp(std::round(bottom), 0.0, RollGeometry::last_bottom(p.step_count(), p.steps_per_bar()));
    if (clamped == bottom_)
        return;
    bottom_ = clamped;
    repaint();
}

int SequencerView::playing_step() const
{
    const Transport& transport = processor_.transport();
    const Pattern& p = processor_.pattern();
    if (!transport.shown_playing() && transport.shown_beat() <= 0.0)
        return -1;
    const double in_pattern = std::fmod(transport.shown_beat(), p.length_beats());
    return std::clamp((int)std::floor(in_pattern / p.step_beats() + 1e-9), 0, p.step_count() - 1);
}

void SequencerView::refresh()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double seconds = last_frame_ms_ > 0.0 ? (now - last_frame_ms_) / 1000.0 : 0.0;
    last_frame_ms_ = now;
    if (processor_.pattern_version() != shown_version_)
        reload();
    const bool glowing = fade(seconds);
    const int step = playing_step();
    if (step == shown_step_)
    {
        if (glowing)
            repaint();
        return;
    }
    shown_step_ = step;
    const bool playing = processor_.transport().shown_playing();
    if (step >= 0 && playing)
        light_step(step);
    const int per_bar = processor_.pattern().steps_per_bar();
    if (step >= 0 && playing && (step < bottom_ || step >= bottom_ + per_bar))
        scroll_to(double(step / per_bar * per_bar)); // the bar playing
    repaint();
}

void SequencerView::light_step(int step)
{
    const auto& hits = processor_.pattern().hits;
    for (size_t i = 0; i < hits.size() && i < glow_.size(); ++i)
        if (hits[i].step == step)
            glow_[i] = 1.0f;
}

bool SequencerView::fade(double seconds)
{
    const float keep = (float)std::exp(-seconds / GLOW.seconds);
    bool any = false;
    for (float& light : glow_)
    {
        light = light * keep < GLOW.threshold ? 0.0f : light * keep;
        any = any || light > 0.0f;
    }
    return any;
}

float SequencerView::rhythm_mark(int step) const
{
    const Pattern& p = processor_.pattern();
    const int in_beat = step % p.steps_per_beat;
    if (step % p.steps_per_bar() == 0)
        return RHYTHM.downbeat;
    if (in_beat == 0)
        return RHYTHM.beat;
    if (p.steps_per_beat % 2 == 0 && in_beat == p.steps_per_beat / 2)
        return RHYTHM.off_beat;
    return RHYTHM.other;
}

void SequencerView::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    scroll_by((wheel.isReversed ? -wheel.deltaY : wheel.deltaY) * processor_.pattern().steps_per_bar());
}

void SequencerView::scroll_by(double steps)
{
    if (steps != 0.0)
        scroll_to(bottom_ + (std::fabs(steps) < 1.0 ? std::copysign(1.0, steps) : steps));
}

void SequencerView::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(DIMENSIONS.roll_header);
    left_handed_.setBounds(header.removeFromRight(DIMENSIONS.lap_switch_width));
    names_area_ = area.removeFromBottom(DIMENSIONS.roll_names);
    left_gutter_ = area.removeFromLeft(DIMENSIONS.roll_gutter);
    right_gutter_ = area.removeFromRight(DIMENSIONS.roll_gutter);
    roll_area_ = area;
    names_area_ = names_area_.withX(roll_area_.getX()).withWidth(roll_area_.getWidth());
}

void SequencerView::paint(juce::Graphics& g)
{
    const RollGeometry geo = geometry();
    const StepRange range = geo.visible(processor_.pattern().step_count());
    {
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(roll_area_);
        g.setOrigin(roll_area_.getPosition());
        paint_fields(g, geo, range);
        paint_hits(g, geo, range);
    }
    paint_gutters(g, geo, range);
    paint_names(g, geo);
}

// the bar and beat numbers on the left, a LED a step on the right
void SequencerView::paint_gutters(juce::Graphics& g, const RollGeometry& geo, StepRange range) const
{
    const Pattern& p = processor_.pattern();
    for (int s = range.first; s <= range.last; ++s)
    {
        const float y = (float)(roll_area_.getY() + geo.row_y(s));
        if (y < roll_area_.getY() || y > roll_area_.getBottom())
            continue;
        const bool bar_start = s % p.steps_per_bar() == 0, beat_start = s % p.steps_per_beat == 0;
        if (beat_start)
        {
            const auto label = left_gutter_.toFloat().withHeight(16.0f).withCentre({left_gutter_.toFloat().getCentreX(), y});
            g.setColour(colour(bar_start ? COLORS.text : COLORS.muted));
            g.setFont(bar_start ? HandPanLookAndFeel::heading_font(TYPOGRAPHY.label)
                                : Typefaces::get().interface(TYPOGRAPHY.caption, Typefaces::Weight::light));
            g.drawText(juce::String(bar_start ? s / p.steps_per_bar() + 1 : (s % p.steps_per_bar()) / p.steps_per_beat + 1),
                       label, juce::Justification::centred, false);
        }
        const float led = s == shown_step_ ? 10.0f : (beat_start ? 7.0f : 4.0f);
        g.setColour(colour(s == shown_step_ ? COLORS.playing : (beat_start ? COLORS.muted : COLORS.line)));
        g.fillEllipse(juce::Rectangle<float>(led, led).withCentre({right_gutter_.toFloat().getCentreX(), y}));
    }
}

void SequencerView::paint_fields(juce::Graphics& g, const RollGeometry& geo, StepRange range) const
{
    for (int s = range.first; s <= range.last; ++s)
    {
        const bool playing = s == shown_step_;
        const float mark = rhythm_mark(s);
        const auto ring = playing ? colour(COLORS.text) : colour(COLORS.line).interpolatedWith(colour(COLORS.text_dim), mark);
        const float width = playing || mark >= RHYTHM.beat ? BORDERS.ring : BORDERS.hairline;
        for (int column : columns_)
        {
            const auto area = circle(geo.centre(s, column), geo.radius(column));
            g.setColour(colour(COLORS.raised));
            g.fillEllipse(area);
            g.setColour(ring);
            g.drawEllipse(area, width);
        }
    }
}

void SequencerView::paint_hits(juce::Graphics& g, const RollGeometry& geo, StepRange range) const
{
    const auto& hits = processor_.pattern().hits;
    for (size_t i = 0; i < hits.size(); ++i)
    {
        const PatternHit& hit = hits[i];
        if (hit.step < range.first || hit.step > range.last || hit.field < 0 || hit.field >= (int)columns_.size())
            continue;
        const int column = columns_[(size_t)hit.field];
        const bool right = i < hit_hands_.size() && hit_hands_[i] == Hand::right;
        const RollPoint centre = geo.hit_centre(hit.step, column, hit.x, hit.y, hit.size);
        const double radius = geo.hit_radius(column, hit.size);
        const auto hand = colour(right ? COLORS.right_hand : COLORS.left_hand);
        const float light = glow(i);
        if (light > 0.0f) // a halo of the hand's colour, fading with the note
        {
            const auto halo = circle(centre, radius * (1.0 + GLOW.spread * light) + geo.radius(column) * light);
            g.setGradientFill(juce::ColourGradient(hand.withAlpha(GLOW.strength * light), halo.getCentre(),
                                                   hand.withAlpha(0.0f), {halo.getCentreX(), halo.getY()}, true));
            g.fillEllipse(halo);
        }
        const auto area = circle(centre, radius);
        g.setColour(hand.brighter(0.5f * light));
        g.fillEllipse(area);
        const bool selected = (int)i == selected_;
        g.setColour(selected ? colour(COLORS.text) : colour(right ? COLORS.right_hand_deep : COLORS.left_hand_deep));
        g.drawEllipse(area, selected ? BORDERS.ring : BORDERS.hairline);
    }
}

// the fields' names, under the lowest line's circles
void SequencerView::paint_names(juce::Graphics& g, const RollGeometry& geo) const
{
    const auto& notes = processor_.notes();
    g.setColour(colour(COLORS.text_dim));
    g.setFont(Typefaces::get().interface(TYPOGRAPHY.small));
    for (size_t f = 0; f < notes.size() && f < columns_.size(); ++f)
    {
        const float x = (float)(roll_area_.getX() + geo.centre(0, columns_[f]).x);
        const auto label = names_area_.toFloat().withWidth((float)geo.column_spacing()).withCentre({x, names_area_.toFloat().getCentreY()});
        g.drawText(juce::String(juce::CharPointer_UTF8(name_of_midi(notes[f]).c_str())), label,
                   juce::Justification::centred, false);
    }
}
