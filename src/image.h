#pragma once

#include "moth_ui/iimage.h"

class Image : public moth_ui::IImage {
public:
    Image(TextureRef texture, moth_ui::IntVec2 const& textureDimensions, moth_ui::IntRect const& sourceRect);
    virtual ~Image() = default;

    int GetWidth() const override;
    int GetHeight() const override;
    moth_ui::IntVec2 GetDimensions() const override;

    TextureRef GetTexture() const {
        return m_texture;
    }

    moth_ui::IntVec2 const& GetTextureDimensions() const {
        return m_textureDimensions;
    }

    moth_ui::IntRect const& GetSourceRect() const {
        return m_sourceRect;
    }

private:
    TextureRef m_texture;
    moth_ui::IntVec2 m_textureDimensions;
    moth_ui::IntRect m_sourceRect;
};
