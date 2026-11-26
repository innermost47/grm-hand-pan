// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// A pattern of the Hand Pan Roll: bars of steps, and on them the hits the player drew on the fields. A hit's circle
// is how hard it is played; two hits on one field at one step are a flam. Beats are quarter notes, as a host counts
// them (ppq).
#pragma once

#include <vector>

#include "../constants/sequencer.hpp"

namespace grmh
{
enum class Hand
{
    left,
    right
};

// who plays a hit: the hand the player forced, or the one the hands' logic chooses (core/hand_assignment.hpp)
enum class HandChoice
{
    automatic,
    left,
    right
};

enum class Handedness
{
    right_handed,
    left_handed
};

struct TimeSignature
{
    int numerator = TIME_SIGNATURE_NUMERATOR_DEFAULT;
    int denominator = TIME_SIGNATURE_DENOMINATOR_DEFAULT;
};

// one hit drawn on a field at a step
struct PatternHit
{
    int step = 0;  // from the start of the pattern
    int field = 0; // index in the instrument's notes, the ding first (a change of scale keeps the hits on their fields)
    double x = 0.0, y = 0.0; // where on the field, in its radius (-1..1): the place the player drew (the engine strikes
                             // every field at the same place until the strike points "O" and "P" are modelled)
    double size = HIT_SIZE_DEFAULT; // 0..1, the circle drawn: the bigger, the louder
    HandChoice hand = HandChoice::automatic;
};

struct Pattern
{
    TimeSignature signature;
    int steps_per_beat = STEPS_PER_BEAT_DEFAULT; // steps in one beat of the signature (its denominator's note)
    int bars = 1;
    std::vector<PatternHit> hits;

    int steps_per_bar() const { return signature.numerator * steps_per_beat; }
    int step_count() const { return bars * steps_per_bar(); }
    double step_beats() const { return 4.0 / signature.denominator / steps_per_beat; } // quarter notes
    double length_beats() const { return step_count() * step_beats(); }
};

// the two hands of the player
struct PlayerHands
{
    Handedness handedness = Handedness::right_handed;
    double weak_hand_speed = WEAK_HAND_SPEED_DEFAULT; // the weaker hand's fingertip speed, relative to the stronger one
};

// a hit as it is played: on its step, a few milliseconds early for the graces of a flam
struct ScheduledStrike
{
    double beat = 0.0;   // quarter notes from the start of the pattern
    double offset = 0.0; // s, added to the beat (negative: a grace before it)
    int note = 0;        // MIDI
    int velocity = 0;    // MIDI, 1-127
    Hand hand = Hand::right;
};
} // namespace grmh
