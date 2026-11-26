// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The scales a handpan can be tuned to, as MIDI notes, the ding first.
#pragma once

#include <array>

namespace grmh
{
// D Kurd: D3, then A3 Bb3 C4 D4 E4 F4 G4 A4 around it (natural minor from A3, the most common handpan scale)
constexpr std::array<int, 9> D_KURD{50, 57, 58, 60, 62, 64, 65, 67, 69};
constexpr const char* D_KURD_NAME = "D Kurd";
} // namespace grmh
