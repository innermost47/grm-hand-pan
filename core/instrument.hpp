// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The instrument as it is played: the model of these settings, its modes excited as the recordings say.
#pragma once

#include "model.hpp"
#include "participation_network.hpp"

namespace grmh
{
inline Model solve_instrument(const Settings& settings, double sample_rate)
{
    return Model::solve(settings, sample_rate, participation());
}
} // namespace grmh
