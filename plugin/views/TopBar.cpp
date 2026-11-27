// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "TopBar.h"

#include "../../constants/plugin.hpp"
#include "../../constants/scales.hpp"
#include "../../constants/sequencer.hpp"
#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../../core/pattern_editor.hpp"
#include "../../core/song_position.hpp"
#include "../style/HandPanLookAndFeel.h"
#include "../style/Typefaces.h"

using namespace grmh;
using namespace grmh::ui;

TopBar::TopBar(HandPanProcessor& processor) : processor_(processor)
{
	auto& parameters = processor_.parameters();
	addChildComponent(loader_);
	for (juce::Component* c : { (juce::Component*)&volume_,		(juce::Component*)&width_,		(juce::Component*)&on_lap_,
								(juce::Component*)&play_,		(juce::Component*)&pause_,		(juce::Component*)&stop_,
								(juce::Component*)&position_,	(juce::Component*)&signature_,	(juce::Component*)&scale_,
								(juce::Component*)&follow_playhead_ })
		addAndMakeVisible(c);
	volume_attachment_ = std::make_unique<SliderAttachment>(parameters, plugin::PARAM_VOLUME, volume_.slider());
	width_attachment_ = std::make_unique<SliderAttachment>(parameters, plugin::PARAM_STEREO_WIDTH, width_.slider());
	on_lap_.setButtonText(plugin::PARAM_ON_LAP_NAME);
	on_lap_attachment_ = std::make_unique<ButtonAttachment>(parameters, plugin::PARAM_ON_LAP, on_lap_);

	follow_playhead_.setButtonText(plugin::PARAM_FOLLOW_NAME);
	follow_attachment_ = std::make_unique<ButtonAttachment>(parameters, plugin::PARAM_FOLLOW, follow_playhead_);

	play_.onClick = [this] { processor_.transport().play(); };
	pause_.onClick = [this] { processor_.transport().pause(); };
	stop_.onClick = [this] { processor_.transport().stop(); };

	position_.setJustificationType(juce::Justification::centred);
	position_.setFont(Typefaces::get().interface(TYPOGRAPHY.heading));

	if (processor_.standalone())
	{
		tempo_.setTextValueSuffix(" BPM");
		tempo_attachment_ = std::make_unique<SliderAttachment>(parameters, plugin::PARAM_TEMPO, tempo_);
		tempo_.setNumDecimalPlacesToDisplay(0);
		tempo_.setTooltip("The tempo: drag up or down, or double-click to type it");
		addAndMakeVisible(tempo_);
	}
	else
	{
		tempo_shown_.setJustificationType(juce::Justification::centred);
		tempo_shown_.setTooltip("The host's tempo");
		addAndMakeVisible(tempo_shown_);
	}

	for (int i = 0; i < (int)std::size(SIGNATURES); ++i)
		signature_.addItem(juce::String(SIGNATURES[i][0]) + "/" + juce::String(SIGNATURES[i][1]), i + 1);
	signature_.setTooltip("The pattern's time signature");
	signature_.onChange = [this] { choose_signature(); };
	for (int i = 0; i < (int)std::size(BAR_CHOICES); ++i)
		bars_.addItem(juce::String(BAR_CHOICES[i]) + (BAR_CHOICES[i] == 1 ? " bar" : " bars"), i + 1);
	bars_.setTooltip("The pattern's length (shorter: its hits past the end are removed)");
	bars_.onChange = [this] { choose_bars(); };
	addAndMakeVisible(bars_);
	show_pattern();

	scale_.addItem(D_KURD_NAME, 1);
	scale_.setSelectedId(1, juce::dontSendNotification);
	scale_.setTooltip("The scale the handpan is tuned to");
	refresh();
}

void TopBar::choose_signature()
{
	const int i = signature_.getSelectedId() - 1;
	if (i < 0 || i >= (int)std::size(SIGNATURES))
		return;
	Pattern pattern = processor_.pattern();
	PatternEditor::set_signature(pattern, { SIGNATURES[i][0], SIGNATURES[i][1] }, SIGNATURES[i][2]);
	processor_.set_pattern(pattern);
}

void TopBar::choose_bars()
{
	const int i = bars_.getSelectedId() - 1;
	if (i < 0 || i >= (int)std::size(BAR_CHOICES))
		return;
	Pattern pattern = processor_.pattern();
	PatternEditor::set_bars(pattern, BAR_CHOICES[i]);
	processor_.set_pattern(pattern);
}

void TopBar::show_pattern()
{
	const Pattern& pattern = processor_.pattern();
	for (int i = 0; i < (int)std::size(SIGNATURES); ++i)
		if (SIGNATURES[i][0] == pattern.signature.numerator && SIGNATURES[i][1] == pattern.signature.denominator)
			signature_.setSelectedId(i + 1, juce::dontSendNotification);
	for (int i = 0; i < (int)std::size(BAR_CHOICES); ++i)
		if (BAR_CHOICES[i] == pattern.bars)
			bars_.setSelectedId(i + 1, juce::dontSendNotification);
	shown_version_ = processor_.pattern_version();
}

void TopBar::refresh()
{
	if (processor_.pattern_version() != shown_version_)
		show_pattern();
	loader_.set_active(processor_.solving());
	const Transport& transport = processor_.transport();
	const bool playing = transport.shown_playing();
	const SongPosition at = song_position(transport.shown_beat(), processor_.pattern());
	position_.setText(juce::String(at.bar) + "." + juce::String(at.beat) + "." + juce::String(at.step),
		juce::dontSendNotification);
	play_.set_lit(playing);
	pause_.set_lit(!playing && transport.shown_beat() > 0.0);
	if (tempo_shown_.isVisible())
	{
		const double tempo = processor_.shown_tempo();
		tempo_shown_.setText(tempo > 0.0 ? juce::String(tempo, 1) + " BPM" : juce::String(juce::CharPointer_UTF8("\xe2\x80\x93 BPM")),
			juce::dontSendNotification);
	}
}

void TopBar::paint(juce::Graphics& g)
{
	g.setColour(HandPanLookAndFeel::colour(COLORS.text));
	g.setFont(Typefaces::get().title(TYPOGRAPHY.display));
	g.drawText(JucePlugin_Name, title_area_, juce::Justification::centredLeft, false);

	g.setColour(HandPanLookAndFeel::colour(COLORS.line));
	g.fillRect(getLocalBounds().removeFromBottom(1));
}

void TopBar::resized()
{
	auto area = getLocalBounds().reduced(SPACING.xl, 0);
	const auto centred = [&](juce::Rectangle<int> r, int height)
		{
			return r.withSizeKeepingCentre(r.getWidth(), height);
		};

	// Left: title, knobs, stacked switches
	auto title = area.removeFromLeft(DIMENSIONS.title_width);
	title_area_ = title.withTrimmedBottom(DIMENSIONS.loader_height);
	loader_.setBounds(title.removeFromBottom(DIMENSIONS.loader_height + SPACING.lg)
		.withTrimmedBottom(SPACING.lg));
	area.removeFromLeft(SPACING.lg);

	const int knob_height = DIMENSIONS.knob + DIMENSIONS.knob_caption;

	for (Knob* knob : { &volume_, &width_ })
	{
		knob->setBounds(centred(area.removeFromLeft(DIMENSIONS.knob), knob_height));
		area.removeFromLeft(SPACING.xl);
	}

	const int switch_column_width = std::max(
		DIMENSIONS.lap_switch_width,
		DIMENSIONS.follow_playhead_switch_width
	);
	const int switch_gap = SPACING.xs;
	const int switches_height = 2 * DIMENSIONS.button_height + switch_gap;

	auto switches_col = area.removeFromLeft(switch_column_width)
		.withSizeKeepingCentre(switch_column_width, switches_height);

	on_lap_.setBounds(switches_col.removeFromTop(DIMENSIONS.button_height)
		.withSizeKeepingCentre(DIMENSIONS.lap_switch_width, DIMENSIONS.button_height));
	switches_col.removeFromTop(switch_gap);
	follow_playhead_.setBounds(switches_col.removeFromTop(DIMENSIONS.button_height)
		.withSizeKeepingCentre(DIMENSIONS.follow_playhead_switch_width, DIMENSIONS.button_height));

	area.removeFromLeft(SPACING.xl);

	// Right: scale
	scale_.setBounds(centred(area.removeFromRight(DIMENSIONS.scale_width), DIMENSIONS.field_height));
	area.removeFromRight(SPACING.lg);

	// Middle: transport inside remaining space between left and right
	juce::Component* tempo = processor_.standalone()
		? static_cast<juce::Component*>(&tempo_)
		: static_cast<juce::Component*>(&tempo_shown_);

	const int button = DIMENSIONS.transport_button;
	const int transport_width = 3 * button + 2 * SPACING.sm + SPACING.xl
		+ DIMENSIONS.position_width + SPACING.md
		+ DIMENSIONS.signature_width + SPACING.md
		+ DIMENSIONS.bars_width + SPACING.md
		+ DIMENSIONS.tempo_width;

	auto middle = area.withSizeKeepingCentre(std::min(transport_width, area.getWidth()), getHeight());

	for (TransportButton* b : { &play_, &pause_, &stop_ })
	{
		b->setBounds(centred(middle.removeFromLeft(button), button));
		middle.removeFromLeft(SPACING.sm);
	}

	middle.removeFromLeft(SPACING.xl - SPACING.sm);
	position_.setBounds(centred(middle.removeFromLeft(DIMENSIONS.position_width), DIMENSIONS.field_height));
	middle.removeFromLeft(SPACING.md);
	signature_.setBounds(centred(middle.removeFromLeft(DIMENSIONS.signature_width), DIMENSIONS.field_height));
	middle.removeFromLeft(SPACING.md);
	bars_.setBounds(centred(middle.removeFromLeft(DIMENSIONS.bars_width), DIMENSIONS.field_height));
	middle.removeFromLeft(SPACING.md);
	tempo->setBounds(centred(middle.removeFromLeft(DIMENSIONS.tempo_width), DIMENSIONS.field_height));
}