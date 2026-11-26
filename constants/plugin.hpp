// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin's parameters and timings.
#pragma once

namespace grmh::plugin
{
constexpr const char* PARAM_ON_LAP = "on_lap";
constexpr const char* PARAM_ON_LAP_NAME = "On the lap";
constexpr bool ON_LAP_DEFAULT = true; // the instrument on the player's knees (Anthony, 26/11/2026)
constexpr const char* PARAM_STEREO_WIDTH = "stereo_width";
constexpr const char* PARAM_STEREO_WIDTH_NAME = "Stereo width";
constexpr double STEREO_WIDTH_STEP = 0.01; // a model is patched per step, no finer
constexpr const char* PARAM_VOLUME = "volume";
constexpr const char* PARAM_VOLUME_NAME = "Volume";
constexpr float VOLUME_MIN_DB = -60.0f; // and below: silence
constexpr float VOLUME_MAX_DB = 12.0f;
constexpr float VOLUME_DEFAULT_DB = 0.0f;
constexpr double VOLUME_RAMP_SECONDS = 0.02; // a change of volume glides, never clicks
constexpr const char* PARAM_TEMPO = "tempo";
constexpr const char* PARAM_TEMPO_NAME = "Tempo";

constexpr int SOLVER_PERIOD_MS = 30;          // how often the solver looks at the settings
constexpr int SOLVER_STOP_TIMEOUT_MS = 60000; // the first calibrations take seconds: let them end
constexpr double TAIL_SECONDS = 8.0;          // a ding rings that long after its last strike
} // namespace grmh::plugin
