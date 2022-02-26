#include "common.h"
#include "image.h"

Image::Image(TextureRef texture, moth_ui::IntVec2 const& textureDimensions, moth_ui::IntRect const& sourceRect)
    : m_texture(texture)
    , m_textureDimensions(textureDimensions)
    , m_sourceRect(sourceRect) {
}

int Image::GetWidth() const {
    return m_sourceRect.bottomRight.x - m_sourceRect.topLeft.x;
}

int Image::GetHeight() const {
    return m_sourceRect.bottomRight.y - m_sourceRect.topLeft.y;
}

moth_ui::IntVec2 Image::GetDimensions() const {
    return { GetWidth(), GetHeight() };
}
