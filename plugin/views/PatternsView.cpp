// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "PatternsView.h"

#include "../../constants/plugin.hpp"
#include "../../constants/sizes.hpp"

using namespace grmh;
using namespace grmh::ui;

namespace
{
constexpr int COLUMNS = 2;
}

PatternsView::PatternsView(HandPanProcessor& processor) : processor_(processor)
{
    for (int s = 0; s < PATTERNS_PER_BANK; ++s)
    {
        slots_[(size_t)s].on_select = [this, s] { choose(s); };
        slots_[(size_t)s].on_menu = [this, s] { show_menu(s); };
        slots_[(size_t)s].on_drag = [this, s]
        {
            auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this);
            if (container != nullptr && !container->isDragAndDropActive())
                container->startDragging(bank_ * PATTERNS_PER_BANK + s, &slots_[(size_t)s]);
        };
        addAndMakeVisible(slots_[(size_t)s]);
    }
    for (int b = 0; b < BANKS; ++b)
    {
        auto& button = banks_[(size_t)b];
        button.setButtonText(juce::String::charToString((juce::juce_wchar)PatternLibrary::bank_letter(b)));
        button.setTooltip("Bank " + button.getButtonText());
        button.setMouseClickGrabsKeyboardFocus(false);
        button.onClick = [this, b] { show_bank(b); };
        addAndMakeVisible(button);
    }
    midi_mode_.addItemList(juce::StringArray(plugin::MIDI_MODE_CHOICES[0], plugin::MIDI_MODE_CHOICES[1]), 1);
    midi_mode_.setTooltip("What the MIDI keys play: notes, or the patterns (from C1: A1 to D16), each from the next "
                          "bar line while its key is held");
    midi_mode_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor_.parameters(), plugin::PARAM_MIDI_MODE, midi_mode_);
    addAndMakeVisible(midi_mode_);
    bank_ = processor_.library().current().bank;
    refresh();
}

void PatternsView::show_bank(int bank)
{
    bank_ = juce::jlimit(0, BANKS - 1, bank);
    refresh();
}

void PatternsView::choose(int slot) { processor_.select_pattern({bank_, slot}); refresh(); }

void PatternsView::refresh()
{
    const PatternLibrary& library = processor_.library();
    const int playing = processor_.shown_pattern();
    for (int s = 0; s < PATTERNS_PER_BANK; ++s)
    {
        const SlotId id{bank_, s};
        slots_[(size_t)s].show(s + 1, juce::String(library.name(id)), library.used(id), library.current() == id,
                               playing == bank_ * PATTERNS_PER_BANK + s);
    }
    for (int b = 0; b < BANKS; ++b)
        banks_[(size_t)b].setToggleState(b == bank_, juce::dontSendNotification);
}

void PatternsView::show_menu(int slot)
{
    const SlotId id{bank_, slot};
    juce::PopupMenu menu;
    menu.addItem(1, "Rename...");
    menu.addItem(2, "Copy");
    menu.addItem(3, "Paste", clipboard_.has_value());
    menu.addSeparator();
    menu.addItem(4, "Clear");
    juce::Component::SafePointer<PatternsView> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&slots_[(size_t)slot]),
                       [self, id](int chosen)
                       {
                           if (self == nullptr)
                               return;
                           HandPanProcessor& processor = self->processor_;
                           if (chosen == 1)
                               self->rename(id.slot);
                           else if (chosen == 2)
                               self->clipboard_ = processor.library().slot(id);
                           else if (chosen == 3 && self->clipboard_.has_value())
                               processor.set_slot(id, *self->clipboard_);
                           else if (chosen == 4)
                               processor.set_slot(id, PatternSlot{});
                           self->refresh();
                       });
}

void PatternsView::rename(int slot)
{
    const SlotId id{bank_, slot};
    naming_ = std::make_unique<juce::AlertWindow>("Rename the pattern", "", juce::MessageBoxIconType::NoIcon, this);
    naming_->addTextEditor("name", juce::String(processor_.library().name(id)));
    naming_->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    naming_->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PatternsView> self(this);
    naming_->enterModalState(true, juce::ModalCallbackFunction::create(
                                       [self, id](int result)
                                       {
                                           if (self == nullptr || self->naming_ == nullptr)
                                               return;
                                           if (result == 1)
                                           {
                                               PatternSlot renamed = self->processor_.library().slot(id);
                                               renamed.name = self->naming_->getTextEditorContents("name").trim()
                                                                  .toStdString();
                                               self->processor_.set_slot(id, renamed);
                                           }
                                           self->naming_.reset();
                                           self->refresh();
                                       }));
}

void PatternsView::resized()
{
    auto area = getLocalBounds();
    auto bank_row = area.removeFromBottom(DIMENSIONS.button_height);
    area.removeFromBottom(SPACING.lg);
    midi_mode_.setBounds(area.removeFromBottom(DIMENSIONS.field_height));
    area.removeFromBottom(SPACING.lg);
    const int bank_width = (bank_row.getWidth() - (BANKS - 1) * SPACING.sm) / BANKS;
    for (auto& button : banks_)
    {
        button.setBounds(bank_row.removeFromLeft(bank_width));
        bank_row.removeFromLeft(SPACING.sm);
    }
    const int rows = PATTERNS_PER_BANK / COLUMNS;
    const int column_width = (area.getWidth() - SPACING.sm) / COLUMNS;
    for (int s = 0; s < PATTERNS_PER_BANK; ++s)
    {
        const int column = s / rows, row = s % rows;
        slots_[(size_t)s].setBounds(area.getX() + column * (column_width + SPACING.sm),
                                    area.getY() + row * (DIMENSIONS.slot_height + SPACING.sm), column_width,
                                    DIMENSIONS.slot_height);
    }
}
