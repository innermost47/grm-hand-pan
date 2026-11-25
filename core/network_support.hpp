// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What every network of the instrument shares: reading a result as a Measured, measuring a value known only to lie
// between two bounds.
#pragma once

#include <cmath>
#include <string>

#include "../models/core.hpp"
#include "grm.hpp"

namespace grmh::network
{
// a solved positive node, with its relative uncertainty
inline Measured measured(const grm::Result& result, const grm::Network& net, const std::string& node)
{
    const auto i = static_cast<size_t>(net.index(node));
    return {result.values[i], result.uncertainty[i]};
}

// a positive value known to lie between two bounds: their geometric mean, with half their ratio as sigma (in dex)
inline grm::Measurement between(double low, double high)
{
    return {std::sqrt(low * high), 0.5 * std::fabs(std::log10(high / low))};
}
} // namespace grmh::network
