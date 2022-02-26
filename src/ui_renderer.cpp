#include "common.h"
#include "ui_renderer.h"
#include "image.h"

UIRenderer::UIRenderer(SDL_Renderer& renderer)
    : m_renderer(renderer) {
}

void UIRenderer::SetRenderColor(uint32_t argb) {
    auto const a = static_cast<uint8_t>((argb >> 24) & 0x00FF);
    auto const r = static_cast<uint8_t>((argb >> 16) & 0x00FF);
    auto const g = static_cast<uint8_t>((argb >> 8) & 0x00FF);
    auto const b = static_cast<uint8_t>(argb & 0x00FF);
    SDL_SetRenderDrawColor(&m_renderer, r, g, b, a);
}

void UIRenderer::RenderRect(moth_ui::IntRect const& rect) {
    auto const sdlRect{ ToSDL(rect) };
    SDL_RenderDrawRect(&m_renderer, &sdlRect);
}

void UIRenderer::RenderFilledRect(moth_ui::IntRect const& rect) {
    auto const sdlRect{ ToSDL(rect) };
    SDL_RenderDrawRect(&m_renderer, &sdlRect);
}

void UIRenderer::RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& sourceRect, moth_ui::IntRect const& destRect) {
    auto const& internalImage = static_cast<Image&>(image);
    auto const texture = internalImage.GetTexture();
    auto const& textureSourceRect = internalImage.GetSourceRect();

    auto const sdlsourceRect{ ToSDL(MergeRects(textureSourceRect, sourceRect)) };
    auto const sdlDestRect{ ToSDL(destRect) };
    SDL_RenderCopy(&m_renderer, texture.get(), &sdlsourceRect, &sdlDestRect);
}
