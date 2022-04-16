#pragma once

#include "moth_ui/layout/layout_entity.h"
#include "image.h"

inline char const* GetEntityLabel(std::shared_ptr<moth_ui::LayoutEntity> entity) {
    char const* typeString = nullptr;
    switch (entity->GetType()) {
    case moth_ui::LayoutEntityType::Entity:
        typeString = "Entity";
        break;
    case moth_ui::LayoutEntityType::Group:
        typeString = "Group";
        break;
    case moth_ui::LayoutEntityType::Ref:
        typeString = "Ref";
        break;
    case moth_ui::LayoutEntityType::Rect:
        typeString = "Rect";
        break;
    case moth_ui::LayoutEntityType::Image:
        typeString = "Image";
        break;
    case moth_ui::LayoutEntityType::Text:
        typeString = "Text";
        break;
    case moth_ui::LayoutEntityType::Clip:
        typeString = "Clip";
        break;
    default:
        assert(false && "Unknown entity.");
    }
    static std::string stringBuffer;
    if (entity->m_id.empty()) {
        return typeString;
    } else {
        stringBuffer = fmt::format("{} ({})", entity->m_id, typeString);
        return stringBuffer.c_str();
    }
}

namespace imgui_ext {
    inline void Inspect(char const* name, moth_ui::IImage* value) {
        auto const image = static_cast<Image*>(value);
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

        // TODO show the source rect on top of this image. Will probably need to write a custom ImGui::Image like implementation
    }
}
