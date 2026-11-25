// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The structs shared by the core (no JUCE).
#pragma once

#include <cmath>

namespace grmh
{
// a published value and its spread: the spread is relative (one standard deviation, or half the published range),
// and becomes the sigma of the measurement in a GRM network
struct Measured
{
    double value;
    double spread;

    // the sigma a network takes, in dex (log10 units), for a positive quantity
    double sigma_dex() const { return spread / std::log(10.0); }
    double low() const { return value * (1.0 - spread); }
    double high() const { return value * (1.0 + spread); }
};

// the three lowest modes of one tone field, as measured: (0,1), (1,1)a, (1,1)b in Hz
struct MeasuredToneField
{
    const char* note;
    double fundamental;
    double octave;
    double compound_fifth;
};

// T60 statistics of the three strongest modes of a group of tone fields (seconds)
struct DecayStatistics
{
    const char* group;
    double mean;
    double standard_deviation;
    double min;
    double max;
};
} // namespace grmh
