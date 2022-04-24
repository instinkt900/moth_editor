#pragma once

#include "moth_ui/layout/layout_entity.h"
#include "image.h"

inline char const* GetEntityLabel(std::shared_ptr<moth_ui::LayoutEntity> entity) {
    static std::string stringBuffer;
    if (entity->m_id.empty()) {
        stringBuffer = magic_enum::enum_name(entity->GetType());
    } else {
        stringBuffer = fmt::format("{} ({})", entity->m_id, magic_enum::enum_name(entity->GetType()));
    }
    return stringBuffer.c_str();
}

namespace imgui_ext {
    inline void Inspect(char const* name, moth_ui::IImage const* value) {
        if (value == nullptr) {
            return;
        }
        auto const image = static_cast<Image const*>(value);
        auto const texture = image->GetTexture();
        auto const& sourceRect = image->GetSourceRect();
        auto const textureDimensions = static_cast<moth_ui::FloatVec2>(image->GetTextureDimensions());
        auto const uv0 = static_cast<moth_ui::FloatVec2>(sourceRect.topLeft) / textureDimensions;
        auto const uv1 = static_cast<moth_ui::FloatVec2>(sourceRect.bottomRight) / textureDimensions;

        float constexpr ImageWidth = 200;
        auto const imageDimensions = image->GetDimensions();
        auto const scale = ImageWidth / imageDimensions.x;
        auto const scaledDimensions = static_cast<moth_ui::FloatVec2>(imageDimensions) * scale;
        ImGui::Image(texture.get(), ImVec2(scaledDimensions.x, scaledDimensions.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
    }
}
