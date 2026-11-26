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

// a MIDI note's name as a musician writes it, in UTF-8, flats for the black keys (a handpan's scales are written in
// flats): 58 -> "B♭3"
inline std::string name_of_midi(int midi)
{
    static const char* const names[12] = {"C", "D\xe2\x99\xad", "D", "E\xe2\x99\xad", "E", "F",
                                          "G\xe2\x99\xad", "G", "A\xe2\x99\xad", "A", "B\xe2\x99\xad", "B"};
    const int octave = midi / 12 - 1;
    return std::string(names[((midi % 12) + 12) % 12]) + std::to_string(octave);
}
} // namespace grmh
