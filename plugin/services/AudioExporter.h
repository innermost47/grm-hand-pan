// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The pattern being edited, or the song, written to a WAV file: rendered offline by the same engine as the plugin
// (the instrument as last solved, the tempo it plays at, the volume), once, with the ringing after the last strike;
// stereo, 24 bits. The rendering runs on a thread of its own: the interface never waits for it.
#pragma once

#include <functional>

#include "../HandPanProcessor.h"
#include "../JuceHeader.h"

class AudioExporter
{
public:
    enum class What
    {
        pattern,
        song
    };

    // `played` at `tempo` (a pattern once, a song once) and its ringing, at `gain`, in two channels
    static juce::AudioBuffer<float> render(const grmh::SoundModel& sound, const grmh::CompiledPattern& played,
                                           double tempo, float gain);

    // `audio` written as a 24-bit WAV file at `sample_rate`; false when the file cannot be written
    static bool write(const juce::File& file, const juce::AudioBuffer<float>& audio, double sample_rate);

    // asks where to write, then renders and writes on a thread of its own; tells how it went
    static void choose_and_export(HandPanProcessor& processor, What what);
};
