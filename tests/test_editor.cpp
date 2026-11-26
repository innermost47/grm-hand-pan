// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The editor as a host opens it: its size, its widgets tied to the parameters (a knob moved is the parameter moved,
// the parameter moved is the knob moved), the signature chosen is the pattern's, the transport's buttons drive the
// plugin's clock.
#include <cstdio>

#include "../constants/plugin.hpp"
#include "../constants/sizes.hpp"
#include "../core/pattern_editor.hpp"
#include "../plugin/HandPanEditor.h"
#include "support/test_report.hpp"

using namespace grmh;

namespace
{
template <class T>
T* find_child(juce::Component& parent, const std::function<bool(T&)>& matches)
{
    for (auto* child : parent.getChildren())
    {
        if (auto* t = dynamic_cast<T*>(child); t != nullptr && matches(*t))
            return t;
        if (auto* deeper = find_child<T>(*child, matches))
            return deeper;
    }
    return nullptr;
}

// a mouse event on `c` at `at`, pressed at `down`
juce::MouseEvent mouse(juce::Component& c, juce::Point<float> at, juce::Point<float> down, juce::ModifierKeys mods,
                       bool dragged, int clicks = 1)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), at, mods,
                            juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation,
                            juce::MouseInputSource::defaultRotation, juce::MouseInputSource::defaultTiltX,
                            juce::MouseInputSource::defaultTiltY, &c, &c, now, down, now, clicks, dragged);
}

const juce::ModifierKeys LEFT{juce::ModifierKeys::leftButtonModifier};
const juce::ModifierKeys COMMAND{juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::commandModifier};

void click(SequencerView& roll, juce::Point<float> at, juce::ModifierKeys mods = LEFT)
{
    roll.mouseDown(mouse(roll, at, at, mods, false));
    roll.mouseUp(mouse(roll, at, at, mods, false));
}

void drag(SequencerView& roll, juce::Point<float> from, juce::Point<float> to, juce::ModifierKeys mods = LEFT)
{
    roll.mouseDown(mouse(roll, from, from, mods, false));
    roll.mouseDrag(mouse(roll, to, from, mods, true));
    roll.mouseUp(mouse(roll, to, from, mods, true));
}

// the roll's gestures and shortcuts, as Anthony validated them (26/11/2026)
void check_drawing(test::TestReport& report, HandPanProcessor& processor, SequencerView& roll)
{
    processor.transport().stop();
    processor.set_pattern(Pattern{});
    roll.refresh();
    click(roll, roll.field_point(0, 5));
    const auto& hits = [&]() -> const std::vector<PatternHit>& { return processor.pattern().hits; };
    report.expect(hits().size() == 1 && hits()[0].step == 0 && hits()[0].field == 5 &&
                      roll.selection() == std::vector<int>{0},
                  "a click on a field: a hit there, selected");

    drag(roll, roll.field_point(2, 4), roll.field_point(2, 4) - juce::Point<float>(0.0f, 80.0f));
    report.expect(hits().size() == 2 && hits()[1].size > 0.9, "a hit drawn with a drag up: loud");

    const double before = hits()[0].size;
    drag(roll, roll.hit_point(0), roll.hit_point(0) + juce::Point<float>(0.0f, 40.0f));
    report.expect(hits()[0].size < before && hits().size() == 2, "a drag down on a hit: softer, at any time");

    click(roll, roll.hit_point(1), COMMAND);
    report.expect(roll.selection().size() == 2, "Ctrl + click on a hit: added to the selection");
    drag(roll, roll.hit_point(1), roll.field_point(6, 4), COMMAND);
    report.expect(hits()[0].step == 4 && hits()[1].step == 6 && hits()[0].field == 5,
                  "Ctrl + drag: the selection moved four steps later, each on its field");

    roll.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
    drag(roll, roll.field_point(15, 0) + juce::Point<float>(-400.0f, -10.0f),
         roll.field_point(5, 0) + juce::Point<float>(400.0f, 30.0f), COMMAND);
    report.expect(roll.selection() == std::vector<int>{1}, "Ctrl + drag elsewhere: a rectangle selects what it covers");

    click(roll, roll.field_point(6, 2));
    click(roll, roll.field_point(6, 7));
    report.expect(hits().size() == 3 && PatternEditor::hits_at(processor.pattern(), 6) == 2,
                  "a third hit at a step: refused (two hands)");

    roll.keyPressed(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0));
    roll.keyPressed(juce::KeyPress(juce::KeyPress::deleteKey));
    report.expect(hits().empty(), "Ctrl + A, Delete: the pattern emptied");
    roll.keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0));
    report.expect(hits().size() == 3, "Ctrl + Z: back");
    roll.keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0));
    report.expect(hits().empty(), "Ctrl + Shift + Z: emptied again");
    roll.undo();

    roll.keyPressed(juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0));
    roll.keyPressed(juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0));
    click(roll, roll.field_point(10, 1));
    roll.keyPressed(juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)); // the hit the click drew
    roll.keyPressed(juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0));
    report.expect(hits().size() == 6 && roll.selection().size() == 3,
                  "Ctrl + C, a click at step 10, Ctrl + V: the three hits pasted there, selected");

    roll.mouseDoubleClick(mouse(roll, roll.hit_point(5), roll.hit_point(5), LEFT, false, 2));
    report.expect(hits().size() == 5, "a double-click on a hit: removed");
    click(roll, roll.hit_point(0));
    roll.clear();
    report.expect(hits().size() == 4, "Clear with a hit selected: that hit");
    roll.clear();
    report.expect(hits().empty(), "Clear with nothing selected: the whole pattern");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    test::TestReport report("editor");
    HandPanProcessor processor;
    processor.setPlayConfigDetails(0, 2, 48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    report.expect(editor != nullptr && editor->getWidth() == ui::DIMENSIONS.editor_width &&
                      editor->getHeight() == ui::DIMENSIONS.editor_height,
                  "the editor opens at its size");

    auto* volume = find_child<juce::Slider>(*editor, [](juce::Slider& s) { return s.getName() == "V"; });
    auto* width = find_child<juce::Slider>(*editor, [](juce::Slider& s) { return s.getName() == "W"; });
    report.expect(volume != nullptr && width != nullptr, "the V and W knobs are there");
    if (volume != nullptr && width != nullptr)
    {
        volume->setValue(-12.0, juce::sendNotificationSync);
        const float volume_db = processor.parameters().getRawParameterValue(plugin::PARAM_VOLUME)->load();
        report.expect(std::fabs(volume_db + 12.0f) < 0.05f, "V moved is the volume moved");
        // on the message thread, an attachment follows its parameter at once
        processor.parameters().getParameter(plugin::PARAM_STEREO_WIDTH)->setValueNotifyingHost(0.25f);
        report.expect(std::fabs(width->getValue() - 0.25) < 0.011, "the width moved by the host is W moved");
    }

    auto* lap = find_child<juce::ToggleButton>(*editor, [](juce::ToggleButton& b)
                                               { return b.getButtonText() == plugin::PARAM_ON_LAP_NAME; });
    report.expect(lap != nullptr && lap->getToggleState(), "the switch 'On the lap', on by default");

    auto* signature = find_child<juce::ComboBox>(*editor, [](juce::ComboBox& c) { return c.getText() == "4/4"; });
    report.expect(signature != nullptr, "the signature shows the pattern's: 4/4");
    if (signature != nullptr)
    {
        signature->setText("6/8", juce::sendNotificationSync);
        const Pattern& p = processor.pattern();
        report.expect(p.signature.numerator == 6 && p.signature.denominator == 8 && p.steps_per_bar() == 12,
                      "6/8 chosen: the pattern is in 6/8, twelve sixteenths a bar");
    }

    auto* play = find_child<TransportButton>(*editor, [](TransportButton& b) { return b.getName() == "Play"; });
    report.expect(play != nullptr, "the Play button is there");
    if (play != nullptr)
    {
        play->onClick(); // what a click runs (a click itself is delivered later, by the message loop)
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        processor.prepareToPlay(48000.0, 512);
        processor.processBlock(buffer, midi);
        processor.processBlock(buffer, midi);
        report.expect(processor.transport().shown_playing() && processor.transport().shown_beat() > 0.0,
                      "Play clicked: the plugin's clock runs");
    }
    // the roll: a left-hander's switch, the wheel through the bars
    auto* left_handed = find_child<juce::ToggleButton>(*editor, [](juce::ToggleButton& b)
                                                       { return b.getButtonText() == "Left-handed"; });
    report.expect(left_handed != nullptr && !left_handed->getToggleState(), "the roll's switch 'Left-handed', off");
    if (left_handed != nullptr)
    {
        left_handed->setToggleState(true, juce::dontSendNotification);
        left_handed->onClick();
        report.expect(processor.hands().handedness == Handedness::left_handed,
                      "switched on: the player leads with the left hand");
    }
    Pattern two_bars = processor.pattern();
    two_bars.bars = 2;
    processor.set_pattern(two_bars);
    auto& roll = dynamic_cast<HandPanEditor&>(*editor).sequencer_view();
    roll.scroll_by(two_bars.steps_per_bar());
    report.expect(roll.bottom_step() == two_bars.steps_per_bar(), "scrolled up a bar: the second bar");
    roll.scroll_by(10.0 * two_bars.steps_per_bar());
    report.expect(roll.bottom_step() == two_bars.steps_per_bar(), "never past the pattern's last bar");
    roll.scroll_by(-0.2);
    report.expect(roll.bottom_step() == two_bars.steps_per_bar() - 1, "a small turn down: a step down");

    // a hit glows when its step plays, then fades
    Pattern lit;
    PatternHit first;
    first.field = 0;
    lit.hits = {first};
    processor.set_pattern(lit);
    processor.transport().stop();
    processor.transport().play();
    juce::AudioBuffer<float> block(2, 512);
    juce::MidiBuffer none;
    processor.processBlock(block, none);
    roll.refresh();
    report.expect(roll.glow(0) == 1.0f, "the hit of the step playing lights up");
    juce::Thread::sleep(300);
    roll.refresh();
    report.expect(roll.glow(0) > 0.0f && roll.glow(0) < 1.0f, "then fades, as the note does");
    check_drawing(report, processor, roll);
    editor.reset();
    return report.finish();
}
