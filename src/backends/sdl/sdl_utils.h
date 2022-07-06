#pragma once

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
