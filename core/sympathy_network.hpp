// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// How fast two modes of different fields at the same frequency exchange their energy (g, 1/s), solved from two
// measurements of [A15]:
// - a coupled mode reaches its maximum 209 ms after the strike (Table 4.4): two tuned modes hand over all their energy
//   in a quarter of their exchange, pi / (2 g);
// - a D4 strike with its neighbours free carries 4.7 dB more energy on average than with them damped (Table 4.7, four
//   handpans, their spread as the sigma). The network renders a D4 strike on a D Kurd with its neighbours and alone
//   (the neighbours damped) to compare.
#pragma once

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "../constants/scales.hpp"
#include "../constants/sympathy.hpp"
#include "couplings.hpp"
#include "grm.hpp"
#include "model.hpp"
#include "network_support.hpp"
#include "offline_renderer.hpp"
#include "participation_network.hpp"

namespace grmh
{
constexpr double SYMPATHY_LISTEN = 3.0;        // s of a strike whose energy is summed
constexpr int SYMPATHY_PROBE_NOTE = D_KURD[4]; // D4, the field [A15] measured

inline double signal_energy(const std::vector<float>& samples)
{
    return std::accumulate(samples.begin(), samples.end(), 0.0, [](double e, float s) { return e + double(s) * s; });
}

// the onset of a coupled mode, true within its resolution, earlier rather than later [A15 Table 4.4]
inline grm::Measurement coupled_onset()
{
    return network::between(SYMPATHY_COUPLED_ONSET - SYMPATHY_ONSET_RESOLUTION, SYMPATHY_COUPLED_ONSET);
}

// the coupling the onset alone gives (two tuned modes hand over their energy in pi / (2 g)): what the mistuning is
// solved with, before the energy gain joins it
inline double onset_coupling_rate() { return PI / (2.0 * coupled_onset().value); }

// a D4 strike on a D Kurd, with its neighbours and alone, the instrument mistuned by `mistuning_spread`: the fields are
// solved once, the coupling rate changes only which modes exchange and how fast
class SympathyProbe
{
public:
    explicit SympathyProbe(double mistuning_spread = mistuning_spread(onset_coupling_rate()))
    {
        const Calibration calibration{participation(), 0.0, mistuning_spread};
        Settings all, alone;
        all.notes.assign(D_KURD.begin(), D_KURD.end());
        alone.notes = {SYMPATHY_PROBE_NOTE};
        all_ = Model::solve(all, CALIBRATION_SAMPLE_RATE, calibration);
        alone_energy_ = energy(Model::solve(alone, CALIBRATION_SAMPLE_RATE, calibration).sound());
    }

    // the energy (dB) the strike gains when the other fields are free to sing, at coupling rate g
    double energy_gain(double coupling_rate) const
    {
        SoundModel sound = all_.sound();
        find_couplings(all_.fields(), coupling_rate, CALIBRATION_SAMPLE_RATE, sound);
        return 10.0 * std::log10(energy(sound) / alone_energy_);
    }

private:
    static double energy(const SoundModel& sound)
    {
        return signal_energy(
            OfflineRenderer::render(sound, {{0.0, SYMPATHY_PROBE_NOTE, MIDI_VELOCITY_REFERENCE}}, SYMPATHY_LISTEN));
    }

    Model all_;
    double alone_energy_ = 0.0;
};

// the measured gain: the mean of the four handpans and their standard deviation (dB)
inline Measured measured_sympathy_gain_db()
{
    const auto n = static_cast<double>(SYMPATHY_ENERGY_GAINS_DB.size());
    const double mean = std::accumulate(SYMPATHY_ENERGY_GAINS_DB.begin(), SYMPATHY_ENERGY_GAINS_DB.end(), 0.0) / n;
    double variance = 0.0;
    for (const double g : SYMPATHY_ENERGY_GAINS_DB)
        variance += (g - mean) * (g - mean) / (n - 1);
    return {mean, std::sqrt(variance)};
}

inline Measured solve_coupling_rate()
{
    const SympathyProbe probe;
    const Measured gain_db = measured_sympathy_gain_db();
    grm::Network net;
    net.add_node("g", 7.0);
    net.add_node("onset", SYMPATHY_COUPLED_ONSET);
    net.add_node("gain", std::pow(10.0, gain_db.value / 10.0));
    // two tuned modes hand over their energy in a quarter of their exchange: onset g = pi / 2
    net.add_log("energy handed over", {{"onset", 1}, {"g", 1}}, std::log10(PI / 2.0), 1e-9);
    const int ig = net.index("g"), igain = net.index("gain");
    net.add_fn(
        "energy the neighbours add", {"g", "gain"},
        [&probe, ig, igain](const grm::Values& v) { return probe.energy_gain(v[ig]) / 10.0 - std::log10(v[igain]); },
        1e-4);
    // an energy ratio in dB is a tenth of a dex
    const auto r = net.solve(
        {{"gain", {std::pow(10.0, gain_db.value / 10.0), gain_db.spread / 10.0}}, {"onset", coupled_onset()}});
    if (!r.converged)
        throw std::runtime_error("the sympathy network did not converge");
    return network::measured(r, net, "g");
}

// the coupling rate solved once for the whole program (its inputs are measurements)
inline const Measured& coupling_rate()
{
    static const Measured solved = solve_coupling_rate();
    return solved;
}

// the mistuning of an instrument, the coupling taken out of the measured beatings
inline double instrument_mistuning() { return mistuning_spread(onset_coupling_rate()); }
} // namespace grmh
