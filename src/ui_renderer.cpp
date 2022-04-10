#include "common.h"
#include "ui_renderer.h"
#include "image.h"
#include "font.h"

struct ColorComponents {
    explicit ColorComponents(moth_ui::Color const& color)
        : r(static_cast<uint8_t>(255 * std::clamp(color.r, 0.0f, 1.0f)))
        , g(static_cast<uint8_t>(255 * std::clamp(color.g, 0.0f, 1.0f)))
        , b(static_cast<uint8_t>(255 * std::clamp(color.b, 0.0f, 1.0f)))
        , a(static_cast<uint8_t>(255 * std::clamp(color.a, 0.0f, 1.0f))) {
    }

    uint8_t r, g, b, a;
};

SDL_BlendMode ToSDL(moth_ui::BlendMode mode) {
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

UIRenderer::UIRenderer(SDL_Renderer& renderer)
    : m_renderer(renderer) {
    m_drawColor.push({ 1.0f, 1.0f, 1.0f, 1.0f });
    m_blendMode.push(moth_ui::BlendMode::Replace);
}

void UIRenderer::PushBlendMode(moth_ui::BlendMode mode) {
    m_blendMode.push(mode);
}

void UIRenderer::PopBlendMode() {
    if (m_blendMode.size() > 1) {
        m_blendMode.pop();
    }
}

void UIRenderer::PushColor(moth_ui::Color const& color) {
    auto const modColor = m_drawColor.top() * color;
    m_drawColor.push(modColor);
}

void UIRenderer::PopColor() {
    if (m_drawColor.size() > 1) {
        m_drawColor.pop();
    }
}

void UIRenderer::DrawRect(moth_ui::IntRect const& rect, moth_ui::Color const& color, moth_ui::BlendMode blendMode) {
    auto const sdlRect{ ToSDL(rect) };
    auto const modColor = m_drawColor.top() * color;
    ColorComponents components{ modColor };
    SDL_SetRenderDrawBlendMode(&m_renderer, ToSDL(blendMode));
    SDL_SetRenderDrawColor(&m_renderer, components.r, components.g, components.b, components.a);
    SDL_RenderDrawRect(&m_renderer, &sdlRect);
}

void UIRenderer::DrawFilledRect(moth_ui::IntRect const& rect, moth_ui::Color const& color, moth_ui::BlendMode blendMode) {
    auto const sdlRect{ ToSDL(rect) };
    auto const modColor = m_drawColor.top() * color;
    ColorComponents const components{ modColor };
    SDL_SetRenderDrawBlendMode(&m_renderer, ToSDL(blendMode));
    SDL_SetRenderDrawColor(&m_renderer, components.r, components.g, components.b, components.a);
    SDL_RenderDrawRect(&m_renderer, &sdlRect);
}

void UIRenderer::RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& sourceRect, moth_ui::IntRect const& destRect, moth_ui::ImageScaleType scaleType, float scale) {
    auto const& internalImage = static_cast<Image&>(image);
    auto const texture = internalImage.GetTexture();
    auto const& textureSourceRect = internalImage.GetSourceRect();
    auto const sdlsourceRect{ ToSDL(MergeRects(textureSourceRect, sourceRect)) };
    ColorComponents const components{ m_drawColor.top() };
    SDL_SetTextureBlendMode(texture.get(), ToSDL(m_blendMode.top()));
    SDL_SetTextureColorMod(texture.get(), components.r, components.g, components.b);
    SDL_SetTextureAlphaMod(texture.get(), components.a);

    if (scaleType == moth_ui::ImageScaleType::Stretch) {
        auto const sdlDestRect{ ToSDL(destRect) };
        SDL_RenderCopy(&m_renderer, texture.get(), &sdlsourceRect, &sdlDestRect);
    } else if (scaleType == moth_ui::ImageScaleType::Tile) {
        // sdl doesnt have a tiling texture ability. we need to manually tile
        auto const destWidth = destRect.bottomRight.x - destRect.topLeft.x;
        auto const destHeight = destRect.bottomRight.y - destRect.topLeft.y;
        auto const imageWidth = static_cast<int>(image.GetWidth() * scale);
        auto const imageHeight = static_cast<int>(image.GetHeight() * scale);
        auto const sdlTotalDestRect{ ToSDL(destRect) };
        SDL_RenderSetClipRect(&m_renderer, &sdlTotalDestRect);
        for (auto y = destRect.topLeft.y; y < destRect.bottomRight.y; y += imageHeight) {
            for (auto x = destRect.topLeft.x; x < destRect.bottomRight.x; x += imageWidth) {
                SDL_Rect sdlDestRect{ x, y, imageWidth, imageHeight };
                SDL_RenderCopy(&m_renderer, texture.get(), &sdlsourceRect, &sdlDestRect);
            }
        }
        SDL_RenderSetClipRect(&m_renderer, nullptr);
    }
}

void UIRenderer::RenderText(std::string const& text, moth_ui::IFont& font, moth_ui::TextHorizAlignment horizontalAlignment, moth_ui::TextVertAlignment verticalAlignment, moth_ui::IntRect const& destRect) {
    auto const fcFont = static_cast<Font&>(font).GetFontObj();

    auto const destWidth = destRect.bottomRight.x - destRect.topLeft.x;
    auto const destHeight = destRect.bottomRight.y - destRect.topLeft.y;
    auto const textHeight = FC_GetColumnHeight(fcFont.get(), destWidth, "%s", text.c_str());

    auto x = static_cast<float>(destRect.topLeft.x);
    switch (horizontalAlignment) {
    case moth_ui::TextHorizAlignment::Left:
        break;
    case moth_ui::TextHorizAlignment::Center:
        x = x + destWidth / 2;
        break;
    case moth_ui::TextHorizAlignment::Right:
        x = x + destWidth;
        break;
    }

    auto y = static_cast<float>(destRect.topLeft.y);
    switch (verticalAlignment) {
    case moth_ui::TextVertAlignment::Top:
        break;
    case moth_ui::TextVertAlignment::Middle:
        y = y + (destHeight - textHeight) / 2;
        break;
    case moth_ui::TextVertAlignment::Bottom:
        y = y + destHeight - textHeight;
        break;
    }

    FC_Effect effect;
    effect.alignment = ToSDL(horizontalAlignment);
    effect.color = ToSDL(m_drawColor.top());
    effect.scale.x = 1.0f;
    effect.scale.y = 1.0f;

    FC_DrawColumnEffect(fcFont.get(), &m_renderer, x, y, destWidth, effect, "%s", text.c_str());
}
