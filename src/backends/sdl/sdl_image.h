#pragma once

#include "moth_ui/iimage.h"
#include "smart_sdl.h"

class SDLImage : public moth_ui::IImage {
public:
    explicit SDLImage(TextureRef texture);
    SDLImage(TextureRef texture, moth_ui::IntVec2 const& textureDimensions, moth_ui::IntRect const& sourceRect);
    virtual ~SDLImage() = default;

    int GetWidth() const override;
    int GetHeight() const override;
    moth_ui::IntVec2 GetDimensions() const override;
    void ImGui(moth_ui::IntVec2 const& size, moth_ui::FloatVec2 const& uv0, moth_ui::FloatVec2 const& uv1) const override;

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
