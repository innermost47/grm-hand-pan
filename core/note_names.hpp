// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Note names to MIDI numbers: "D3", "Bb3", "F#4" -> 50, 58, 66.
#pragma once

#include <stdexcept>
#include <string>

namespace grmh
{
inline int midi_of_name(const std::string& name)
{
    static const int semitone[7] = {9, 11, 0, 2, 4, 5, 7}; // A B C D E F G
    if (name.size() < 2 || name[0] < 'A' || name[0] > 'G')
        throw std::invalid_argument("not a note name: " + name);
    int note = semitone[name[0] - 'A'];
    size_t i = 1;
    if (name[i] == '#')
        ++note, ++i;
    else if (name[i] == 'b')
        --note, ++i;
    return note + 12 * (std::stoi(name.substr(i)) + 1);
}
} // namespace grmh
