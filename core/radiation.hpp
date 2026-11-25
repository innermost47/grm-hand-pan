// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// How much of a mode's volume velocity reaches the listener. A field vibrating in a mode with n nodal diameters is
// 2^n parts of equal area moving in opposite phases: a monopole (n = 0), a dipole of two half discs (n = 1), a
// lateral quadrupole of four quarter discs (n = 2) [KF00]. The centroids of two neighbouring parts are 8a / (3 pi)
// apart (the centroid of a half or quarter disc lies 4a / (3 pi) from its straight edges). Their sum, averaged in
// energy over all directions, against a monopole of the whole field: 1 for a monopole, (kd)^n times a geometric
// factor while the field is small against the wavelength, 1 / sqrt(2^n) when its parts radiate on their own.
#pragma once

#include <cmath>
#include <complex>

#include "../constants/core.hpp"

namespace grmh
{
// the distance between the centroids of two neighbouring parts of a field of radius `radius`
inline double part_spacing(double radius) { return 8.0 * radius / (3.0 * PI); }

// the pressure a mode of `multipole_order` radiates, over that of a monopole of the same field, at wavenumber
// k = 2 pi f / c, for a field of radius `radius`: the rms of the parts' sum over the sphere
inline double radiation_factor(int multipole_order, double wavenumber, double radius)
{
    if (multipole_order <= 0)
        return 1.0;
    const double d = part_spacing(radius);
    const int parts = multipole_order == 1 ? 2 : 4;
    constexpr int THETA_STEPS = 128, PHI_STEPS = 256; // resolves kd up to ~40, a handpan reaches ~4 at 4 kHz
    double energy = 0.0, weights = 0.0;
    for (int i = 0; i < THETA_STEPS; ++i)
    {
        const double theta = PI * (i + 0.5) / THETA_STEPS, w = std::sin(theta); // solid angle weight
        for (int j = 0; j < PHI_STEPS; ++j)
        {
            const double phi = 2.0 * PI * (j + 0.5) / PHI_STEPS;
            const double ux = std::sin(theta) * std::cos(phi), uy = std::sin(theta) * std::sin(phi);
            // the parts at (+-d/2) on x (dipole), or (+-d/2, +-d/2) with alternating signs (quadrupole)
            const double sx = 2.0 * std::sin(0.5 * wavenumber * d * ux);
            const double sy = multipole_order == 1 ? 1.0 : 2.0 * std::sin(0.5 * wavenumber * d * uy);
            const double sum = sx * sy; // |sum of the parts' phases|
            energy += w * sum * sum;
            weights += w;
        }
    }
    return std::sqrt(energy / weights) / parts;
}
} // namespace grmh
