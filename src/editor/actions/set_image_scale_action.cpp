#include "common.h"
#include "set_image_scale_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_image.h"

SetImageScaleAction::SetImageScaleAction(std::shared_ptr<moth_ui::Node> node, float oldScale, float newScale)
    : m_node(node)
    , m_oldScale(oldScale)
    , m_newScale(newScale) {
}

SetImageScaleAction::~SetImageScaleAction() {
}

void SetImageScaleAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(m_node->GetLayoutEntity());
    layoutEntity->m_imageScale = m_newScale;
    m_node->ReloadEntity();
}

void SetImageScaleAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(m_node->GetLayoutEntity());
    layoutEntity->m_imageScale = m_oldScale;
    m_node->ReloadEntity();
}

void SetImageScaleAction::OnImGui() {
    if (ImGui::CollapsingHeader("SetImageScaleAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Scale", "%f", m_oldScale);
        ImGui::LabelText("New Scale", "%f", m_newScale);
    }
}
