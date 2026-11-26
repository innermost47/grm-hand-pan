// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Which hand plays each hit of a pattern. There is no fixed rule among handpan players: each side is usually struck
// by its own hand, the fields in the middle (the ding, the field in front, the one at the back) by either, and
// beginners are taught to alternate. So: a hit the player gave a hand keeps it; a field on a side is struck by that
// side's hand; a field in the middle by the hand that did not play last (the strong hand leads). A player has two
// fingers to strike with: when one hand would strike several fields at once and the other none, the hit nearest the
// middle (the softer one if tied, the grace of a flam) goes to the free hand.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>

#include "../models/pattern.hpp"
#include "layout.hpp"

namespace grmh
{
inline Hand other_hand(Hand hand) { return hand == Hand::left ? Hand::right : Hand::left; }

inline Hand strong_hand(Handedness handedness)
{
    return handedness == Handedness::right_handed ? Hand::right : Hand::left;
}

class HandAssignment
{
public:
    // the hand of each hit of `pattern` (in the order of its hits), on an instrument of these notes (the ding first)
    static std::vector<Hand> assign(const Pattern& pattern, const std::vector<int>& notes, Handedness handedness)
    {
        const std::vector<double> positions = field_positions(notes);
        const auto& hits = pattern.hits;
        std::vector<size_t> order(hits.size());
        std::iota(order.begin(), order.end(), size_t{0});
        std::stable_sort(order.begin(), order.end(),
                         [&](size_t a, size_t b) { return hits[a].step < hits[b].step; });

        std::vector<Hand> hands(hits.size(), strong_hand(handedness));
        Hand last = other_hand(strong_hand(handedness)); // so that the strong hand leads
        for (size_t begin = 0; begin < order.size();)
        {
            size_t end = begin;
            while (end < order.size() && hits[order[end]].step == hits[order[begin]].step)
                ++end;
            const std::vector<size_t> step(order.begin() + (long)begin, order.begin() + (long)end);
            last = assign_step(hits, step, positions, last, hands);
            begin = end;
        }
        return hands;
    }

private:
    static double position_of(int field, const std::vector<double>& positions)
    {
        return field >= 0 && field < (int)positions.size() ? positions[(size_t)field] : 0.0;
    }

    // the hits of one step; returns the hand that played last there (the loudest hit: a flam's main stroke)
    static Hand assign_step(const std::vector<PatternHit>& hits, const std::vector<size_t>& step,
                            const std::vector<double>& positions, Hand last, std::vector<Hand>& hands)
    {
        std::array<int, 2> count{0, 0};
        std::vector<size_t> movable;
        const auto give = [&](size_t i, Hand hand)
        {
            hands[i] = hand;
            ++count[(size_t)hand];
        };
        for (size_t i : step) // the player's choice
            if (hits[i].hand != HandChoice::automatic)
                give(i, hits[i].hand == HandChoice::left ? Hand::left : Hand::right);
        std::vector<size_t> middle;
        for (size_t i : step) // each side by its hand
        {
            if (hits[i].hand != HandChoice::automatic)
                continue;
            movable.push_back(i);
            const double position = position_of(hits[i].field, positions);
            if (std::fabs(position) <= CENTRE_POSITION_TOLERANCE)
                middle.push_back(i);
            else
                give(i, position > 0.0 ? Hand::right : Hand::left);
        }
        for (size_t i : middle) // the middle by the hand less busy, else the one that did not play last
            give(i, count[0] < count[1] ? Hand::left : count[1] < count[0] ? Hand::right : other_hand(last));

        balance(hits, positions, movable, count, hands);

        size_t loudest = step.front();
        for (size_t i : step)
            if (hits[i].size >= hits[loudest].size)
                loudest = i;
        return hands[loudest];
    }

    // two fingers: one hand striking several fields while the other is free gives its hit nearest the middle away
    static void balance(const std::vector<PatternHit>& hits, const std::vector<double>& positions,
                        const std::vector<size_t>& movable, std::array<int, 2>& count, std::vector<Hand>& hands)
    {
        for (Hand busy : {Hand::left, Hand::right})
        {
            if (count[(size_t)busy] < 2 || count[(size_t)other_hand(busy)] > 0)
                continue;
            const size_t* chosen = nullptr;
            for (const size_t& i : movable)
            {
                if (hands[i] != busy)
                    continue;
                const auto nearer = [&](size_t a, size_t b)
                {
                    const double pa = std::fabs(position_of(hits[a].field, positions));
                    const double pb = std::fabs(position_of(hits[b].field, positions));
                    return pa < pb || (pa == pb && hits[a].size < hits[b].size);
                };
                if (chosen == nullptr || nearer(i, *chosen))
                    chosen = &i;
            }
            if (chosen == nullptr)
                continue;
            hands[*chosen] = other_hand(busy);
            --count[(size_t)busy];
            ++count[(size_t)other_hand(busy)];
        }
    }
};
} // namespace grmh
