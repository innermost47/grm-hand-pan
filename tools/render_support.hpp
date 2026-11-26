// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// What the renderers share: the settings of a D Kurd from the command line, and writing a rendering with its peak.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../constants/scales.hpp"
#include "../core/wav_writer.hpp"
#include "../models/sound.hpp"

namespace grmh
{
// a D Kurd, on the lap if `--lap` is given; the output path is the first other argument, or `fallback`
inline Settings d_kurd_from_arguments(int argc, char** argv, std::string& path, const char* fallback)
{
    Settings settings;
    settings.notes.assign(D_KURD.begin(), D_KURD.end());
    path = fallback;
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "--lap") == 0)
            settings.on_lap = true;
        else
            path = argv[i];
    return settings;
}

// writes the channels and says how loud they peak and how long they last
inline void write_rendering(const std::string& path, const std::vector<std::vector<float>>& channels, int sample_rate)
{
    float peak = 0.0f;
    for (const auto& channel : channels)
        for (const float s : channel)
            peak = std::max(peak, std::fabs(s));
    WavWriter::write(path, channels, sample_rate);
    std::printf("peak %.1f dBFS, wrote %s (%.1f s, %zu channels)\n", 20 * std::log10(peak + 1e-30f), path.c_str(),
                channels.front().size() / double(sample_rate), channels.size());
}
} // namespace grmh
