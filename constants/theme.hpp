// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 Anthony Charretier
//
// The colours of the interface: the dark panels of design/gemini.jpg, made more minimal and Japanese (Anthony,
// 26/11/2026). Sumi ink for the ground, washi paper for the text, the cherry blossom (sakura) and the light indigo
// (hanada) for the two hands, vermilion (shu) only for what is playing now. The palette names the colours, the scheme
// says what each one is for: a widget asks the scheme, never the palette.
#pragma once

#include <cstdint>

namespace grmh::ui
{
struct Palette
{
    std::uint32_t sumi_deep = 0xff111013;  // the window
    std::uint32_t sumi = 0xff19181c;       // a panel
    std::uint32_t sumi_raised = 0xff232127; // a button, a field, a knob
    std::uint32_t sumi_line = 0xff36333b;  // hairlines, rings at rest
    std::uint32_t washi = 0xffeee8df;      // text
    std::uint32_t washi_dim = 0xffa39c93;  // secondary text
    std::uint32_t stone = 0xff6e6962;      // ticks, what is disabled
    std::uint32_t sakura = 0xffe9a9bb;
    std::uint32_t sakura_deep = 0xffb87588;
    std::uint32_t hanada = 0xff8fb3d1;
    std::uint32_t hanada_deep = 0xff5d82a3;
    std::uint32_t shu = 0xffe0674a;
};

struct ColorScheme
{
    std::uint32_t window, panel, raised, line, text, text_dim, muted, accent, accent_deep, right_hand, right_hand_deep,
        left_hand, left_hand_deep, playing;

    static constexpr ColorScheme from(const Palette& p)
    {
        return {p.sumi_deep, p.sumi,   p.sumi_raised, p.sumi_line,   p.washi,  p.washi_dim,   p.stone,
                p.sakura,    p.sakura_deep, p.sakura, p.sakura_deep, p.hanada, p.hanada_deep, p.shu};
    }
};

inline constexpr Palette PALETTE{};
inline constexpr ColorScheme COLORS = ColorScheme::from(PALETTE);

// how much of a colour shows, by use
struct Opacity
{
    float hover = 0.08f;    // a wash over what the pointer is on
    float pressed = 0.16f;
    float disabled = 0.4f;
    float faint = 0.25f;    // a hint: an empty place, a ring at rest
};

inline constexpr Opacity OPACITY{};
} // namespace grmh::ui
