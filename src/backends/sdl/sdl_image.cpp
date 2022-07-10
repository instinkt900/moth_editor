#include "common.h"
#include "sdl_image.h"

SDLImage::SDLImage(TextureRef texture)
    : m_texture(texture) {
    SDL_QueryTexture(texture->GetImpl(), NULL, NULL, &m_textureDimensions.x, &m_textureDimensions.y);
    m_sourceRect = moth_ui::IntRect{ { 0, 0 }, m_textureDimensions };
}

SDLImage::SDLImage(TextureRef texture, moth_ui::IntVec2 const& textureDimensions, moth_ui::IntRect const& sourceRect)
    : m_texture(texture)
    , m_textureDimensions(textureDimensions)
    , m_sourceRect(sourceRect) {
}

int SDLImage::GetWidth() const {
    return m_sourceRect.bottomRight.x - m_sourceRect.topLeft.x;
}

int SDLImage::GetHeight() const {
    return m_sourceRect.bottomRight.y - m_sourceRect.topLeft.y;
}

moth_ui::IntVec2 SDLImage::GetDimensions() const {
    return { GetWidth(), GetHeight() };
}

void SDLImage::ImGui(moth_ui::IntVec2 const& size, moth_ui::FloatVec2 const& uv0, moth_ui::FloatVec2 const& uv1) const {
    ImGui::Image(m_texture ? m_texture->GetImpl() : nullptr,
        ImVec2(static_cast<float>(size.x), static_cast<float>(size.y)),
        ImVec2(uv0.x, uv0.y),
        ImVec2(uv1.x, uv1.y));
}
