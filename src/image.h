#pragma once

#include "moth_ui/iimage.h"

class Image : public ui::IImage {
public:
    Image(TextureRef texture, IntRect const& sourceRect);
    virtual ~Image() = default;

    int GetWidth() const override;
    int GetHeight() const override;
    IntVec2 GetDimensions() const override;

    TextureRef GetTexture() const {
        return m_texture;
    }

    IntRect const& GetSourceRect() const {
        return m_sourceRect;
    }

private:
    TextureRef m_texture;
    IntRect m_sourceRect;
};
