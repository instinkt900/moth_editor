#pragma once

#include "uilib/layout/layout_entity.h"

namespace ui {
    inline char const* GetEntityLabel(std::shared_ptr<LayoutEntity> entity) {
        char const* typeString = nullptr;
        switch (entity->GetType()) {
        case LayoutEntityType::Entity:
            typeString = "Entity";
            break;
        case LayoutEntityType::Group:
            typeString = "Group";
            break;
        case LayoutEntityType::Image:
            typeString = "Image";
            break;
        }
        static std::string stringBuffer;
        if (entity->GetId().empty()) {
            return typeString;
        } else {
            stringBuffer = fmt::format("{} ({})", entity->GetId(), typeString);
            return stringBuffer.c_str();
        }
    }
}
