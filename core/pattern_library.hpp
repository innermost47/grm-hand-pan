// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The player's patterns: BANKS banks (A, B, C, D) of PATTERNS_PER_BANK patterns, each with its name, and the one being
// edited (the one the roll shows and the plugin plays).
#pragma once

#include <array>
#include <string>

#include "../constants/sequencer.hpp"
#include "../models/library.hpp"

namespace grmh
{
class PatternLibrary
{
public:
    static bool valid(SlotId id)
    {
        return id.bank >= 0 && id.bank < BANKS && id.slot >= 0 && id.slot < PATTERNS_PER_BANK;
    }

    static char bank_letter(int bank) { return (char)('A' + bank); }

    const PatternSlot& slot(SlotId id) const { return slots_[index(id)]; }
    PatternSlot& slot(SlotId id) { return slots_[index(id)]; }

    // a slot's name as shown: its own, or "Pattern N"
    std::string name(SlotId id) const
    {
        const std::string& own = slot(id).name;
        return own.empty() ? "Pattern " + std::to_string(id.slot + 1) : own;
    }

    bool used(SlotId id) const { return !slot(id).pattern.hits.empty(); }

    SlotId current() const { return current_; }
    void select(SlotId id)
    {
        if (valid(id))
            current_ = id;
    }
    const Pattern& pattern() const { return slot(current_).pattern; }
    Pattern& pattern() { return slot(current_).pattern; }

    bool operator==(const PatternLibrary& o) const { return slots_ == o.slots_ && current_ == o.current_; }

private:
    static size_t index(SlotId id)
    {
        const SlotId safe = valid(id) ? id : SlotId{};
        return (size_t)(safe.bank * PATTERNS_PER_BANK + safe.slot);
    }

    std::array<PatternSlot, (size_t)(BANKS * PATTERNS_PER_BANK)> slots_{};
    SlotId current_;
};
} // namespace grmh
