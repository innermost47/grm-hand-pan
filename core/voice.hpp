// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The voice of one tone field: a bank of damped complex oscillators, one per tuned mode. Each oscillator z is the
// mode's velocity (its real part, m/s); its acceleration is (i w - a) z, exact, never a difference of samples. A
// strike adds to the velocity, so a field struck while it rings keeps ringing.
#pragma once

#include <array>

#include "../models/sound.hpp"

namespace grmh
{
// what a field pushes into the air during one sample
struct FieldMotion
{
    double volume_acceleration = 0.0; // m3/s2, all its modes: what the listener hears
    double breathing = 0.0;           // m3/s, its fundamental alone: the only mode that changes the cavity's volume
};

class FieldVoice
{
public:
    // reads `sound` from now on; the modes keep ringing through a new model
    void set_sound(const FieldSound* sound) { sound_ = sound; }

    // a strike at a MIDI velocity (1-127)
    void strike(int midi_velocity)
    {
        if (sound_ == nullptr || midi_velocity <= 0 || midi_velocity >= MIDI_VELOCITIES)
            return;
        for (size_t m = 0; m < re_.size(); ++m)
            re_[m] += sound_->modes[m].strike_velocity[(size_t)midi_velocity];
    }

    // advances one sample
    FieldMotion step()
    {
        FieldMotion motion;
        if (sound_ == nullptr)
            return motion;
        double acceleration = 0.0;
        for (size_t m = 0; m < re_.size(); ++m)
        {
            const ModeSound& mode = sound_->modes[m];
            const double re = re_[m] * mode.rotation_re - im_[m] * mode.rotation_im;
            im_[m] = re_[m] * mode.rotation_im + im_[m] * mode.rotation_re;
            re_[m] = re;
            acceleration -= mode.radiating_area * (mode.angular_frequency * im_[m] + mode.decay_rate * re_[m]);
        } // Re((i w - a) z), times the air the mode moves
        motion.volume_acceleration = acceleration;
        motion.breathing = sound_->area * re_[0];
        return motion;
    }

    void silence()
    {
        re_.fill(0.0);
        im_.fill(0.0);
    }

private:
    const FieldSound* sound_ = nullptr;
    std::array<double, FIELD_MODES> re_{}, im_{};
};
} // namespace grmh
