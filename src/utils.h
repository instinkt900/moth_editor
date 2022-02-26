#pragma once

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
