// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "ArrangerView.h"

#include "../../constants/plugin.hpp"
#include "../../constants/sizes.hpp"
#include "../../constants/theme.hpp"
#include "../../core/arrangement_editor.hpp"
#include "../services/AudioExporter.h"
#include "../style/HandPanLookAndFeel.h"

using namespace grmh;
using namespace grmh::ui;

ArrangerView::ArrangerView(HandPanProcessor& processor) : processor_(processor)
{
    song_.setButtonText(plugin::PARAM_SONG_NAME);
    song_.setTooltip("Play the song (the blocks below, once, from the song's start) instead of the pattern being "
                     "edited");
    song_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters(), plugin::PARAM_SONG, song_);
    add_pattern_.setTooltip("Add the pattern being edited at the end of the song");
    add_rest_.setTooltip("Add a bar of silence at the end of the song");
    add_pattern_.onClick = [this] { add_edited(); };
    add_rest_.onClick = [this] { add_rest(); };
    export_.setTooltip("Write the song to a WAV file");
    export_.onClick = [this] { AudioExporter::choose_and_export(processor_, AudioExporter::What::song); };
    loop_.setButtonText("Loop");
    loop_.setTooltip("Play the blocks selected (click, Shift + click), or the whole song, over and over");
    loop_.onClick = [this] { set_loop(loop_.getToggleState()); };
    for (juce::Component* c : {(juce::Component*)&song_, (juce::Component*)&add_pattern_, (juce::Component*)&add_rest_,
                               (juce::Component*)&loop_, (juce::Component*)&export_, (juce::Component*)&viewport_})
        addAndMakeVisible(c);
    for (juce::Button* b : {(juce::Button*)&song_, (juce::Button*)&add_pattern_, (juce::Button*)&add_rest_,
                            (juce::Button*)&loop_, (juce::Button*)&export_})
        b->setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(true);
    viewport_.setViewedComponent(&list_, false);
    viewport_.setScrollBarsShown(true, false);
    refresh();
}

// ------------------------------------------------------------------ commands

void ArrangerView::change(const std::function<void(Arrangement&)>& edit)
{
    const Arrangement before = processor_.arrangement();
    Arrangement arrangement = before;
    edit(arrangement);
    history_.record(before, arrangement);
    if (arrangement != before)
        processor_.set_arrangement(arrangement);
    refresh();
}

void ArrangerView::undo()
{
    Arrangement arrangement = processor_.arrangement();
    if (history_.undo(arrangement))
        processor_.set_arrangement(arrangement);
    refresh();
}

void ArrangerView::redo()
{
    Arrangement arrangement = processor_.arrangement();
    if (history_.redo(arrangement))
        processor_.set_arrangement(arrangement);
    refresh();
}

bool ArrangerView::keyPressed(const juce::KeyPress& key)
{
    const auto command = juce::ModifierKeys::commandModifier, shift = juce::ModifierKeys::shiftModifier;
    if (key == juce::KeyPress('z', command, 0))
        undo();
    else if (key == juce::KeyPress('z', command | shift, 0) || key == juce::KeyPress('y', command, 0))
        redo();
    else
        return false;
    return true;
}

void ArrangerView::set_rest_signature(int block, TimeSignature signature)
{
    change([block, signature](Arrangement& a) { ArrangementEditor::set_signature(a, block, signature); });
}

void ArrangerView::show_signature_menu(int block)
{
    juce::PopupMenu menu;
    const TimeSignature current = processor_.arrangement().blocks[(size_t)block].signature;
    for (int i = 0; i < (int)std::size(SIGNATURES); ++i)
        menu.addItem(i + 1, juce::String(SIGNATURES[i][0]) + "/" + juce::String(SIGNATURES[i][1]), true,
                     current == TimeSignature{SIGNATURES[i][0], SIGNATURES[i][1]});
    juce::Component::SafePointer<ArrangerView> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(rows_[(size_t)block].get()),
                       [self, block](int chosen)
                       {
                           if (self != nullptr && chosen > 0)
                               self->set_rest_signature(block, {SIGNATURES[chosen - 1][0], SIGNATURES[chosen - 1][1]});
                       });
}

void ArrangerView::add_edited()
{
    change([this](Arrangement& a) { ArrangementEditor::add_pattern(a, processor_.library().current()); });
}

void ArrangerView::add_rest()
{
    change([](Arrangement& a) { ArrangementEditor::add_rest(a); });
}

void ArrangerView::drop_pattern(int pattern, int at)
{
    const SlotId slot{pattern / PATTERNS_PER_BANK, pattern % PATTERNS_PER_BANK};
    if (PatternLibrary::valid(slot))
        change([&](Arrangement& a) { ArrangementEditor::add_pattern(a, slot, at); });
}

void ArrangerView::next_variation(int block)
{
    change([block](Arrangement& a) { ArrangementEditor::next_variation(a, block); });
}

void ArrangerView::change_count(int block, int delta)
{
    change([block, delta](Arrangement& a)
           {
               if (block >= 0 && block < (int)a.blocks.size())
                   ArrangementEditor::set_count(a, block, a.blocks[(size_t)block].count + delta);
           });
}

void ArrangerView::remove(int block)
{
    change([block](Arrangement& a) { ArrangementEditor::remove(a, block); });
}

void ArrangerView::move(int from, int to)
{
    change([from, to](Arrangement& a) { ArrangementEditor::move(a, from, to); });
}

void ArrangerView::edit(int block)
{
    const auto& blocks = processor_.arrangement().blocks;
    if (block < 0 || block >= (int)blocks.size())
        return;
    if (blocks[(size_t)block].rest)
        show_signature_menu(block);
    else
        processor_.select_pattern(blocks[(size_t)block].slot);
}

void ArrangerView::set_loop(bool loop)
{
    change([loop](Arrangement& a) { ArrangementEditor::set_loop(a, loop); });
}

// a selection is not a change of the song: never in the history
void ArrangerView::select(int block, bool extend)
{
    Arrangement arrangement = processor_.arrangement();
    const int anchor = extend && arrangement.selected_first >= 0 ? arrangement.selected_first : block;
    ArrangementEditor::select(arrangement, anchor, block);
    if (arrangement != processor_.arrangement())
        processor_.set_arrangement(arrangement);
    if (!extend)
        edit(block);
    refresh();
}

// ------------------------------------------------------------------ the blocks

int ArrangerView::playing_block() const
{
    if (!processor_.song_mode() || !processor_.transport().shown_playing())
        return -1;
    const auto spans = processor_.song_spans();
    return ArrangementCompiler::block_at(
        spans, ArrangementCompiler::song_place(processor_.arrangement(), spans, processor_.transport().shown_beat()));
}

// a row a block; the rows past the last block are hidden, never destroyed (a block removed by its own row's click
// must not delete the row still handling that click)
void ArrangerView::rebuild()
{
    const auto& blocks = processor_.arrangement().blocks;
    while (rows_.size() < blocks.size())
    {
        const int i = (int)rows_.size();
        auto row = std::make_unique<ArrangementRow>();
        row->on_variation = [this, i] { next_variation(i); };
        row->on_less = [this, i] { change_count(i, -1); };
        row->on_more = [this, i] { change_count(i, 1); };
        row->on_remove = [this, i] { remove(i); };
        row->on_select = [this, i](bool extend) { select(i, extend); };
        row->on_drag = [this, i](const juce::MouseEvent& e)
        {
            drop_index_ = index_at(e.getEventRelativeTo(this).getPosition().y);
            repaint();
        };
        row->on_drop = [this, i](const juce::MouseEvent& e)
        {
            const int to = index_at(e.getEventRelativeTo(this).getPosition().y);
            drop_index_ = -1;
            move(i, to > i ? to - 1 : to); // its index once taken out
            repaint();
        };
        list_.addChildComponent(*row);
        rows_.push_back(std::move(row));
    }
    for (size_t i = 0; i < rows_.size(); ++i)
        rows_[i]->setVisible(i < blocks.size());
    layout_rows();
}

void ArrangerView::refresh()
{
    if (processor_.pattern_version() != shown_version_)
    {
        shown_version_ = processor_.pattern_version();
        rebuild();
    }
    const PatternLibrary& library = processor_.library();
    const Arrangement& arrangement = processor_.arrangement();
    const auto& blocks = arrangement.blocks;
    const int playing = playing_block();
    loop_.setToggleState(arrangement.loop, juce::dontSendNotification);
    for (size_t i = 0; i < blocks.size() && i < rows_.size(); ++i)
    {
        const ArrangementBlock& b = blocks[i];
        const juce::String count = juce::String(juce::CharPointer_UTF8("\xc3\x97")) + juce::String(b.count);
        const bool selected = (int)i >= arrangement.selected_first && (int)i <= arrangement.selected_last;
        if (b.rest)
            rows_[i]->show({}, "Silence  " + juce::String(b.signature.numerator) + "/" +
                                   juce::String(b.signature.denominator) + (b.count == 1 ? "  (1 bar)" : "  (" +
                                   juce::String(b.count) + " bars)"),
                           count, false, (int)i == playing, selected);
        else
            rows_[i]->show(juce::String::charToString((juce::juce_wchar)PatternLibrary::bank_letter(b.slot.bank)),
                           juce::String(library.name(b.slot)) + "  (" +
                               juce::String::charToString((juce::juce_wchar)PatternLibrary::bank_letter(b.slot.bank)) +
                               juce::String(b.slot.slot + 1) + ")",
                           count, b.slot == library.current(), (int)i == playing, selected);
    }
    // the roll follows the block playing
    if (playing != followed_block_)
    {
        followed_block_ = playing;
        if (playing >= 0 && !blocks[(size_t)playing].rest && blocks[(size_t)playing].slot != library.current())
            processor_.select_pattern(blocks[(size_t)playing].slot);
    }
}

// ------------------------------------------------------------------ layout and drops

void ArrangerView::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(DIMENSIONS.button_height);
    song_.setBounds(header.removeFromLeft(DIMENSIONS.arranger_button));
    add_rest_.setBounds(header.removeFromRight(DIMENSIONS.arranger_button));
    header.removeFromRight(SPACING.sm);
    add_pattern_.setBounds(header.removeFromRight(DIMENSIONS.arranger_button));
    area.removeFromTop(SPACING.sm);
    auto second = area.removeFromTop(DIMENSIONS.button_height);
    loop_.setBounds(second.removeFromLeft(DIMENSIONS.arranger_button));
    export_.setBounds(second.removeFromRight(DIMENSIONS.arranger_button));
    area.removeFromTop(SPACING.lg);
    viewport_.setBounds(area);
    layout_rows();
}

void ArrangerView::layout_rows()
{
    const int width = viewport_.getMaximumVisibleWidth();
    int y = 0;
    for (auto& row : rows_)
    {
        row->setBounds(0, y, width, DIMENSIONS.slot_height);
        if (row->isVisible())
            y += DIMENSIONS.slot_height + SPACING.sm;
    }
    list_.setSize(width, std::max(y, viewport_.getHeight()));
}

int ArrangerView::index_at(int y_in_view) const
{
    const int y = y_in_view - viewport_.getY() + viewport_.getViewPositionY();
    return std::clamp((y + DIMENSIONS.slot_height / 2) / (DIMENSIONS.slot_height + SPACING.sm), 0, (int)rows_.size());
}

void ArrangerView::paintOverChildren(juce::Graphics& g)
{
    if (drop_index_ < 0)
        return;
    const int y = viewport_.getY() - viewport_.getViewPositionY() + drop_index_ * (DIMENSIONS.slot_height + SPACING.sm) -
                  SPACING.sm / 2;
    g.setColour(HandPanLookAndFeel::colour(COLORS.accent));
    g.fillRect(viewport_.getX(), y - 1, viewport_.getWidth(), 2);
}

bool ArrangerView::isInterestedInDragSource(const SourceDetails& details) { return details.description.isInt(); }

void ArrangerView::itemDragMove(const SourceDetails& details)
{
    drop_index_ = index_at(details.localPosition.y);
    repaint();
}

void ArrangerView::itemDragExit(const SourceDetails&)
{
    drop_index_ = -1;
    repaint();
}

void ArrangerView::itemDropped(const SourceDetails& details)
{
    const int at = index_at(details.localPosition.y);
    drop_index_ = -1;
    drop_pattern((int)details.description, at);
    repaint();
}
