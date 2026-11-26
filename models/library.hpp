// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The patterns a player keeps: a place in the banks, a pattern and its name.
#pragma once

#include <string>

#include "pattern.hpp"

namespace grmh
{
// a place in the banks: bank 0 is A
struct SlotId
{
    int bank = 0, slot = 0;

    bool operator==(const SlotId& o) const { return bank == o.bank && slot == o.slot; }
    bool operator!=(const SlotId& o) const { return !(*this == o); }
};

struct PatternSlot
{
    std::string name; // empty: the slot's default name ("Pattern 3")
    Pattern pattern;

    bool operator==(const PatternSlot& o) const { return name == o.name && pattern == o.pattern; }
};
} // namespace grmh
