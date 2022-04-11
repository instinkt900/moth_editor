#include "common.h"
#include "change_id_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity.h"

ChangeIdAction::ChangeIdAction(std::shared_ptr<moth_ui::Node> node, std::string const& oldId, std::string const& newId)
    : m_node(node)
    , m_oldId(oldId)
    , m_newId(newId) {
}

ChangeIdAction::~ChangeIdAction() {
}

void ChangeIdAction::Do() {
    auto layoutEntity = m_node->GetLayoutEntity();
    m_node->SetId(m_newId);
    layoutEntity->SetId(m_newId);
}

void ChangeIdAction::Undo() {
    auto layoutEntity = m_node->GetLayoutEntity();
    m_node->SetId(m_oldId);
    layoutEntity->SetId(m_oldId);
}

void ChangeIdAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeIdAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old ID", "%s", m_oldId.c_str());
        ImGui::LabelText("New ID", "%s", m_newId.c_str());
    }
}
