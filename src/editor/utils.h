#pragma once

#include "moth_ui/layout/layout_entity.h"

inline char const* GetEntityLabel(std::shared_ptr<moth_ui::LayoutEntity> entity) {
    char const* typeString = nullptr;
    switch (entity->GetType()) {
    case moth_ui::LayoutEntityType::Entity:
        typeString = "Entity";
        break;
    case moth_ui::LayoutEntityType::Group:
        typeString = "Group";
        break;
    case moth_ui::LayoutEntityType::Image:
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
