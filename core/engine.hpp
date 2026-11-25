// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The real-time part: the voices of the fields, the air of the cavity, the pressure that reaches the listener. It
// reads a SoundModel and never allocates, locks or solves a network.
#pragma once

#include <array>

#include "../models/sound.hpp"
#include "cavity_resonator.hpp"
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
        if (field >= 0)
            voices_[(size_t)field].strike(midi_velocity);
    }

    // the pressure at the listener, in full scale, for `count` samples: p = rho / (4 pi r) dU/dt, dU/dt the volume
    // acceleration of the fields out of the shell and of the Gu's plug
    void render(float* out, int count)
    {
        for (int n = 0; n < count; ++n)
        {
            if (sound_ == nullptr)
            {
                out[n] = 0.0f;
                continue;
            }
            double acceleration = 0.0, breathing = 0.0;
            for (int i = 0; i < sound_->field_count; ++i)
            {
                const FieldMotion motion = voices_[(size_t)i].step();
                acceleration += motion.volume_acceleration;
                breathing += motion.breathing;
            }
            acceleration += cavity_.step(breathing);
            out[n] = (float)(sound_->pressure_gain * acceleration);
            exchange();
        }
    }

    // a mode's amplitude (m/s), for views and tests
    double amplitude(int field, int mode) const { return voices_[(size_t)field].amplitude((size_t)mode); }

    void silence()
    {
        for (auto& voice : voices_)
            voice.silence();
        cavity_.silence();
    }

private:
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
};
} // namespace grmh
