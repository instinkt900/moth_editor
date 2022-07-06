#pragma once

#include "moth_ui/layout/layout_entity.h"

inline char const* GetEntityLabel(std::shared_ptr<moth_ui::LayoutEntity> entity) {
    static std::string stringBuffer;
    if (entity->m_id.empty()) {
        stringBuffer = magic_enum::enum_name(entity->GetType());
    } else {
        stringBuffer = fmt::format("{} ({})", entity->m_id, magic_enum::enum_name(entity->GetType()));
    }
    return stringBuffer.c_str();
}
