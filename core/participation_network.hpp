// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// How strongly a strike at the sweet spot excites each kind of mode, solved from recorded notes [MUMT22]. A plate
// struck at the apex of its dome would hardly excite the modes whose nodal lines cross there [A15]; real fields, with
// their domes and elliptic outlines, do excite their octave and fifth, and hardly their untuned modes. The model
// renders each recorded note with every participation at 1 and measures it with the meter of the recordings: a mode's
// level scales with its participation, so the difference is one measurement of it, weighed with the spread from field
// to field [A15]. The finger's click is not solved here: at the reference strike the plate theory's click hardly
// reaches above what the modes put there as they start, so the recordings' attack cannot weigh it (tools/probe_click).
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

// what the model gives a recorded note with every participation at 1, and its attack without the click
struct UnitLevels
{
    ModeLevels levels;
    double attack_without_click = 0.0; // dB
};

inline UnitLevels unit_participation_levels(const RecordedNote& note)
{
    const auto measure = [&](const Calibration& calibration)
    {
        Settings settings;
        settings.notes = {note.note};
        const Model model = Model::solve(settings, CALIBRATION_SAMPLE_RATE, calibration);
        const auto rendered =
            OfflineRenderer::render(model.sound(), {{0.0, note.note, MIDI_VELOCITY_REFERENCE}}, 1.0);
        return ModeLevelMeter::measure(rendered, CALIBRATION_SAMPLE_RATE, Model::note_frequency(note.note));
    };
    Calibration silent_finger;
    silent_finger.participation.click = {0.0, 0.0};
    return {measure(Calibration{}), measure(silent_finger).attack};
}

inline Participation solve_participation()
{
    grm::Network net;
    const char* kinds[3] = {"octave", "fifth", "untuned"};
    for (const char* kind : kinds)
        net.add_node(kind, 1.0);
    const double field_dex = FIELD_LEVEL_SPREAD_DB / 20.0; // an amplitude, in dex
    std::map<std::string, grm::Measurement> measured;
    for (const RecordedNote& r : RECORDED_NOTES)
    {
        const ModeLevels model = unit_participation_levels(r).levels;
        // a mode's level scales with its participation: the recording over the model, as an amplitude ratio
        const double gaps[3] = {r.octave - model.octave, r.fifth - model.fifth, r.untuned - model.untuned};
        for (int k = 0; k < 3; ++k)
        {
            const std::string node = std::string(kinds[k]) + " of " + r.name;
            net.add_node(node, 1.0);
            net.add_log(node, {{node, 1}, {kinds[k], -1}}, 0.0, 1e-9);
            measured[node] = {std::pow(10.0, gaps[k] / 20.0), field_dex};
        }
    }
    // the notes share one participation per kind, each note a measurement of it: their relations are exact, the
    // spread is in the measurements
    const auto result = net.solve(measured);
    if (!result.converged)
        throw std::runtime_error("the participation network did not converge");
    Participation solved{network::measured(result, net, "octave"), network::measured(result, net, "fifth"),
                         network::measured(result, net, "untuned")};
    return solved; // the click keeps the plate theory's weight, 1
}

// the participation solved once for the whole program (its inputs are recordings)
inline const Participation& participation()
{
    static const Participation solved = solve_participation();
    return solved;
}
} // namespace grmh
