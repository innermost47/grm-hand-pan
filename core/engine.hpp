// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The real-time part: the voices of the fields, the air of the cavity, the fingers on the steel, the pressure that
// reaches the listener. It reads a SoundModel and never allocates, locks or solves a network.
#pragma once

#include <array>

#include "../models/sound.hpp"
#include "cavity_resonator.hpp"
#include "finger_contact.hpp"
#include "voice.hpp"

namespace grmh
{
class Engine
{
public:
    // plays `sound` from now on (a model from the mailbox); the voices keep ringing
    void set_model(const SoundModel* sound)
    {
        sound_ = sound;
        if (sound_ == nullptr)
            return;
        for (int i = 0; i < MAX_FIELDS; ++i)
            voices_[(size_t)i].set_sound(i < sound_->field_count ? &sound_->fields[(size_t)i] : nullptr);
        cavity_.set(sound_->cavity.frequency, sound_->cavity.quality_factor, sound_->sample_rate);
    }

    // a finger strikes the field tuned to `note`, at a MIDI velocity; a note no field is tuned to is silent
    void strike(int note, int midi_velocity)
    {
        if (sound_ == nullptr)
            return;
        const int field = sound_->field_of(note);
        if (field < 0 || midi_velocity <= 0 || midi_velocity >= MIDI_VELOCITIES)
            return;
        voices_[(size_t)field].strike(midi_velocity);
        const FieldSound& struck = sound_->fields[(size_t)field];
        contacts_.press(sound_->contact.length[(size_t)midi_velocity], sound_->contact.peak_force[(size_t)midi_velocity],
                        &struck.click_residual, struck.click_tail, struck.left_gain, struck.right_gain);
    }

    // the pressure at the listener, in full scale, for `count` samples: the modes' p = rho / (4 pi r) dU/dt, dU/dt the
    // volume acceleration of the fields out of the shell and of the Gu's plug, and the fingers' click
    void render(float* out, int count)
    {
        for (int n = 0; n < count; ++n)
            out[n] = (float)step().mono;
    }

    // the same in two channels: each field and its click from where it sits, the Gu in the middle
    void render_stereo(float* left, float* right, int count)
    {
        for (int n = 0; n < count; ++n)
        {
            const Pressure p = step();
            left[n] = (float)p.left;
            right[n] = (float)p.right;
        }
    }

    // a mode's amplitude (m/s), for views and tests
    double amplitude(int field, int mode) const { return voices_[(size_t)field].amplitude((size_t)mode); }

    void silence()
    {
        for (auto& voice : voices_)
            voice.silence();
        cavity_.silence();
        contacts_.release();
    }

private:
    // the pressure of one sample (full scale), heard as one and in two channels
    struct Pressure
    {
        double mono = 0.0, left = 0.0, right = 0.0;
    };

    Pressure step()
    {
        Pressure p;
        if (sound_ == nullptr)
            return p;
        double breathing = 0.0;
        for (int i = 0; i < sound_->field_count; ++i)
        {
            const FieldSound& field = sound_->fields[(size_t)i];
            const FieldMotion motion = voices_[(size_t)i].step();
            const double field_pressure = sound_->pressure_gain * motion.volume_acceleration;
            p.mono += field_pressure;
            p.left += field.left_gain * field_pressure;
            p.right += field.right_gain * field_pressure;
            breathing += motion.breathing;
        }
        const double gu = sound_->pressure_gain * cavity_.step(breathing);
        double click_left = 0.0, click_right = 0.0;
        const double click = contacts_.step(click_left, click_right);
        p.mono += gu + sound_->contact.click_gain * click;
        p.left += gu + sound_->contact.click_gain * click_left;
        p.right += gu + sound_->contact.click_gain * click_right;
        exchange();
        return p;
    }

    // the fields sing together: each coupled pair of modes exchanges its energy over one sample
    void exchange()
    {
        for (int i = 0; i < sound_->coupling_count; ++i)
        {
            const Coupling& c = sound_->couplings[(size_t)i];
            FieldVoice::exchange(voices_[(size_t)c.field_a], (size_t)c.mode_a, voices_[(size_t)c.field_b],
                                 (size_t)c.mode_b, c.cos_angle, c.sin_angle);
        }
    }

    const SoundModel* sound_ = nullptr;
    std::array<FieldVoice, MAX_FIELDS> voices_{};
    CavityResonator cavity_;
    FingerContacts contacts_;
};
} // namespace grmh
