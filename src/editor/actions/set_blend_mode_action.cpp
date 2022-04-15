#include "common.h"
#include "set_blend_mode_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity.h"

SetBlendModeAction::SetBlendModeAction(std::shared_ptr<moth_ui::Node> node, moth_ui::BlendMode oldBlendMode, moth_ui::BlendMode newBlendMode)
    : m_node(node)
    , m_oldBlendMode(oldBlendMode)
    , m_newBlendMode(newBlendMode) {
}

SetBlendModeAction::~SetBlendModeAction() {
}

void SetBlendModeAction::Do() {
    auto layoutEntity = m_node->GetLayoutEntity();
    m_node->SetBlendMode(m_newBlendMode);
    layoutEntity->m_blend = m_newBlendMode;
}

void SetBlendModeAction::Undo() {
    auto layoutEntity = m_node->GetLayoutEntity();
    m_node->SetBlendMode(m_oldBlendMode);
    layoutEntity->m_blend = m_oldBlendMode;
}

void SetBlendModeAction::OnImGui() {
    if (ImGui::CollapsingHeader("SetBlendModeAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Blend Mode", "%s", magic_enum::enum_name(m_oldBlendMode));
        ImGui::LabelText("New Blend Mode", "%s", magic_enum::enum_name(m_newBlendMode));
    }
}
