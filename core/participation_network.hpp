// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// How strongly a strike at the sweet spot excites each kind of mode, solved from recorded notes [MUMT22]. A plate
// struck at the apex of its dome would hardly excite the modes whose nodal lines cross there [A15]; real fields, with
// their domes and elliptic outlines, do excite their octave and fifth, and hardly their untuned modes. The model
// renders each recorded note with every participation at 1 and measures it with the meter of the recordings: the
// difference is one measurement of the participation, weighed with the spread from field to field [A15].
#pragma once

#include <cmath>
#include <map>
#include <stdexcept>
#include <string>

#include "../constants/recordings.hpp"
#include "grm.hpp"
#include "mode_levels.hpp"
#include "model.hpp"
#include "network_support.hpp"
#include "offline_renderer.hpp"

namespace grmh
{
// the sample rate the recordings are measured at
constexpr double CALIBRATION_SAMPLE_RATE = 44100.0;

// the modes' levels against the fundamental, rendered with every participation at 1, for each recorded note
inline std::array<ModeLevels, RECORDED_NOTES.size()> unit_participation_levels()
{
    std::array<ModeLevels, RECORDED_NOTES.size()> levels{};
    for (size_t i = 0; i < RECORDED_NOTES.size(); ++i)
    {
        Settings settings;
        settings.notes = {RECORDED_NOTES[i].note};
        const Model model = Model::solve(settings, CALIBRATION_SAMPLE_RATE, Participation{});
        const auto rendered = OfflineRenderer::render(
            model.sound(), {{0.0, RECORDED_NOTES[i].note, MIDI_VELOCITY_REFERENCE}}, 1.0);
        levels[i] =
            ModeLevelMeter::measure(rendered, CALIBRATION_SAMPLE_RATE, Model::note_frequency(RECORDED_NOTES[i].note));
    }
    return levels;
}

inline Participation solve_participation()
{
    const auto model = unit_participation_levels();
    grm::Network net;
    const char* kinds[3] = {"octave", "fifth", "untuned"};
    for (const char* kind : kinds)
        net.add_node(kind, 1.0);
    const double field_dex = FIELD_LEVEL_SPREAD_DB / 20.0; // an amplitude, in dex
    std::map<std::string, grm::Measurement> measured;
    for (size_t i = 0; i < RECORDED_NOTES.size(); ++i)
    {
        const RecordedNote& r = RECORDED_NOTES[i];
        const double gaps[3] = {r.octave - model[i].octave, r.fifth - model[i].fifth, r.untuned - model[i].untuned};
        for (int k = 0; k < 3; ++k)
        {
            // what this note says of the participation: the recording over the model, as an amplitude ratio
            const std::string node = std::string(kinds[k]) + " of " + r.name;
            net.add_node(node, 1.0);
            net.add_log(node, {{node, 1}, {kinds[k], -1}}, 0.0, 1e-9);
            measured[node] = {std::pow(10.0, gaps[k] / 20.0), field_dex};
        }
    }
    // a note sees the participation of its field: the notes share one participation per kind, each note a measurement
    // of it. Their relation is exact; the spread is in the measurements.
    const auto result = net.solve(measured);
    if (!result.converged)
        throw std::runtime_error("the participation network did not converge");
    return {network::measured(result, net, "octave"), network::measured(result, net, "fifth"),
            network::measured(result, net, "untuned")};
}

// the participation solved once for the whole program (its inputs are recordings)
inline const Participation& participation()
{
    static const Participation solved = solve_participation();
    return solved;
}
} // namespace grmh
