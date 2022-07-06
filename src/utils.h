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

struct ColorComponents {
    explicit ColorComponents(moth_ui::Color const& color)
        : r(static_cast<uint8_t>(255 * std::clamp(color.r, 0.0f, 1.0f)))
        , g(static_cast<uint8_t>(255 * std::clamp(color.g, 0.0f, 1.0f)))
        , b(static_cast<uint8_t>(255 * std::clamp(color.b, 0.0f, 1.0f)))
        , a(static_cast<uint8_t>(255 * std::clamp(color.a, 0.0f, 1.0f))) {
    }

    uint8_t r, g, b, a;
};
