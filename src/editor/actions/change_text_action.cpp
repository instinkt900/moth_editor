#include "common.h"
#include "change_text_action.h"
#include "moth_ui/node_text.h"
#include "moth_ui/layout/layout_entity_text.h"

ChangeTextAction::ChangeTextAction(std::shared_ptr<moth_ui::NodeText> node, std::string const& oldText, std::string const& newText)
    : m_node(node)
    , m_oldText(oldText)
    , m_newText(newText) {
}

ChangeTextAction::~ChangeTextAction() {
}

void ChangeTextAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    m_node->SetText(m_newText.c_str());
    layoutEntity->m_text = m_newText;
}

void ChangeTextAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    m_node->SetText(m_oldText.c_str());
    layoutEntity->m_text = m_oldText;
}

void ChangeTextAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeTextAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Text", "%s", m_oldText.c_str());
        ImGui::LabelText("New Text", "%s", m_newText.c_str());
    }
}
