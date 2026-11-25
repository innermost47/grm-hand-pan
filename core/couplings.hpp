// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Which modes of different fields sing together: every pair that exchanges at least SYMPATHY_MIN_EXCHANGE of its
// amplitude (two oscillators detuned by dw and coupled at g exchange 2g / sqrt(4g^2 + dw^2)), the strongest first when
// there are more than the audio thread holds. Over one sample, a pair exchanges energy as two weakly coupled
// oscillators do: their complex amplitudes turn into each other by g / sample rate.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "../constants/sympathy.hpp"
#include "../models/sound.hpp"

namespace grmh
{
inline void find_couplings(const std::vector<ToneField>& fields, double coupling_rate, double sample_rate,
                           SoundModel& sound)
{
    struct Candidate
    {
        Coupling coupling;
        double exchange; // the share of amplitude the pair exchanges
    };
    std::vector<Candidate> candidates;
    if (coupling_rate > 0.0)
        for (size_t a = 0; a < fields.size(); ++a)
            for (size_t b = a + 1; b < fields.size(); ++b)
                for (size_t m = 0; m < fields[a].modes.size(); ++m)
                    for (size_t n = 0; n < fields[b].modes.size(); ++n)
                    {
                        const double dw =
                            2.0 * PI * (fields[a].modes[m].frequency.value - fields[b].modes[n].frequency.value);
                        const double exchange =
                            2.0 * coupling_rate / std::sqrt(4.0 * coupling_rate * coupling_rate + dw * dw);
                        if (exchange >= SYMPATHY_MIN_EXCHANGE)
                            candidates.push_back({{(int)a, (int)m, (int)b, (int)n, 1.0, 0.0}, exchange});
                    }
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& x, const Candidate& y) { return x.exchange > y.exchange; });
    const double angle = coupling_rate / sample_rate;
    sound.coupling_count = (int)std::min(candidates.size(), (size_t)MAX_COUPLINGS);
    for (int i = 0; i < sound.coupling_count; ++i)
    {
        Coupling c = candidates[(size_t)i].coupling;
        c.cos_angle = std::cos(angle);
        c.sin_angle = std::sin(angle);
        sound.couplings[(size_t)i] = c;
    }
}
} // namespace grmh
