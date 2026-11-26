// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
#include "SessionState.h"

#include "../../constants/state.hpp"

using namespace grmh;

// a double written with the 17 significant digits that bring it back bit for bit (the XML's own text keeps fewer)
juce::var SessionState::exact(double value)
{
    char text[32];
    std::snprintf(text, sizeof text, "%.17g", value);
    return juce::String(text);
}

double SessionState::exact(const juce::var& value, double fallback)
{
    return value.isVoid() ? fallback : std::strtod(value.toString().toRawUTF8(), nullptr);
}

juce::ValueTree SessionState::write_slot(SlotId id, const PatternSlot& slot)
{
    const Pattern& p = slot.pattern;
    juce::ValueTree tree(state::SLOT);
    tree.setProperty(state::BANK, id.bank, nullptr)
        .setProperty(state::INDEX, id.slot, nullptr)
        .setProperty(state::NAME, juce::String(slot.name), nullptr)
        .setProperty(state::NUMERATOR, p.signature.numerator, nullptr)
        .setProperty(state::DENOMINATOR, p.signature.denominator, nullptr)
        .setProperty(state::STEPS_PER_BEAT, p.steps_per_beat, nullptr)
        .setProperty(state::BARS, p.bars, nullptr);
    for (const PatternHit& h : p.hits)
    {
        juce::ValueTree hit(state::HIT);
        hit.setProperty(state::STEP, h.step, nullptr)
            .setProperty(state::FIELD, h.field, nullptr)
            .setProperty(state::X, exact(h.x), nullptr)
            .setProperty(state::Y, exact(h.y), nullptr)
            .setProperty(state::SIZE, exact(h.size), nullptr)
            .setProperty(state::HAND, (int)h.hand, nullptr);
        tree.appendChild(hit, nullptr);
    }
    return tree;
}

PatternSlot SessionState::read_slot(const juce::ValueTree& tree)
{
    PatternSlot slot;
    Pattern& p = slot.pattern;
    slot.name = tree.getProperty(state::NAME).toString().toStdString();
    p.signature.numerator = tree.getProperty(state::NUMERATOR, p.signature.numerator);
    p.signature.denominator = tree.getProperty(state::DENOMINATOR, p.signature.denominator);
    p.steps_per_beat = tree.getProperty(state::STEPS_PER_BEAT, p.steps_per_beat);
    p.bars = tree.getProperty(state::BARS, p.bars);
    for (const auto& hit : tree)
    {
        if (!hit.hasType(state::HIT))
            continue;
        PatternHit h;
        h.step = hit.getProperty(state::STEP);
        h.field = hit.getProperty(state::FIELD);
        h.x = exact(hit.getProperty(state::X), h.x);
        h.y = exact(hit.getProperty(state::Y), h.y);
        h.size = exact(hit.getProperty(state::SIZE), h.size);
        h.hand = (HandChoice)juce::jlimit(0, 2, (int)hit.getProperty(state::HAND));
        p.hits.push_back(h);
    }
    return slot;
}

void SessionState::write(juce::ValueTree& tree, const PatternLibrary& library, const PlayerHands& hands)
{
    juce::ValueTree saved(state::LIBRARY);
    saved.setProperty(state::CURRENT_BANK, library.current().bank, nullptr)
        .setProperty(state::CURRENT_SLOT, library.current().slot, nullptr);
    const PatternSlot untouched{};
    for (int b = 0; b < BANKS; ++b)
        for (int s = 0; s < PATTERNS_PER_BANK; ++s)
            if (!(library.slot({b, s}) == untouched))
                saved.appendChild(write_slot({b, s}, library.slot({b, s})), nullptr);
    tree.appendChild(saved, nullptr);

    juce::ValueTree player(state::HANDS);
    player.setProperty(state::LEFT_HANDED, hands.handedness == Handedness::left_handed, nullptr)
        .setProperty(state::WEAK_HAND_SPEED, exact(hands.weak_hand_speed), nullptr);
    tree.appendChild(player, nullptr);
}

void SessionState::read(juce::ValueTree& tree, PatternLibrary& library, PlayerHands& hands)
{
    const juce::ValueTree saved = tree.getChildWithName(state::LIBRARY);
    if (saved.isValid())
    {
        library = PatternLibrary{};
        for (const auto& slot : saved)
            if (slot.hasType(state::SLOT))
            {
                const SlotId id{(int)slot.getProperty(state::BANK), (int)slot.getProperty(state::INDEX)};
                if (PatternLibrary::valid(id))
                    library.slot(id) = read_slot(slot);
            }
        library.select({(int)saved.getProperty(state::CURRENT_BANK), (int)saved.getProperty(state::CURRENT_SLOT)});
        tree.removeChild(saved, nullptr);
    }
    const juce::ValueTree player = tree.getChildWithName(state::HANDS);
    if (player.isValid())
    {
        hands.handedness = (bool)player.getProperty(state::LEFT_HANDED) ? Handedness::left_handed
                                                                         : Handedness::right_handed;
        hands.weak_hand_speed = exact(player.getProperty(state::WEAK_HAND_SPEED), hands.weak_hand_speed);
        tree.removeChild(player, nullptr);
    }
}
