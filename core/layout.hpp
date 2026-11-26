// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Where each field sits on the instrument, as the player sees it from above, and where it sounds between the two
// channels. The ding is in the middle; the other fields ring it, evenly spaced: the lowest straight in front of the
// player (struck with the right hand), then up the scale on the left, on the right, on the left... the highest at the
// back (the layout of a handpan as Anthony plays it, 26/11/2026). A field is heard from the side it sits on.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "../constants/core.hpp"
#include "../models/sound.hpp"

namespace grmh
{
// the angle (radians) of the k-th field of the ring (0 the lowest), out of `ring` fields: 0 in front of the player,
// positive on their right
inline double ring_angle(int k, int ring)
{
    const double step = 2.0 * PI / ring;
    const int side = (k + 1) / 2;                 // 0, 1, 1, 2, 2, ...
    return (k % 2 == 1 ? -1.0 : 1.0) * side * step; // 0, left, right, left, right, ...
}

// where a field at `angle` sounds, from -1 (left) to +1 (right)
inline double stereo_position(double angle) { return std::sin(angle); }

// the channels' gains for a position and a stereo width (0 mono, 1 a field on the side hard in one channel), at
// constant power, the middle at 1 in each channel (as loud in each as the mono render)
inline void stereo_gains(double position, double width, double& left, double& right)
{
    const double phi = (std::clamp(width, 0.0, 1.0) * position + 1.0) * PI / 4.0;
    left = std::sqrt(2.0) * std::cos(phi);
    right = std::sqrt(2.0) * std::sin(phi);
}

// the position of each field of these notes (the ding first, the others in any order)
inline std::vector<double> field_positions(const std::vector<int>& notes)
{
    std::vector<double> positions(notes.size(), 0.0);
    std::vector<size_t> ring;
    for (size_t i = 1; i < notes.size(); ++i)
        ring.push_back(i);
    std::sort(ring.begin(), ring.end(), [&](size_t a, size_t b) { return notes[a] < notes[b]; });
    for (size_t k = 0; k < ring.size(); ++k)
        positions[ring[k]] = stereo_position(ring_angle((int)k, (int)ring.size()));
    return positions;
}

// the fields' gains of a sound model, for these notes and this width: a change of width solves no network
inline void apply_layout(SoundModel& sound, const std::vector<int>& notes, double width)
{
    const std::vector<double> positions = field_positions(notes);
    for (size_t i = 0; i < notes.size() && i < (size_t)MAX_FIELDS; ++i)
        stereo_gains(positions[i], width, sound.fields[i].left_gain, sound.fields[i].right_gain);
}
} // namespace grmh
