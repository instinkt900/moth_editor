#include "common.h"
#include "set_image_scale_type_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_image.h"

SetImageScaleTypeAction::SetImageScaleTypeAction(std::shared_ptr<moth_ui::Node> node, moth_ui::ImageScaleType oldScaleType, moth_ui::ImageScaleType newScaleType)
    : m_node(node)
    , m_oldScaleType(oldScaleType)
    , m_newScaleType(newScaleType) {
}

SetImageScaleTypeAction::~SetImageScaleTypeAction() {
}

void SetImageScaleTypeAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(m_node->GetLayoutEntity());
    layoutEntity->m_imageScaleType = m_newScaleType;
    m_node->ReloadEntity();
}

void SetImageScaleTypeAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(m_node->GetLayoutEntity());
    layoutEntity->m_imageScaleType = m_oldScaleType;
    m_node->ReloadEntity();
}

void SetImageScaleTypeAction::OnImGui() {
    if (ImGui::CollapsingHeader("SetImageScaleTypeAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Scale Type", "%s", magic_enum::enum_name(m_oldScaleType));
        ImGui::LabelText("New Scale Type", "%s", magic_enum::enum_name(m_newScaleType));
    }
}
