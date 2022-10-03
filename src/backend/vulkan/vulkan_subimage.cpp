#include "common.h"
#include "vulkan_subimage.h"
#include "vulkan_graphics.h"

namespace backend::vulkan {
    Graphics* SubImage::s_graphicsContext = nullptr;

    SubImage::SubImage(std::shared_ptr<Image> texture, moth_ui::IntVec2 const& textureDimensions, moth_ui::IntRect const& sourceRect)
        : m_texture(texture)
        , m_textureDimensions(textureDimensions)
        , m_sourceRect(sourceRect) {
    }

    SubImage::~SubImage() {
    }

    int SubImage::GetWidth() const {
        return m_sourceRect.bottomRight.x - m_sourceRect.topLeft.x;
    }

    int SubImage::GetHeight() const {
        return m_sourceRect.bottomRight.y - m_sourceRect.topLeft.y;
    }

    moth_ui::IntVec2 SubImage::GetDimensions() const {
        return { GetWidth(), GetHeight() };
    }

    void SubImage::ImGui(moth_ui::IntVec2 const& size, moth_ui::FloatVec2 const& uv0, moth_ui::FloatVec2 const& uv1) const {
        if (m_texture && s_graphicsContext) {
            VkDescriptorSet descriptorSet = s_graphicsContext->GetDescriptorSet(*m_texture);
            ImGui::Image(descriptorSet,
                           ImVec2(static_cast<float>(size.x), static_cast<float>(size.y)),
                           ImVec2(uv0.x, uv0.y),
                           ImVec2(uv1.x, uv1.y));
        }
    }
}
