// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The instrument as it is played: the model of these settings, its modes excited as the recordings say, its fields
// singing together as [A15] measured.
#pragma once

#include "model.hpp"
#include "participation_network.hpp"
#include "sympathy_network.hpp"

namespace grmh
{
// what the measurements calibrate, solved once
inline Calibration calibration() { return {participation(), coupling_rate().value, instrument_mistuning()}; }

inline Model solve_instrument(const Settings& settings, double sample_rate)
{
    return Model::solve(settings, sample_rate, calibration());
}
} // namespace grmh
