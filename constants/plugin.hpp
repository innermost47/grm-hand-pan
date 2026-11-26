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
constexpr const char* PARAM_MIDI_MODE = "midi_mode";
constexpr const char* PARAM_MIDI_MODE_NAME = "MIDI mode";
constexpr const char* MIDI_MODE_CHOICES[] = {"Keys play notes", "Keys play patterns"};
constexpr int MIDI_MODE_NOTES = 0, MIDI_MODE_PATTERNS = 1;
// in the patterns' mode, this key plays the first pattern of bank A, and the next ones the following patterns, bank
// after bank (C1 with middle C = C3, as grm-guitare; A = C1-D#2, B = E2-G3, C = G#3-B4, D = C5-D#6)
constexpr int PATTERN_KEY_FIRST = 36;
// the arranger's song played instead of the pattern being edited (the keys' patterns wait while it is on)
constexpr const char* PARAM_SONG = "song";
constexpr const char* PARAM_SONG_NAME = "Song";

constexpr int SOLVER_PERIOD_MS = 30;          // how often the solver looks at the settings
constexpr int SOLVER_STOP_TIMEOUT_MS = 60000; // the first calibrations take seconds: let them end
constexpr double TAIL_SECONDS = 8.0;          // a ding rings that long after its last strike
} // namespace grmh::plugin
