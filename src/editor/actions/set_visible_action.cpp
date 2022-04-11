#include "common.h"
#include "set_visible_action.h"
#include "moth_ui/node.h"

SetVisibleAction::SetVisibleAction(std::shared_ptr<moth_ui::Node> node, bool visible)
    : m_node(node)
    , m_visible(visible) {
}

SetVisibleAction::~SetVisibleAction() {
}

void SetVisibleAction::Do() {
    m_node->SetVisible(m_visible);
}

void SetVisibleAction::Undo() {
    m_node->SetVisible(!m_visible);
}

void SetVisibleAction::OnImGui() {
    if (ImGui::CollapsingHeader("SetVisibleAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Visible", "%s", m_visible ? "true" : "false");
    }
}
