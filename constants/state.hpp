// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The names under which a session is saved (the host's project): change none of them, or old sessions no longer load.
#pragma once

namespace grmh::state
{
constexpr const char* LIBRARY = "Library";
constexpr const char* CURRENT_BANK = "current_bank";
constexpr const char* CURRENT_SLOT = "current_slot";
constexpr const char* SLOT = "Slot";
constexpr const char* BANK = "bank";
constexpr const char* INDEX = "slot";
constexpr const char* NAME = "name";
constexpr const char* NUMERATOR = "numerator";
constexpr const char* DENOMINATOR = "denominator";
constexpr const char* STEPS_PER_BEAT = "steps_per_beat";
constexpr const char* BARS = "bars";
constexpr const char* HIT = "Hit";
constexpr const char* STEP = "step";
constexpr const char* FIELD = "field";
constexpr const char* X = "x";
constexpr const char* Y = "y";
constexpr const char* SIZE = "size";
constexpr const char* HAND = "hand"; // 0 automatic, 1 left, 2 right
constexpr const char* HANDS = "Hands";
constexpr const char* LEFT_HANDED = "left_handed";
constexpr const char* WEAK_HAND_SPEED = "weak_hand_speed";
} // namespace grmh::state
