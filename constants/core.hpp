// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The measured data of the instrument, each value with its source and its spread (the spread is the sigma of the
// measurement in the networks: do not tighten it to get a number). Sources:
//   [RS07]  F. Rohner, S. Schaerer, "History, development and tuning of the HANG", Proc. ISMA 2007, Barcelona.
//   [MR07]  A. Morrison, T. D. Rossing, "Modes of vibration and sound radiation from the Hang",
//           Archives of Acoustics 32(3), 551-560 (2007).
//   [WMR08] D. Wessel, A. Morrison, T. D. Rossing, "Sound of the HANG", Proc. Mtgs. Acoust. 4, 035002 (2008),
//           doi:10.1121/1.3068630.
//   [A15]   E. Alon, "Analysis and synthesis of the handpan sound", MSc by research, University of York (2015),
//           anechoic recordings of four handpans from three makers, struck at the 'D' sweet spot.
//   [EC3]   EN 1993-1-1 (Eurocode 3) 3.2.6, design values of structural steel; EN 1991-1-1 Table A.4, unit weight.
//   [L69]   A. W. Leissa, "Vibration of plates", NASA SP-160 (1969), circular plates.
//   [PB]    PANArt, "Hang" book (English, 2011), section "Helmholtz resonance",
//           https://panart.ch/files/documents/PANArt_Buch_EN_110522.pdf
//   [PU]    PANArt, Hang Urgu product page: "a height of 24 cm, a diameter of 52 cm",
//           https://panart.ch/en/instruments/hang-urgu
//   [KF00]  L. E. Kinsler, A. R. Frey et al., "Fundamentals of Acoustics", 4th ed. (2000), Helmholtz resonator.
//   [CODATA] the molar gas constant (exact since 2019).
#pragma once

#include <array>

#include "../models/core.hpp"

namespace grmh
{
constexpr double PI = 3.14159265358979323846;

// ---- the steel. The nitrided sheet of PANArt has a "high Young's modulus" [RS07] with no figure given: the values
// of structural steel are used, with a spread that covers what the standards and the software in use give.
constexpr Measured STEEL_YOUNG_MODULUS{210.0e9, 0.025}; // Pa, [EC3] 210 GPa; 205 GPa elsewhere in practice
constexpr Measured STEEL_POISSON_RATIO{0.30, 0.03};     // [EC3]
constexpr Measured STEEL_DENSITY{7925.0, 0.01};         // kg/m3, [EC3] 77.0-78.5 kN/m3 = 7850-8000 kg/m3

// ---- the shell [RS07]: "the playing surface of the ding shell (50 cm diameter with uniform thickness of
// 0.91-0.95 mm)"; the spread is half the range
constexpr Measured SHELL_THICKNESS{0.93e-3, 0.0215};  // m
constexpr double SHELL_THICKNESS_MIN = 0.91e-3;       // m
constexpr double SHELL_THICKNESS_MAX = 0.95e-3;       // m
constexpr double DING_SHELL_DIAMETER = 0.50;          // m

// ---- the tone fields: three tuned modes in the ratios 1:2:3 ([MR07] 3, [WMR08] 4, [A15] 4.2.1). The spread is
// the one of the measurements below (checked by test_constants)
constexpr double OCTAVE_RATIO = 2.0;
constexpr double COMPOUND_FIFTH_RATIO = 3.0;
constexpr double MODE_RATIO_SPREAD = 0.03;

constexpr std::array<MeasuredToneField, 5> MEASURED_TONE_FIELDS{{
    {"G3 central, high-voice Hang [MR07]", 189.0, 390.0, 593.0},
    {"F3 central, low-voice Hang [MR07]", 174.0, 348.0, 520.0},
    {"F#4, low-voice Hang [MR07]", 368.0, 738.0, 1104.0},
    {"D3 central, Integral Hang [WMR08]", 148.0, 295.0, 445.0},
    {"D3 damped, handpan 3 [A15]", 152.2, 298.1, 449.2},
}};

// the modes above the three tuned ones are not tuned: (2,1)a and (2,1)b of the G3 field at 1418 and 1543 Hz [MR07],
// a fourth resonance of the Integral Hang D3 at 1344 Hz [WMR08]: "the higher modes came to lie around the third
// octave due to the stiffening effect of the dome" [RS07]
constexpr std::array<double, 3> UNTUNED_MODE_RATIOS{1418.0 / 189.0, 1543.0 / 189.0, 1344.0 / 148.0};
// their spread covers the Integral Hang's 9.08 from the G3's 8.16
constexpr double UNTUNED_RATIO_SPREAD = 0.10;

// ---- the modes of a field, and how each radiates [MR07, section 4]: "The active intensity maps show monopole
// radiation characteristics at the fundamental and second harmonic frequencies ... The intensity field at the third
// resonance frequency exhibits a dipole pattern". The (2,1) modes have two nodal diameters [MR07]: quadrupoles.
constexpr std::array<FieldModeShape, 5> FIELD_MODE_SHAPES{{
    {"(0,1) fundamental", 1.0, 0.0, 0},
    {"(1,1)a octave", OCTAVE_RATIO, MODE_RATIO_SPREAD, 0},
    {"(1,1)b compound fifth", COMPOUND_FIFTH_RATIO, MODE_RATIO_SPREAD, 1},
    {"(2,1)a", UNTUNED_MODE_RATIOS[0], UNTUNED_RATIO_SPREAD, 2},
    {"(2,1)b", UNTUNED_MODE_RATIOS[1], UNTUNED_RATIO_SPREAD, 2},
}};

// the peaks of [A15] at four times a field's fundamental (the double octave, 0.6-1.3 kHz), with their T60 (Tables
// 6.2-6.4, 6.6 and 6.7-6.8, 6.10, 6.12): measured decays above the tuned modes of their field
constexpr std::array<DecayMeasurement, 6> HIGH_PEAK_DECAYS{{
    {1178.0, 4, 2.3}, // instrument 1, D4
    {787.2, 4, 5.0},  // instrument 2, G3
    {1049.0, 4, 1.8}, // instrument 2, C4
    {1253.0, 4, 1.7}, // instrument 2, Eb4
    {588.6, 4, 2.6},  // instrument 3, D3
    {1323.0, 4, 1.7}, // instrument 4, E4
}};

// ---- the tone field as a plate: f1 = lambda^2 / (2 pi a^2) * sqrt(D / (rho h)), D = E h^3 / (12 (1 - nu^2)).
// The edge of a field is "soft" [RS07], neither clamped nor simply supported: the frequency parameter lambda^2 is the
// geometric mean of the two circular plate values [L69], its spread half their ratio in dex, so that the effective
// radius carries this uncertainty.
constexpr double PLATE_LAMBDA2_CLAMPED = 10.2158;          // [L69] clamped circular plate, fundamental
constexpr double PLATE_LAMBDA2_SIMPLY_SUPPORTED = 4.935;   // [L69] simply supported, nu = 0.3

// ---- the losses of the tuned modes: the law of LossLaw is solved from DECAY_MEASUREMENTS. Each measured T60 weighs
// with the spread of one tone field around the mean of all instruments, [A15] Table 4.2 (1.0 s over 2.9 s)
constexpr double LOSS_REFERENCE_FREQUENCY = 440.0; // Hz

// ---- the decay of the three strongest modes, undamped, [A15] Table 4.2
constexpr DecayStatistics DECAY_ALL{"all instruments", 2.9, 1.0, 0.9, 5.9};
constexpr std::array<DecayStatistics, 3> DECAY_BY_REGISTER{{
    {"low (B2-B3)", 3.2, 0.9, 1.6, 5.1},
    {"mid (C4-E4)", 3.0, 1.2, 1.2, 5.9},
    {"high (F4-C5)", 2.5, 0.9, 0.9, 4.2},
}};

// ---- the cavity (Helmholtz) resonance: "a small peak at 85 Hz, the frequency of the cavity resonance (near F2)" on
// the Integral Hang [WMR08]; "about 85 Hz ... an appropriate positioning on the player's lap can lower the resonance
// to the ding's octave (74 Hz)" [PB]; PANArt tunes it to D2 on the later generation [RS07]. The Gu opening and
// neck are not published: the cavity network solves the effective neck from the frequency (decision of 25/11/2026).
constexpr Measured CAVITY_RESONANCE_INTEGRAL_HANG{85.0, 0.02}; // Hz, read on a spectrum
constexpr Measured CAVITY_RESONANCE_ON_LAP{74.0, 0.02};        // Hz [PB]
constexpr double CAVITY_RESONANCE_TARGET_NOTE_MIDI = 38.0;     // D2 [RS07]

// ---- the vessel [PU]: overall height and diameter; the cavity inside is the ding shell's diameter [RS07]
constexpr double HANG_HEIGHT = 0.24;   // m
constexpr double HANG_DIAMETER = 0.52; // m, with the brass flange ring
constexpr double MEASURED_DIMENSION_SPREAD = 0.01; // a dimension published to the centimetre

// the shape of the two shells is not published: the cavity volume V = s D^2 H lies between two cones (s = pi / 12)
// and two half ellipsoids (s = pi / 6); s is their geometric mean, its spread half their ratio in dex
constexpr double CAVITY_SHAPE_CONES = PI / 12.0;
constexpr double CAVITY_SHAPE_ELLIPSOIDS = PI / 6.0;

// the radiation of the neck [KF00]: Q = r sqrt(V (L'/S)^3), r = 2 pi for a flanged opening, 4 pi for an unflanged
// one; the Gu sits in the curved shell, between the two: r is their geometric mean, its spread half their ratio
constexpr double NECK_RADIATION_FLANGED = 2.0 * PI;
constexpr double NECK_RADIATION_UNFLANGED = 4.0 * PI;

// ---- the air: c^2 = gamma R T / M
constexpr double GAMMA_AIR = 1.4;            // diatomic gas
constexpr double R_GAS = 8.314462618;        // J/(mol K) [CODATA]
constexpr double M_AIR = 0.028965;           // kg/mol, dry air
constexpr double ROOM_TEMPERATURE = 293.15;  // K, 20 degrees C
} // namespace grmh
