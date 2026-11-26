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
