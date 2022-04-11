#include "common.h"
#include "change_font_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_text.h"

ChangeFontAction::ChangeFontAction(std::shared_ptr<moth_ui::Node> node, std::string const& oldFont, std::string const& newFont)
    : m_node(node)
    , m_oldFont(oldFont)
    , m_newFont(newFont) {
}

ChangeFontAction::~ChangeFontAction() {
}

void ChangeFontAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_fontName = m_newFont;
    m_node->ReloadEntity();
}

void ChangeFontAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_fontName = m_oldFont;
    m_node->ReloadEntity();
}

void ChangeFontAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeFontAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Font", "%d", m_oldFont);
        ImGui::LabelText("New Font", "%d", m_newFont);
    }
}
