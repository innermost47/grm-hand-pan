// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin's parameters and timings.
#pragma once

namespace grmh::plugin
{
constexpr const char* PARAM_ON_LAP = "on_lap";
constexpr const char* PARAM_ON_LAP_NAME = "On the lap";
constexpr const char* PARAM_STEREO_WIDTH = "stereo_width";
constexpr const char* PARAM_STEREO_WIDTH_NAME = "Stereo width";
constexpr double STEREO_WIDTH_STEP = 0.01; // a model is patched per step, no finer

constexpr int SOLVER_PERIOD_MS = 30;          // how often the solver looks at the settings
constexpr int SOLVER_STOP_TIMEOUT_MS = 60000; // the first calibrations take seconds: let them end
constexpr double TAIL_SECONDS = 8.0;          // a ding rings that long after its last strike
} // namespace grmh::plugin
