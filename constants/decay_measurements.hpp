// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The decay of each tuned mode of 33 tone fields, measured by E. Alon, "Analysis and synthesis of the handpan
// sound", MSc by research, University of York (2015) [A15]: anechoic recordings, undamped, struck at the 'D' sweet
// spot. Frequencies from Appendix E (Tables 6.2, 6.3, 6.4, 6.6), T60 from Appendix F (Tables 6.7, 6.8, 6.10, 6.12):
// instruments 1 and 2 (one maker), 3 and 4 after their tuning by an established maker.
//
// Selection: the three strongest peaks of each field carry a T60; a peak is kept when its ratio to the field's
// fundamental is 1, 2 or 3 within MODE_RATIO_SPREAD (its own tuned mode). Peaks at other ratios (0.5, 1.31, 4...) are
// a neighbour field or the double octave of a coupled pair, and are left out, as is the one T60 Alon marks
// inaccurate (B3 of instrument 1, 744.1 Hz).
#pragma once

#include <array>

#include "../models/core.hpp"

namespace grmh
{
constexpr std::array<DecayMeasurement, 84> DECAY_MEASUREMENTS{{
    // instrument 1: Tables 6.2 and 6.7
    {126.4, 1, 2.8}, {250.3, 2, 2.1},                   // B2
    {186.9, 1, 2.5}, {556.9, 3, 4.7}, {374.6, 2, 3.2},  // F#3
    {221.8, 1, 2.7}, {663.2, 3, 4.1}, {440.2, 2, 2.5},  // A3
    {248.8, 1, 4.0}, {499.0, 2, 5.1},                   // B3
    {280.4, 1, 3.0}, {557.5, 2, 4.0},                   // C#4
    {589.8, 2, 5.9}, {295.3, 1, 2.6},                   // D4
    {334.3, 1, 2.8}, {664.5, 2, 4.6},                   // E4
    {743.5, 2, 3.8}, {375.0, 1, 4.0},                   // F#4
    // instrument 2: Tables 6.3 and 6.8
    {134.3, 1, 3.4}, {397.1, 3, 4.7},                   // C3
    {197.3, 1, 2.7}, {394.9, 2, 2.6},                   // G3
    {211.7, 1, 2.7}, {419.7, 2, 3.0}, {625.4, 3, 3.8},  // Ab3
    {253.2, 1, 2.8}, {501.2, 2, 4.3}, {751.4, 3, 1.6},  // B3
    {262.3, 1, 3.0}, {529.5, 2, 2.3},                   // C4
    {591.6, 2, 5.9}, {297.0, 1, 3.5}, {882.7, 3, 3.7},  // D4
    {626.8, 2, 5.6}, {313.6, 1, 2.9},                   // Eb4
    {787.4, 2, 2.9}, {395.6, 1, 4.2}, {1181.0, 3, 2.2}, // G4
    // instrument 3, post-tuned: Tables 6.4 and 6.10
    {147.5, 1, 2.1}, {296.2, 2, 2.9},                   // D3
    {220.4, 1, 2.2}, {662.0, 3, 4.3}, {441.8, 2, 2.4},  // A3
    {703.1, 3, 4.9}, {235.1, 1, 1.9}, {467.8, 2, 3.6},  // A#3
    {589.0, 2, 1.7}, {295.5, 1, 1.7}, {882.6, 3, 1.9},  // D4
    {662.3, 2, 4.7}, {990.0, 3, 3.3}, {332.0, 1, 1.9},  // E4
    {701.5, 2, 2.8}, {351.6, 1, 2.1},                   // F4
    {393.9, 1, 2.8}, {786.5, 2, 3.6}, {1178.0, 3, 2.6}, // G4
    {883.2, 2, 3.7}, {442.8, 1, 3.0}, {1325.0, 3, 2.3}, // A4
    // instrument 4, post-tuned: Tables 6.6 and 6.12
    {591.2, 3, 2.5}, {197.7, 1, 1.9}, {394.1, 2, 2.1},  // G3
    {525.1, 2, 2.8}, {262.3, 1, 2.0}, {787.5, 3, 2.6},  // C4
    {591.5, 2, 2.9}, {883.1, 3, 2.9}, {296.2, 1, 1.5},  // D4
    {662.0, 2, 2.7}, {331.7, 1, 1.7},                   // E4
    {700.7, 2, 2.9}, {351.4, 1, 1.8}, {1051.0, 3, 1.8}, // F4
    {786.0, 2, 2.0}, {394.2, 1, 2.4},                   // G4
    {882.7, 2, 2.5}, {442.7, 1, 2.0}, {1324.0, 3, 1.7}, // A4
    {467.2, 1, 1.9}, {936.4, 2, 2.3}, {1403.0, 3, 1.3}, // Bb4
    {525.6, 1, 2.3}, {1052.0, 2, 1.8},                  // C5
}};
} // namespace grmh
