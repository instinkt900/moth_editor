#pragma once

#include "moth_ui/iimage.h"
#include "vulkan_image.h"

namespace backend::vulkan {
    class SubImage : public moth_ui::IImage {
    public:
        SubImage(std::shared_ptr<Image> texture, moth_ui::IntVec2 const& textureDimensions, moth_ui::IntRect const& sourceRect);
        virtual ~SubImage();

        int GetWidth() const override;
        int GetHeight() const override;
        moth_ui::IntVec2 GetDimensions() const override;
        void ImGui(moth_ui::IntVec2 const& size, moth_ui::FloatVec2 const& uv0, moth_ui::FloatVec2 const& uv1) const override;

        std::shared_ptr<Image> m_texture;
        moth_ui::IntVec2 m_textureDimensions;
        moth_ui::IntRect m_sourceRect;

        static class Graphics* s_graphicsContext;
    };
}
