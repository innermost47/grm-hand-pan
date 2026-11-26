// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "AudioExporter.h"

#include "../../constants/plugin.hpp"
#include "../../core/offline_renderer.hpp"

using namespace grmh;

namespace
{
constexpr int WAV_BITS = 24;

void tell(const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, title, message);
}
} // namespace

juce::AudioBuffer<float> AudioExporter::render(const SoundModel& sound, const CompiledPattern& played, double tempo,
                                               float gain)
{
    PatternSequencer sequencer;
    sequencer.set(&played.strikes, played.length_beats, played.loop);
    const auto strikes = sequencer.timeline(1, tempo);
    const double last = strikes.empty() ? 0.0 : strikes.back().time;
    const double seconds = std::max(played.length_beats * 60.0 / tempo, last) + plugin::TAIL_SECONDS;
    const auto channels = OfflineRenderer::render_stereo(sound, strikes, seconds);
    juce::AudioBuffer<float> audio(2, (int)channels[0].size());
    for (int c = 0; c < 2; ++c)
    {
        audio.copyFrom(c, 0, channels[(size_t)c].data(), audio.getNumSamples());
        audio.applyGain(c, 0, audio.getNumSamples(), gain);
    }
    return audio;
}

bool AudioExporter::write(const juce::File& file, const juce::AudioBuffer<float>& audio, double sample_rate)
{
    file.deleteFile();
    auto file_stream = std::make_unique<juce::FileOutputStream>(file);
    if (!file_stream->openedOk())
        return false;
    std::unique_ptr<juce::OutputStream> stream = std::move(file_stream);
    const auto options = juce::AudioFormatWriter::Options{}
                             .withSampleRate(sample_rate)
                             .withNumChannels(audio.getNumChannels())
                             .withBitsPerSample(WAV_BITS);
    juce::WavAudioFormat wav;
    const std::unique_ptr<juce::AudioFormatWriter> writer = wav.createWriterFor(stream, options); // takes the stream
    return writer != nullptr && writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples());
}

void AudioExporter::choose_and_export(HandPanProcessor& processor, What what)
{
    const auto sound = processor.sound_model();
    if (sound == nullptr)
    {
        tell("Export", "The instrument is still being solved: try again in a few seconds.");
        return;
    }
    const CompiledPattern played = what == What::song ? processor.compiled_song() : processor.compiled_pattern();
    if (played.strikes.empty())
    {
        tell("Export", what == What::song ? "The song is empty: add blocks to the arranger first."
                                          : "The pattern is empty: draw some hits first.");
        return;
    }
    const double tempo = processor.played_tempo();
    const float gain = processor.output_gain();
    const juce::String name = what == What::song ? "GRM Hand Pan - song.wav" : "GRM Hand Pan - pattern.wav";
    static std::unique_ptr<juce::FileChooser> chooser; // alive while the dialog is open
    chooser = std::make_unique<juce::FileChooser>(
        "Export to WAV", juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(name), "*.wav");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                             juce::FileBrowserComponent::warnAboutOverwriting,
                         [sound, played, tempo, gain](const juce::FileChooser& chosen)
                         {
                             const juce::File file = chosen.getResult().withFileExtension("wav");
                             if (file == juce::File{})
                                 return;
                             juce::Thread::launch(
                                 [sound, played, tempo, gain, file]
                                 {
                                     const auto audio = render(*sound, played, tempo, gain);
                                     const bool written = write(file, audio, sound->sample_rate);
                                     juce::MessageManager::callAsync(
                                         [written, file]
                                         {
                                             tell("Export", written ? "Written: " + file.getFullPathName()
                                                                    : "Could not write " + file.getFullPathName());
                                         });
                                 });
                         });
}
