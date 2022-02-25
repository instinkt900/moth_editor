#include "common.h"
#include "ui_renderer.h"
#include "image.h"

UIRenderer::UIRenderer(SDL_Renderer& renderer)
    : m_renderer(renderer) {
}

void UIRenderer::SetRenderColor(uint32_t argb) {
    uint8_t const a = static_cast<uint8_t>((argb >> 24) & 0x00FF);
    uint8_t const r = static_cast<uint8_t>((argb >> 16) & 0x00FF);
    uint8_t const g = static_cast<uint8_t>((argb >> 8) & 0x00FF);
    uint8_t const b = static_cast<uint8_t>(argb & 0x00FF);
    SDL_SetRenderDrawColor(&m_renderer, r, g, b, a);
}

void UIRenderer::RenderRect(moth_ui::IntRect const& rect) {
    SDL_Rect sdlRect{ ToSDL(rect) };
    SDL_RenderDrawRect(&m_renderer, &sdlRect);
}

void UIRenderer::RenderFilledRect(moth_ui::IntRect const& rect) {
    SDL_Rect sdlRect{ ToSDL(rect) };
    SDL_RenderDrawRect(&m_renderer, &sdlRect);
}

void UIRenderer::RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& destRect) {
    Image& internalImage = static_cast<Image&>(image);
    auto texture = internalImage.GetTexture();
    auto const& sourceRect = internalImage.GetSourceRect();

    SDL_Rect sdlsourceRect{ ToSDL(sourceRect) };
    SDL_Rect sdlDestRect{ ToSDL(destRect) };
    SDL_RenderCopy(&m_renderer, texture.get(), &sdlsourceRect, &sdlDestRect);
}
