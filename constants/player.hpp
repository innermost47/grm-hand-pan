// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The measured data of the player's finger, each value with its source and its spread. Sources:
//   [HH97]   A. Z. Hajian, R. D. Howe, "Identification of the mechanical impedance at the human finger tip",
//            J. Biomech. Eng. 119(1), 109-114 (1997), as quoted by M. K. O'Malley, A. Gupta, "Haptic interfaces"
//            (2008): "equivalent mass ... 3.5 to 8.7 g ... damping and stiffness increased linearly with force",
//            fingertip forces 2-20 N.
//   [J03]    D. L. Jindrich, Y. Zhou, T. Becker, J. T. Dennerlein, "Non-linear viscoelastic models predict fingertip
//            pulp force-displacement characteristics during voluntary tapping", J. Biomech. 36(4), 497-503 (2003),
//            abstract: "very rapid (less than 5 ms) force transients ... Energy dissipation by the fingertip averaged
//            81%, and varied little (<3%) across conditions".
//   [JBD04]  D. L. Jindrich, A. D. Balakrishnan, J. T. Dennerlein, "Finger joint impedance during tapping on a
//            computer keyswitch", J. Biomech. (2004): the force transducer of these measurements "had ... a resonant
//            frequency of 1000 Hz".
//   [K06]    P. L. Kuo, D. L. Lee, D. L. Jindrich, J. T. Dennerlein, "Finger joint coordination during tapping",
//            J. Biomech. 39 (2006): fingertip velocity at the start of contact 0.45 (SD 0.04) m/s.
//   [D16]    J. Dallard, "Modelisation du doigt dans un contexte de manipulation fine : une approche elements finis
//            et experimentale", PhD thesis, Universite Claude Bernard Lyon 1 (2016), HAL tel-01356196: index pulp
//            deflection under a plane, by MRI, 7 subjects (Table 4.6): from 2 N to 4 N the pulp goes 0.17 mm (all
//            subjects, 15 degrees) to 0.28-0.29 mm (45 degrees; 15 degrees without two subjects) deeper, "strain
//            hardening"; "une reponse fortement non-lineaire avec une raideur variable qui evolue avec le chargement".
#pragma once

namespace grmh
{
// ---- the finger, as a mass on the pulp [HH97]
constexpr double FINGER_MASS_MIN = 3.5e-3; // kg
constexpr double FINGER_MASS_MAX = 8.7e-3; // kg

// ---- the pulp's force transient: shorter than 5 ms [J03], and no shorter than the 1 ms period of the force
// transducer that measured it [JBD04]: its duration lies between the two
constexpr double PULP_CONTACT_MIN = 1.0e-3; // s
constexpr double PULP_CONTACT_MAX = 5.0e-3; // s

// ---- the pulp hardens [D16]: doubling the force (2 N to 4 N) presses it this much deeper. An exponential pulp,
// F = a exp(b d), is then as stiff as it is loaded, k = b F, b = ln(2) / step: the stiffness [HH97] saw grow
// linearly with the force
constexpr double PULP_FORCE_RATIO = 2.0;
constexpr double PULP_DEFLECTION_STEP_MIN = 0.17e-3; // m
constexpr double PULP_DEFLECTION_STEP_MAX = 0.29e-3; // m

// ---- the energy the pulp dissipates in a tap [J03]: the rest bounces back, e^2 = 1 - dissipated
constexpr double PULP_ENERGY_DISSIPATED = 0.81;
constexpr double PULP_ENERGY_DISSIPATED_SPREAD = 0.03;

// ---- the fingertip velocity at contact of a tap on a keyswitch [K06]
constexpr double TAP_VELOCITY_REFERENCE = 0.45;        // m/s
constexpr double TAP_VELOCITY_REFERENCE_SPREAD = 0.09; // relative, 0.04 / 0.45

// ---- the fingertip forces over which [HH97] measured the finger
constexpr double FINGERTIP_FORCE_MIN = 2.0;  // N
constexpr double FINGERTIP_FORCE_MAX = 20.0; // N
} // namespace grmh
