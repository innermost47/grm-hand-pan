// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin as a host sees it, without a host: create it, prepare it, keep calling processBlock while the solver
// works, then play it from MIDI: a note sounds, finite and below full scale; a note on the right sounds on the right;
// a note no field is tuned to is silent; it stops and plays again; its state is saved and restored.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>

#include "../constants/plugin.hpp"
#include "../core/arrangement_editor.hpp"
#include "../plugin/services/AudioExporter.h"
#include "../constants/scales.hpp"
#include "../plugin/HandPanProcessor.h"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
constexpr double RATE = 48000.0;
constexpr int BLOCK = 512;

struct Heard
{
    double left = 0.0, right = 0.0, peak = 0.0;
    bool finite = true;
};

// plays `blocks` blocks, a note-on at the start of the first if `note` >= 0
Heard play(HandPanProcessor& plugin, int note, int velocity, int blocks)
{
    Heard heard;
    juce::AudioBuffer<float> buffer(2, BLOCK);
    for (int b = 0; b < blocks; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0 && note >= 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)velocity), 17);
        buffer.clear();
        plugin.processBlock(buffer, midi);
        for (int n = 0; n < BLOCK; ++n)
        {
            const float l = buffer.getSample(0, n), r = buffer.getSample(1, n);
            heard.finite = heard.finite && std::isfinite(l) && std::isfinite(r);
            heard.left += double(l) * l;
            heard.right += double(r) * r;
            heard.peak = std::max(heard.peak, (double)std::max(std::fabs(l), std::fabs(r)));
        }
    }
    return heard;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juce_init;
    test::TestReport report("plugin host");
    HandPanProcessor plugin;
    const auto* on_lap = plugin.getParameters().size() > 0
                             ? dynamic_cast<juce::AudioParameterBool*>(plugin.getParameters()[0])
                             : nullptr;
    report.expect(on_lap != nullptr && on_lap->getName(64) == grmh::plugin::PARAM_ON_LAP_NAME && on_lap->get(),
                  "the handpan is played on the lap by default");
    plugin.setPlayConfigDetails(0, 2, RATE, BLOCK);
    plugin.prepareToPlay(RATE, BLOCK);

    // the host keeps calling processBlock while the solver works: silence, never a crash
    const auto start = std::chrono::steady_clock::now();
    bool silent_while_waiting = true;
    while (!plugin.instrument_ready() &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(120))
    {
        silent_while_waiting = silent_while_waiting && play(plugin, -1, 0, 1).peak == 0.0;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const double waited = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("  the instrument was ready after %.1f s\n", waited);
    report.expect(plugin.instrument_ready(), "the solver solves the instrument");
    report.expect(silent_while_waiting, "silent, and alive, while it does");
    play(plugin, -1, 0, 2); // the model is taken between two blocks

    const Heard ding = play(plugin, D_KURD[0], 100, 40);
    std::printf("  ding: peak %.1f dBFS\n", 20.0 * std::log10(ding.peak + 1e-30));
    report.expect(ding.finite && ding.peak > 1e-3 && ding.peak < 1.0, "a note-on sounds, finite, below full scale");

    play(plugin, -1, 0, 400); // let it fade
    const Heard e4 = play(plugin, D_KURD[5], 100, 40);
    report.expect(e4.right > 2.0 * e4.left, "E4, on the player's right, sounds on the right");

    play(plugin, -1, 0, 400);
    const Heard stranger = play(plugin, 61, 100, 10);
    report.expect(stranger.peak < 0.05 * ding.peak, "a note no field is tuned to adds nothing");

    // the roll's pattern plays on the plugin's own clock, without a MIDI note
    play(plugin, -1, 0, 400);
    Pattern pattern;
    PatternHit hit;
    hit.field = 0;
    hit.size = 0.8;
    pattern.hits = {hit};
    plugin.set_pattern(pattern);
    plugin.transport().play();
    const Heard rolled = play(plugin, -1, 0, 40);
    plugin.transport().stop();
    report.expect(rolled.finite && rolled.peak > 1e-3, "the roll's pattern plays when Play is pressed");

    // the volume
    play(plugin, -1, 0, 400);
    auto* volume = plugin.parameters().getParameter(grmh::plugin::PARAM_VOLUME);
    volume->setValueNotifyingHost(volume->convertTo0to1(-20.0f));
    play(plugin, -1, 0, 4); // the gain glides there
    const Heard quiet = play(plugin, D_KURD[0], 100, 40);
    std::printf("  ding at -20 dB: %.1f dB under\n", 20.0 * std::log10(ding.peak / quiet.peak));
    report.expect(std::fabs(20.0 * std::log10(ding.peak / quiet.peak) - 20.0) < 0.5, "V at -20 dB: 20 dB quieter");
    volume->setValueNotifyingHost(volume->convertTo0to1(0.0f));

    // keys play patterns: C1 + 1 plays the pattern A2 from the bar line, let go it stops; a key below C1 adds nothing
    plugin.set_pattern(Pattern{});
    PatternSlot second;
    second.pattern = pattern; // a ding on its first step
    plugin.set_slot({0, 1}, second);
    auto* mode = plugin.parameters().getParameter(grmh::plugin::PARAM_MIDI_MODE);
    mode->setValueNotifyingHost(mode->convertTo0to1((float)grmh::plugin::MIDI_MODE_PATTERNS));
    play(plugin, -1, 0, 400);
    plugin.transport().stop();
    plugin.transport().play();
    const Heard keyed = play(plugin, grmh::plugin::PATTERN_KEY_FIRST + 1, 100, 40);
    report.expect(keyed.finite && keyed.peak > 1e-3 && plugin.shown_pattern() == 1,
                  "keys play patterns: C#1 held, the pattern A2 plays from the bar line");
    {
        juce::AudioBuffer<float> buffer(2, BLOCK);
        juce::MidiBuffer off;
        off.addEvent(juce::MidiMessage::noteOff(1, grmh::plugin::PATTERN_KEY_FIRST + 1), 0);
        plugin.processBlock(buffer, off);
    }
    report.expect(plugin.shown_pattern() == -1, "the key let go: the pattern stops");
    play(plugin, -1, 0, 400);
    const Heard below = play(plugin, grmh::plugin::PATTERN_KEY_FIRST - 6, 100, 10);
    report.expect(below.peak < 0.05 * ding.peak, "keys play patterns: a key below C1 strikes nothing");
    plugin.transport().stop();
    mode->setValueNotifyingHost(mode->convertTo0to1((float)grmh::plugin::MIDI_MODE_NOTES));

    // the song: a bar of silence, then A2 (a ding on its first step): silent first, the ding on the second bar
    Arrangement arrangement;
    ArrangementEditor::add_rest(arrangement);
    ArrangementEditor::add_pattern(arrangement, {0, 1});
    plugin.set_arrangement(arrangement);
    auto* song = plugin.parameters().getParameter(grmh::plugin::PARAM_SONG);
    song->setValueNotifyingHost(1.0f);
    play(plugin, -1, 0, 400);
    plugin.transport().stop();
    plugin.transport().play();
    const int bar_blocks = (int)(4.0 * 60.0 / TEMPO_DEFAULT * RATE / BLOCK); // whole blocks within the first bar
    const Heard rest = play(plugin, -1, 0, bar_blocks);
    const Heard second_bar = play(plugin, -1, 0, 40);
    std::printf("  song: first bar %.1f dBFS, second bar %.1f dBFS\n", 20.0 * std::log10(rest.peak + 1e-30),
                20.0 * std::log10(second_bar.peak + 1e-30));
    report.expect(rest.peak < 0.05 * second_bar.peak && second_bar.peak > 1e-3,
                  "Song on: the first bar is silent (what still rings aside), the ding sounds on the second");
    plugin.transport().stop();
    song->setValueNotifyingHost(0.0f);

    // the song exported to a WAV file and read back: stereo, its length and its ringing, not silent
    {
        const auto sound = plugin.sound_model();
        const CompiledPattern compiled = plugin.compiled_song();
        const double tempo = plugin.played_tempo();
        const auto audio = AudioExporter::render(*sound, compiled, tempo, plugin.output_gain());
        const juce::File file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                    .getChildFile("grm_hand_pan_export_test.wav");
        const bool written = AudioExporter::write(file, audio, sound->sample_rate);
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        const double expected = compiled.length_beats * 60.0 / tempo + grmh::plugin::TAIL_SECONDS;
        bool loud = false;
        if (reader != nullptr)
        {
            juce::AudioBuffer<float> back(2, (int)reader->lengthInSamples);
            reader->read(&back, 0, back.getNumSamples(), 0, true, true);
            loud = back.getMagnitude(0, back.getNumSamples()) > 1e-3f;
        }
        report.expect(written && reader != nullptr && reader->numChannels == 2 && reader->bitsPerSample == 24 &&
                          std::fabs(reader->lengthInSamples / reader->sampleRate - expected) < 0.01 && loud,
                      "the song exported: a 24-bit stereo WAV, its length plus the ringing, not silent");
        reader.reset();
        file.deleteFile();
    }

    // stop and play again
    plugin.releaseResources();
    plugin.prepareToPlay(RATE, BLOCK);
    const Heard again = play(plugin, D_KURD[4], 90, 20);
    report.expect(again.finite && again.peak > 1e-3, "stopped and prepared again, it plays");

    // the state goes and comes back
    juce::MemoryBlock state;
    plugin.getStateInformation(state);
    plugin.setStateInformation(state.getData(), (int)state.getSize());
    report.expect(state.getSize() > 0, "its state is saved and restored");
    return report.finish();
}
