#include "common.h"
#include "change_font_size_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_text.h"

ChangeFontSizeAction::ChangeFontSizeAction(std::shared_ptr<moth_ui::Node> node, int oldSize, int newSize)
    : m_node(node)
    , m_oldSize(oldSize)
    , m_newSize(newSize) {
}

ChangeFontSizeAction::~ChangeFontSizeAction() {
}

void ChangeFontSizeAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_fontSize = m_newSize;
    m_node->ReloadEntity();
}

void ChangeFontSizeAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_fontSize = m_oldSize;
    m_node->ReloadEntity();
}

void ChangeFontSizeAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeFontSizeAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Size", "%d", m_oldSize);
        ImGui::LabelText("New Size", "%d", m_newSize);
    }
}
