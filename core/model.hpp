// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// Everything the networks give for one set of settings: the tone fields, the cavity, and the sound model the audio
// thread plays. Solved on the solver thread, never on the audio thread.
#pragma once

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "../constants/audio.hpp"
#include "../models/sound.hpp"
#include "cavity_network.hpp"
#include "handpan_networks.hpp"
#include "strike_table.hpp"

namespace grmh
{
class Model
{
public:
    // the networks' results for these settings, and the sound model at this sample rate
    static Model solve(const Settings& settings, double sample_rate)
    {
        if (settings.notes.empty() || settings.notes.size() > (size_t)MAX_FIELDS)
            throw std::invalid_argument("a handpan has 1 to " + std::to_string(MAX_FIELDS) + " tone fields");
        if (!(sample_rate > 0.0))
            throw std::invalid_argument("the sample rate must be positive");

        Model model;
        model.cavity_ = solve_cavity(settings.on_lap ? CAVITY_RESONANCE_ON_LAP : CAVITY_RESONANCE_INTEGRAL_HANG,
                                     settings.temperature);
        for (const int note : settings.notes)
            model.fields_.push_back(solve_tone_field(note_frequency(note)));

        SoundModel& sound = model.sound_;
        sound.sample_rate = sample_rate;
        sound.field_count = (int)settings.notes.size();
        for (size_t i = 0; i < model.fields_.size(); ++i)
            sound.fields[i] = field_sound(settings.notes[i], model.fields_[i], sample_rate);
        sound.cavity = {model.cavity_.frequency.value, model.cavity_.quality_factor.value};
        const double air_density = ATMOSPHERIC_PRESSURE * M_AIR / (R_GAS * settings.temperature);
        sound.pressure_gain = air_density / (4.0 * PI * LISTENING_DISTANCE) / FULL_SCALE_PRESSURE;
        return model;
    }

    static double note_frequency(int note) { return 440.0 * std::pow(2.0, (note - 69) / 12.0); }

    const std::vector<ToneField>& fields() const { return fields_; }
    const Cavity& cavity() const { return cavity_; }
    const SoundModel& sound() const { return sound_; }

private:
    // a field ready to play: each mode's rotation per sample, and the velocity each MIDI velocity's strike gives it
    // (u = J S(f) / M: the strike's impulse at the mode's frequency, over the field's mass)
    static FieldSound field_sound(int note, const ToneField& field, double sample_rate)
    {
        FieldSound sound;
        sound.note = note;
        const double radius = field.effective_radius.value;
        sound.area = PI * radius * radius;
        const double mass = STEEL_DENSITY.value * SHELL_THICKNESS.value * sound.area;
        const auto& strikes = strike_table();
        for (size_t m = 0; m < field.modes.size(); ++m)
        {
            const double frequency = field.modes[m].frequency.value;
            ModeSound& mode = sound.modes[m];
            mode.angular_frequency = 2.0 * PI * frequency;
            mode.decay_rate = std::log(1000.0) / field.modes[m].t60.value; // -60 dB in T60
            const double decay = std::exp(-mode.decay_rate / sample_rate);
            const double angle = mode.angular_frequency / sample_rate;
            mode.rotation_re = decay * std::cos(angle);
            mode.rotation_im = decay * std::sin(angle);
            for (int velocity = 1; velocity < MIDI_VELOCITIES; ++velocity)
            {
                const Strike& strike = strikes[(size_t)velocity];
                mode.strike_velocity[(size_t)velocity] =
                    strike.impulse.value * expected_strike_spectrum(frequency, strike.contact_time) / mass;
            }
        }
        return sound;
    }

    std::vector<ToneField> fields_;
    Cavity cavity_{};
    SoundModel sound_;
};
} // namespace grmh
