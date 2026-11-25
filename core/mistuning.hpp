// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// No instrument is tuned exactly: a tuner sets each fundamental on a strobe tuner, the overtones land near their
// ratios. Two coupled modes that should coincide beat at sqrt((g / pi)^2 + df^2): their coupling g splits them even in
// unison, their mistuning df adds. [A15] measured 3.1-8.6 Hz at 248-662 Hz (Table 4.3), mostly a fundamental against
// a neighbour's octave. One instrument is one draw: each mode above the fundamental moves by a share drawn from the
// spread those beatings leave once the coupling is taken out, the same at every run for a given seed.
#pragma once

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "../constants/core.hpp"
#include "../constants/sympathy.hpp"
#include "../models/core.hpp"

namespace grmh
{
// the rms relative mistuning of the measured beatings, at coupling rate g (1/s)
inline double mistuning_spread(double coupling_rate)
{
    const double split = coupling_rate / PI; // Hz: the beating of two coupled modes in exact unison
    double sum = 0.0;
    for (const Beating& b : MEASURED_BEATINGS)
    {
        const double df = std::sqrt(std::max(0.0, b.rate * b.rate - split * split));
        sum += (df / b.frequency) * (df / b.frequency);
    }
    return std::sqrt(sum / MEASURED_BEATINGS.size());
}

// the modes of these fields, as one instrument (`seed`) has them: the fundamentals exact, the others mistuned
inline void apply_mistuning(std::vector<ToneField>& fields, unsigned seed, double spread)
{
    if (spread <= 0.0)
        return;
    std::mt19937 random(seed);
    std::normal_distribution<double> share(0.0, spread);
    for (ToneField& field : fields)
        for (size_t m = 1; m < field.modes.size(); ++m)
            field.modes[m].frequency.value *= 1.0 + share(random);
}
} // namespace grmh
