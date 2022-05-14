#pragma once

#include "moth_ui/text_alignment.h"
#include "moth_ui/utils/color.h"
#include "moth_ui/blend_mode.h"

// returns a rect with the coordinates of b within a
inline moth_ui::IntRect MergeRects(moth_ui::IntRect const& a, moth_ui::IntRect const& b) {
    moth_ui::IntRect c;
    c.topLeft = a.topLeft + b.topLeft;
    c.bottomRight = b.bottomRight + a.topLeft;
    // contain c within a
    c.topLeft.x = std::max(c.topLeft.x, a.topLeft.x);
    c.topLeft.y = std::max(c.topLeft.y, a.topLeft.y);
    c.bottomRight.x = std::min(c.bottomRight.x, a.bottomRight.x);
    c.bottomRight.y = std::min(c.bottomRight.y, a.bottomRight.y);
    return c;
}

inline SDL_Rect ToSDL(moth_ui::IntRect const& rect) {
    return { rect.topLeft.x, rect.topLeft.y, rect.bottomRight.x - rect.topLeft.x, rect.bottomRight.y - rect.topLeft.y };
}

inline SDL_FRect ToSDL(moth_ui::FloatRect const& rect) {
    return { rect.topLeft.x, rect.topLeft.y, rect.bottomRight.x - rect.topLeft.x, rect.bottomRight.y - rect.topLeft.y };
}

inline FC_AlignEnum ToSDL(moth_ui::TextHorizAlignment const& textAlign) {
    switch (textAlign) {
    default:
    case moth_ui::TextHorizAlignment::Left:
        return FC_ALIGN_LEFT;
    case moth_ui::TextHorizAlignment::Center:
        return FC_ALIGN_CENTER;
    case moth_ui::TextHorizAlignment::Right:
        return FC_ALIGN_RIGHT;
    }
}

inline SDL_Color ToSDL(moth_ui::Color const& color) {
    return {
        static_cast<Uint8>(color.r * 0xFF),
        static_cast<Uint8>(color.g * 0xFF),
        static_cast<Uint8>(color.b * 0xFF),
        static_cast<Uint8>(color.a * 0xFF)
    };
}

struct ColorComponents {
    explicit ColorComponents(moth_ui::Color const& color)
        : r(static_cast<uint8_t>(255 * std::clamp(color.r, 0.0f, 1.0f)))
        , g(static_cast<uint8_t>(255 * std::clamp(color.g, 0.0f, 1.0f)))
        , b(static_cast<uint8_t>(255 * std::clamp(color.b, 0.0f, 1.0f)))
        , a(static_cast<uint8_t>(255 * std::clamp(color.a, 0.0f, 1.0f))) {
    }

    uint8_t r, g, b, a;
};

inline SDL_BlendMode ToSDL(moth_ui::BlendMode mode) {
    switch (mode) {
    default:
    case moth_ui::BlendMode::Replace:
        return SDL_BlendMode::SDL_BLENDMODE_NONE;
    case moth_ui::BlendMode::Alpha:
        return SDL_BlendMode::SDL_BLENDMODE_BLEND;
    case moth_ui::BlendMode::Add:
        return SDL_BlendMode::SDL_BLENDMODE_ADD;
    case moth_ui::BlendMode::Multiply:
        return SDL_BlendMode::SDL_BLENDMODE_MUL;
    case moth_ui::BlendMode::Modulate:
        return SDL_BlendMode::SDL_BLENDMODE_MOD;
    }
}
