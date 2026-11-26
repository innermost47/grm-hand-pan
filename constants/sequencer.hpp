// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The Hand Pan Roll's defaults and the way a drawn hit becomes a strike. They are choices, not measurements, for
// Anthony's ear to tune, except where a source is given. On the balance of the two hands, the sources found
// (26/11/2026) do not settle a default:
//   [P89]  Petersen et al., "Grip strength and hand dominance: challenging the 10% rule", Am. J. Occup. Ther. (1989),
//          abstract, 310 subjects: the dominant hand grips 12.72 % harder in right-handers, -0.08 % in left-handers.
//   [F06]  Fujii et al., "Tapping speed asymmetry in drummers for single-hand tapping with a stick" (2006), PubMed
//          17037670, abstract, 13 right-handed drummers against 13 non-drummers: the drummers' hands are less unequal.
//   (volumes and pages not checked: PubMed refused the automated reading)
// A grip is not a tap, and a trained player's hands grow alike [F06]: both hands strike alike unless the player says
// otherwise (Anthony, 26/11/2026).
#pragma once

namespace grmh
{
// ---- the grid
constexpr int TIME_SIGNATURE_NUMERATOR_DEFAULT = 4;
constexpr int TIME_SIGNATURE_DENOMINATOR_DEFAULT = 4;
constexpr int STEPS_PER_BEAT_DEFAULT = 4; // sixteenths in 4/4

// ---- the plugin's own clock (when no host gives a tempo)
constexpr double TEMPO_DEFAULT = 90.0; // BPM
constexpr double TEMPO_MIN = 30.0;
constexpr double TEMPO_MAX = 240.0;

// ---- the most strikes one audio block can hold (two hands, flams, a long block at a fast tempo: far above)
constexpr int MAX_BLOCK_STRIKES = 256;

// ---- the signatures the roll offers: numerator, denominator, steps in a beat (sixteenths)
constexpr int SIGNATURES[][3] = {{4, 4, 4}, {3, 4, 4}, {2, 4, 4}, {5, 4, 4},
                                 {6, 8, 2}, {7, 8, 2}, {9, 8, 2}, {12, 8, 2}};

// ---- a hit's circle (0..1) is a MIDI velocity, from the lightest to the hardest
constexpr double HIT_SIZE_DEFAULT = 0.5;
constexpr int HIT_VELOCITY_MIN = 1;
constexpr int HIT_VELOCITY_MAX = 127;

// ---- a flam: the second finger lands this long after the first (each grace this much earlier than the next)
constexpr double FLAM_GAP = 0.03; // s

// ---- the weaker hand's fingertip speed, relative to the stronger one: 1 both alike (see [P89] [F06] above)
constexpr double WEAK_HAND_SPEED_DEFAULT = 1.0;

// ---- a field this close to the middle (stereo position, core/layout.hpp) is on neither side: the ding, the field in
// front of the player, the one at the back
constexpr double CENTRE_POSITION_TOLERANCE = 1e-6;
} // namespace grmh
