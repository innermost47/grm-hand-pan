// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The sizes of the interface, in points at a scale of 1 (the editor scales them all at once): spacing on a grid of 4,
// hairlines, the typographic scale, the size of the editor and of its parts.
#pragma once

namespace grmh::ui
{
struct Spacing
{
    int xs = 2;
    int sm = 4;
    int md = 8;
    int lg = 12;
    int xl = 16;
    int xxl = 24;
};

struct Borders
{
    float hairline = 1.0f;
    float ring = 1.5f;   // a knob's track, a focused field
    float corner = 4.0f; // a panel, a field
};

struct Typography
{
    float caption = 10.0f; // a knob's letter, a LED's bar number
    float small = 11.0f;
    float label = 12.0f;   // buttons, lists
    float body = 13.0f;
    float heading = 15.0f; // a panel's title
    float display = 22.0f; // the plugin's name
    float tracking = 0.08f; // the letter spacing of headings (share of the height)
};

struct Dimensions
{
    int editor_width = 1280;
    int editor_height = 780;
    int top_bar_height = 76;
    int knob = 44;
    int knob_caption = 16; // the knob's name, under it
    int button_height = 26;
    int field_height = 30; // a combo box
    int side_column = 300; // the patterns on the left, the arranger on the right
    int panel_title = 24;  // a panel's title, above its content
    int transport_button = 30;
    int position_width = 96; // bar.beat.step
    int signature_width = 76;
    int tempo_width = 92;
    int scale_width = 240;
    int lap_switch_width = 120;
    int title_width = 170; // the plugin's name, at the left of the top bar
    int loader_size = 14;  // the loader's circle, under the name
    int loader_height = 18;
    int roll_header = 30;  // the roll's buttons and switch, above it
    int toolbar_button = 84;
    int slot_height = 34; // a pattern of a bank
    int slot_number = 22; // its number, before its name
    int bars_width = 96;  // the pattern's bar count, in the top bar
    int block_count = 34; // a block's count ("×2", "2 bars"), between its − and +
    int arranger_button = 84;
    int roll_names = 28;   // the fields' names, under it
    int roll_gutter = 36;  // bars and beats on the left, LEDs on the right
};

inline constexpr Spacing SPACING{};
inline constexpr Borders BORDERS{};
inline constexpr Typography TYPOGRAPHY{};
inline constexpr Dimensions DIMENSIONS{};
} // namespace grmh::ui
