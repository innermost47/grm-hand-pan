// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The plugin's editor as a host opens it, a short pattern in its roll (the ding as a pulse, answers on both sides, a
// flam, ghost notes), saved as a PNG for Anthony to look at. Usage: render_editor [out.png]
#include "../core/arrangement_editor.hpp"
#include "../plugin/HandPanEditor.h"
#include "png_capture.hpp"

namespace
{
grmh::Pattern demo_pattern()
{
    grmh::Pattern p;
    p.bars = 2;
    const auto hit = [](int step, int field, double size, double x, double y)
    {
        grmh::PatternHit h;
        h.step = step;
        h.field = field;
        h.size = size;
        h.x = x;
        h.y = y;
        return h;
    };
    // fields: 0 D3 (ding), 1 A3, 2 Bb3, 3 C4, 4 D4, 5 E4, 6 F4, 7 G4, 8 A4
    p.hits = {hit(0, 0, 0.9, 0.0, 0.0),   hit(2, 4, 0.4, -0.3, 0.2), hit(3, 6, 0.3, 0.2, -0.3),
              hit(4, 1, 0.6, 0.3, 0.1),   hit(6, 5, 0.5, 0.0, 0.3),  hit(7, 0, 0.15, -0.5, 0.5),
              hit(8, 0, 0.8, 0.1, -0.1),  hit(10, 3, 0.5, 0.2, 0.2), hit(11, 7, 0.35, -0.2, 0.0),
              hit(12, 5, 0.9, 0.0, 0.0),  hit(12, 5, 0.3, 0.5, -0.4), // a flam
              hit(14, 8, 0.45, 0.2, 0.3), hit(15, 2, 0.2, -0.4, -0.2)};
    return p;
}


// plays the plugin's own clock up to `beat` (quarter notes), as a host would call it
void play_to(HandPanProcessor& processor, double beat)
{
    constexpr double RATE = 48000.0;
    constexpr int BLOCK = 512;
    processor.prepareToPlay(RATE, BLOCK);
    processor.transport().play();
    juce::AudioBuffer<float> buffer(2, BLOCK);
    juce::MidiBuffer midi;
    const double seconds = beat * 60.0 / grmh::TEMPO_DEFAULT;
    for (int done = 0; done < (int)(seconds * RATE); done += BLOCK)
        processor.processBlock(buffer, midi);
}
} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    HandPanProcessor processor;
    processor.set_pattern(demo_pattern());
    processor.set_slot({0, 1}, {"Verse", demo_pattern()});
    processor.set_slot({0, 2}, {"Fill", demo_pattern()});
    grmh::Arrangement song;
    grmh::ArrangementEditor::add_pattern(song, {0, 0});
    grmh::ArrangementEditor::set_count(song, 0, 2);
    grmh::ArrangementEditor::add_pattern(song, {0, 1});
    grmh::ArrangementEditor::add_pattern(song, {1, 1}); // Verse's variation B
    grmh::ArrangementEditor::add_rest(song);
    grmh::ArrangementEditor::add_pattern(song, {0, 2});
    grmh::ArrangementEditor::select(song, 1, 2); // Verse and its variation, looped
    grmh::ArrangementEditor::set_loop(song, true);
    processor.set_arrangement(song);
    play_to(processor, 3.0); // the third beat: the flam on E4 glows
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    auto& window = dynamic_cast<HandPanEditor&>(*editor);
    window.arranger_view().refresh();
    window.sequencer_view().refresh();
    return grmh::tools::capture_png(*editor, argc > 1 ? argv[1] : nullptr, "renders/editor.png");
}
