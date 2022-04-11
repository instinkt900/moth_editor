#include "common.h"
#include "show_bounds_action.h"
#include "moth_ui/node.h"

ShowBoundsAction::ShowBoundsAction(std::shared_ptr<moth_ui::Node> node, bool visible)
    : m_node(node)
    , m_visible(visible) {
}

ShowBoundsAction::~ShowBoundsAction() {
}

void ShowBoundsAction::Do() {
    m_node->SetShowRect(m_visible);
}

void ShowBoundsAction::Undo() {
    m_node->SetShowRect(!m_visible);
}

void ShowBoundsAction::OnImGui() {
    if (ImGui::CollapsingHeader("ShowBoundsAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Visible", "%s", m_visible ? "true" : "false");
    }
}
