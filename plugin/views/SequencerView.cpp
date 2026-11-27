// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "SequencerView.h"

#include <cmath>

#include "../../constants/plugin.hpp"
#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../../core/hand_assignment.hpp"
#include "../../core/note_names.hpp"
#include "../../core/pattern_compiler.hpp"
#include "../../core/pattern_editor.hpp"
#include "../services/AudioExporter.h"
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
		return juce::Rectangle<float>(d, d).withCentre({ (float)centre.x, (float)centre.y });
	}
} // namespace

// ------------------------------------------------------------------ the view

SequencerView::SequencerView(HandPanProcessor& processor) : processor_(processor)
{
	toolbar_.on_command = [this](RollToolbar::Command command)
		{
			switch (command)
			{
			case RollToolbar::select_all:
				select_all();
				break;
			case RollToolbar::copy:
				copy();
				break;
			case RollToolbar::paste:
				paste();
				break;
			case RollToolbar::clone:
				clone();
				break;
			case RollToolbar::clear:
			case RollToolbar::count:
				clear();
				break;
			case RollToolbar::export_audio:
				AudioExporter::choose_and_export(processor_, AudioExporter::What::pattern);
				break;
			}
		};
	addAndMakeVisible(toolbar_);
	left_handed_.setButtonText("Left-handed");
	left_handed_.setTooltip("Mirror the roll and lead with the left hand");
	left_handed_.setMouseClickGrabsKeyboardFocus(false);
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

void SequencerView::reload()
{
	if (processor_.library().current() != shown_slot_)
	{
		shown_slot_ = processor_.library().current();
		history_ = History<Pattern>{};
		selection_.clear();
		glow_.clear();
		bottom_ = 0.0;
	}
	const auto handedness = processor_.hands().handedness;
	left_handed_.setToggleState(handedness == Handedness::left_handed, juce::dontSendNotification);
	columns_ = RollLayout::columns(processor_.notes(), handedness);
	hit_hands_ = HandAssignment::assign(processor_.pattern(), processor_.notes(), handedness);
	glow_.resize(processor_.pattern().hits.size(), 0.0f);
	const int count = (int)processor_.pattern().hits.size();
	selection_.erase(std::remove_if(selection_.begin(), selection_.end(), [count](int i) { return i >= count; }),
		selection_.end());
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

void SequencerView::scroll_by(double steps)
{
	if (steps != 0.0)
		scroll_to(bottom_ + (std::fabs(steps) < 1.0 ? std::copysign(1.0, steps) : steps));
}

void SequencerView::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
	scroll_by((wheel.isReversed ? -wheel.deltaY : wheel.deltaY) * processor_.pattern().steps_per_bar());
}

int SequencerView::playing_step() const
{
	const Transport& transport = processor_.transport();
	const Pattern& p = processor_.pattern();
	if (!transport.shown_playing() && transport.shown_beat() <= 0.0)
		return -1;
	const double beat = transport.shown_beat();
	const double in_pattern = std::fmod(std::max(0.0, beat - processor_.pattern_origin(beat)), p.length_beats());
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
	const bool follow_playhead = processor_.parameters().getRawParameterValue(grmh::plugin::PARAM_FOLLOW)->load() > 0.5f;
	if (follow_playhead && step >= 0 && playing && drag_ == Drag::none && (step < bottom_ || step >= bottom_ + per_bar))
		scroll_to(double(step / per_bar * per_bar));
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

void SequencerView::resized()
{
	auto area = getLocalBounds();

	auto top_row = area.removeFromTop(DIMENSIONS.left_handed_switch_height);
	top_row.removeFromLeft(SPACING.md);
	left_handed_.setBounds(top_row.removeFromLeft(DIMENSIONS.left_handed_switch_width));

	auto footer = area.removeFromBottom(DIMENSIONS.roll_footer);
	const int toolbar_width = std::min(toolbar_.preferred_width(), footer.getWidth());
	toolbar_.setBounds(footer.withSizeKeepingCentre(toolbar_width, DIMENSIONS.button_height));

	names_area_ = area.removeFromBottom(DIMENSIONS.roll_names);
	left_gutter_ = area.removeFromLeft(DIMENSIONS.roll_gutter);
	right_gutter_ = area.removeFromRight(DIMENSIONS.roll_gutter);
	roll_area_ = area.withTrimmedTop(SPACING.md);
	names_area_ = names_area_.withX(roll_area_.getX()).withWidth(roll_area_.getWidth());
}

// ------------------------------------------------------------------ where things are

RollPoint SequencerView::roll_point(juce::Point<float> view) const
{
	return { view.x - (double)roll_area_.getX(), view.y - (double)roll_area_.getY() };
}

juce::Point<float> SequencerView::field_point(int step, int field) const
{
	const RollPoint c = geometry().centre(step, column_of(field));
	return { (float)(c.x + roll_area_.getX()), (float)(c.y + roll_area_.getY()) };
}

juce::Point<float> SequencerView::hit_point(int hit) const
{
	const PatternHit& h = processor_.pattern().hits[(size_t)hit];
	const RollPoint c = geometry().hit_centre(h.step, column_of(h.field), h.x, h.y, h.size);
	return { (float)(c.x + roll_area_.getX()), (float)(c.y + roll_area_.getY()) };
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

// ------------------------------------------------------------------ selection and changes

bool SequencerView::is_selected(int hit) const
{
	return std::find(selection_.begin(), selection_.end(), hit) != selection_.end();
}

void SequencerView::select_only(int hit)
{
	selection_ = { hit };
	repaint();
}

void SequencerView::toggle(int hit)
{
	if (is_selected(hit))
		selection_.erase(std::find(selection_.begin(), selection_.end(), hit));
	else
		selection_.push_back(hit);
	repaint();
}

void SequencerView::apply(const Pattern& pattern)
{
	processor_.set_pattern(pattern);
	reload();
}

void SequencerView::commit(const std::function<void(Pattern&)>& change)
{
	const Pattern before = processor_.pattern();
	Pattern after = before;
	change(after);
	history_.record(before, after);
	if (after != before)
		apply(after);
}

void SequencerView::hear(const PatternHit& hit)
{
	if (processor_.transport().shown_playing() || hit.field < 0 || hit.field >= (int)processor_.notes().size())
		return;
	processor_.audition(processor_.notes()[(size_t)hit.field], PatternCompiler::hit_velocity(hit.size));
}

void SequencerView::select_all()
{
	selection_ = PatternEditor::all(processor_.pattern());
	repaint();
}

void SequencerView::copy()
{
	if (!selection_.empty())
		clipboard_ = PatternEditor::copy(processor_.pattern(), selection_);
}

void SequencerView::cut()
{
	copy();
	remove_selection();
}

void SequencerView::paste()
{
	std::vector<int> added;
	commit([&](Pattern& p) { added = PatternEditor::paste(p, clipboard_, paste_step_); });
	selection_ = added;
	repaint();
}

void SequencerView::clone()
{
	std::vector<int> added;
	const std::vector<int> from = selection_;
	commit([&](Pattern& p) { added = PatternEditor::clone(p, from); });
	if (!added.empty())
		selection_ = added;
	repaint();
}

void SequencerView::clear()
{
	if (selection_.empty())
		commit([](Pattern& p) { PatternEditor::clear(p); });
	else
		remove_selection();
}

void SequencerView::remove_selection()
{
	const std::vector<int> gone = selection_;
	selection_.clear();
	commit([&](Pattern& p) { PatternEditor::remove(p, gone); });
	repaint();
}

void SequencerView::undo()
{
	Pattern p = processor_.pattern();
	if (history_.undo(p))
	{
		selection_.clear();
		apply(p);
	}
}

void SequencerView::redo()
{
	Pattern p = processor_.pattern();
	if (history_.redo(p))
	{
		selection_.clear();
		apply(p);
	}
}

void SequencerView::show_hand_menu()
{
	juce::PopupMenu menu;
	const HandChoice current = processor_.pattern().hits[(size_t)selection_.front()].hand;
	menu.addItem(1, "Hand: automatic", true, current == HandChoice::automatic);
	menu.addItem(2, "Left hand", true, current == HandChoice::left);
	menu.addItem(3, "Right hand", true, current == HandChoice::right);
	menu.addSeparator();
	menu.addItem(4, "Delete");
	juce::Component::SafePointer<SequencerView> self(this);
	menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(),
		[self](int chosen)
		{
			if (self == nullptr || chosen == 0)
				return;
			if (chosen == 4)
			{
				self->remove_selection();
				return;
			}
			const HandChoice hand = chosen == 2 ? HandChoice::left
				: chosen == 3 ? HandChoice::right
				: HandChoice::automatic;
			const std::vector<int> chosen_hits = self->selection_;
			self->commit([&](Pattern& p) { PatternEditor::set_hand(p, chosen_hits, hand); });
		});
}

// ------------------------------------------------------------------ gestures

void SequencerView::add_hit(RollPoint point, int step, int field)
{
	PatternHit added;
	added.step = step;
	added.field = field;
	added.size = HIT_SIZE_DEFAULT;
	const RollPoint place = geometry().place_on_field(step, column_of(field), point, added.size);
	added.x = place.x;
	added.y = place.y;
	Pattern p = processor_.pattern();
	const int index = PatternEditor::add(p, added);
	if (index < 0)
		return; // two hits at this step already: two hands
	apply(p);
	select_only(index);
	anchor_ = index;
	drag_base_ = p;
	drag_ = Drag::size; // a drag up or down sets how loud
	hear(added);
}

void SequencerView::mouseDown(const juce::MouseEvent& event)
{
	grabKeyboardFocus();
	drag_ = Drag::none;
	before_gesture_ = drag_base_ = processor_.pattern();
	down_y_ = event.position.y;
	const RollPoint point = roll_point(event.position);
	const int hit = hit_under(point);
	int step = 0, field = 0;
	const bool on_field = field_under(point, step, field);
	if (hit >= 0)
		paste_step_ = processor_.pattern().hits[(size_t)hit].step;
	else if (on_field)
		paste_step_ = step;

	if (event.mods.isPopupMenu())
	{
		if (hit >= 0)
		{
			if (!is_selected(hit))
				select_only(hit);
			show_hand_menu();
		}
		return;
	}
	anchor_ = hit;
	if (hit >= 0)
	{
		if (event.mods.isCommandDown())
			drag_ = Drag::move; // a click without a drag toggles it (mouseUp)
		else
		{
			if (!is_selected(hit))
				select_only(hit);
			drag_ = Drag::size;
		}
		return;
	}
	if (event.mods.isCommandDown())
	{
		drag_ = Drag::rectangle;
		rectangle_ = juce::Rectangle<float>((float)point.x, (float)point.y, 0.0f, 0.0f);
		return;
	}
	if (on_field)
		add_hit(point, step, field);
	else
	{
		selection_.clear();
		repaint();
	}
}

void SequencerView::drag_move(RollPoint point)
{
	if (!is_selected(anchor_))
		selection_.push_back(anchor_);
	int step = 0, field = 0;
	if (!field_under(point, step, field))
		return;
	const PatternHit& from = drag_base_.hits[(size_t)anchor_];
	const bool single = selection_.size() == 1;
	const int column = column_of(single ? field : from.field);
	const RollPoint place = geometry().place_on_field(step, column, point, from.size);
	Pattern p;
	if (PatternEditor::move(p, drag_base_, selection_, step - from.step, single ? field : -1, place.x - from.x,
		place.y - from.y))
		apply(p);
}

void SequencerView::mouseDrag(const juce::MouseEvent& event)
{
	switch (drag_)
	{
	case Drag::size:
	{
		Pattern p;
		PatternEditor::resize(p, drag_base_, selection_, (down_y_ - event.position.y) / HIT_SIZE_DRAG_PIXELS);
		apply(p);
		break;
	}
	case Drag::move:
		if (event.mouseWasDraggedSinceMouseDown())
			drag_move(roll_point(event.position));
		break;
	case Drag::rectangle:
	{
		const RollPoint at = roll_point(event.position);
		const auto from = roll_point(event.mouseDownPosition);
		rectangle_ = juce::Rectangle<float>(juce::Point<float>((float)from.x, (float)from.y),
			juce::Point<float>((float)at.x, (float)at.y));
		repaint();
		break;
	}
	case Drag::none:
		break;
	}
}

void SequencerView::mouseUp(const juce::MouseEvent& event)
{
	const auto& hits = processor_.pattern().hits;
	if (drag_ == Drag::size && anchor_ >= 0 && anchor_ < (int)hits.size() &&
		hits[(size_t)anchor_].size != drag_base_.hits[(size_t)anchor_].size)
		hear(hits[(size_t)anchor_]); // its new loudness
	if (drag_ == Drag::move && !event.mouseWasDraggedSinceMouseDown())
		toggle(anchor_);
	if (drag_ == Drag::rectangle && event.mouseWasDraggedSinceMouseDown())
	{
		selection_.clear();
		for (int i = 0; i < (int)hits.size(); ++i)
		{
			const PatternHit& h = hits[(size_t)i];
			const RollPoint c = geometry().hit_centre(h.step, column_of(h.field), h.x, h.y, h.size);
			const StepRange range = geometry().visible(processor_.pattern().step_count());
			if (h.step >= range.first && h.step <= range.last && rectangle_.contains((float)c.x, (float)c.y))
				selection_.push_back(i);
		}
	}
	history_.record(before_gesture_, processor_.pattern()); // the whole gesture, one change
	drag_ = Drag::none;
	repaint();
}

void SequencerView::mouseDoubleClick(const juce::MouseEvent& event)
{
	const int hit = hit_under(roll_point(event.position));
	if (hit < 0 || event.mods.isCommandDown())
		return;
	drag_ = Drag::none;
	selection_ = { hit };
	remove_selection();
}

bool SequencerView::keyPressed(const juce::KeyPress& key)
{
	const auto command = juce::ModifierKeys::commandModifier, shift = juce::ModifierKeys::shiftModifier;
	const struct
	{
		juce::KeyPress key;
		std::function<void()> run;
	} shortcuts[] = {
		{juce::KeyPress(juce::KeyPress::deleteKey), [this] { remove_selection(); }},
		{juce::KeyPress(juce::KeyPress::backspaceKey), [this] { remove_selection(); }},
		{juce::KeyPress(juce::KeyPress::escapeKey), [this] { selection_.clear(); repaint(); }},
		{juce::KeyPress('a', command, 0), [this] { select_all(); }},
		{juce::KeyPress('c', command, 0), [this] { copy(); }},
		{juce::KeyPress('x', command, 0), [this] { cut(); }},
		{juce::KeyPress('v', command, 0), [this] { paste(); }},
		{juce::KeyPress('d', command, 0), [this] { clone(); }},
		{juce::KeyPress('z', command, 0), [this] { undo(); }},
		{juce::KeyPress('z', command | shift, 0), [this] { redo(); }},
		{juce::KeyPress('y', command, 0), [this] { redo(); }},
	};
	for (const auto& s : shortcuts)
		if (key == s.key)
		{
			s.run();
			return true;
		}
	return false;
}

// ------------------------------------------------------------------ painting

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
		if (drag_ == Drag::rectangle)
		{
			g.setColour(colour(COLORS.accent).withAlpha(OPACITY.hover));
			g.fillRect(rectangle_);
			g.setColour(colour(COLORS.accent));
			g.drawRect(rectangle_, BORDERS.hairline);
		}
	}
	paint_gutters(g, geo, range);
	paint_names(g, geo);
}

void SequencerView::paint_gutters(juce::Graphics& g, const RollGeometry& geo, StepRange range) const
{
	const Pattern& p = processor_.pattern();
	for (int s = range.first; s <= range.last; ++s)
	{
		const float y = (float)(roll_area_.getY() + geo.row_y(s));
		const bool bar_start = s % p.steps_per_bar() == 0, beat_start = s % p.steps_per_beat == 0;
		if (beat_start)
		{
			const auto label = left_gutter_.toFloat()
				.withHeight(bar_start ? 20.0f : 16.0f)
				.withCentre({ left_gutter_.toFloat().getCentreX(), y });

			if (bar_start)
			{
				const auto badge = label.withSizeKeepingCentre(
					std::min(label.getWidth() - 4.0f, 22.0f), 18.0f);
				g.setColour(colour(COLORS.accent).withAlpha(0.18f));
				g.fillRoundedRectangle(badge, 4.0f);
				g.setColour(colour(COLORS.accent).withAlpha(0.55f));
				g.drawRoundedRectangle(badge, 4.0f, BORDERS.hairline);

				g.setColour(colour(COLORS.text));
				g.setFont(HandPanLookAndFeel::heading_font(TYPOGRAPHY.label * 1.15f));
				g.drawText(juce::String(s / p.steps_per_bar() + 1), label, juce::Justification::centred, false);
			}
			else
			{
				g.setColour(colour(COLORS.muted));
				g.setFont(Typefaces::get().interface(TYPOGRAPHY.label, Typefaces::Weight::medium));
				g.drawText(juce::String((s % p.steps_per_bar()) / p.steps_per_beat + 1), label,
					juce::Justification::centred, false);
			}
		}
		const float led = s == shown_step_ ? 10.0f : (beat_start ? 7.0f : 4.0f);
		g.setColour(colour(s == shown_step_ ? COLORS.playing : (beat_start ? COLORS.muted : COLORS.line)));
		g.fillEllipse(juce::Rectangle<float>(led, led).withCentre({ right_gutter_.toFloat().getCentreX(), y }));
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
				hand.withAlpha(0.0f), { halo.getCentreX(), halo.getY() }, true));
			g.fillEllipse(halo);
		}
		const auto area = circle(centre, radius);
		g.setColour(hand.brighter(0.5f * light));
		g.fillEllipse(area);
		const bool selected = is_selected((int)i);
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
		const auto label = names_area_.toFloat().withWidth((float)geo.column_spacing()).withCentre({ x, names_area_.toFloat().getCentreY() });
		g.drawText(juce::String(juce::CharPointer_UTF8(name_of_midi(notes[f]).c_str())), label,
			juce::Justification::centred, false);
	}
}
