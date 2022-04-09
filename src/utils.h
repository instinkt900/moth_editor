#pragma once

#include "moth_ui/text_alignment.h"
#include "moth_ui/utils/color.h"

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
