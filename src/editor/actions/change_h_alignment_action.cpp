#include "common.h"
#include "change_h_alignment_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_text.h"

ChangeHAlignmentAction::ChangeHAlignmentAction(std::shared_ptr<moth_ui::Node> node, moth_ui::TextHorizAlignment oldAlignment, moth_ui::TextHorizAlignment newAlignment)
    : m_node(node)
    , m_oldAlignment(oldAlignment)
    , m_newAlignment(newAlignment) {
}

ChangeHAlignmentAction::~ChangeHAlignmentAction() {
}

void ChangeHAlignmentAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_horizontalAlignment = m_newAlignment;
    m_node->ReloadEntity();
}

void ChangeHAlignmentAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_horizontalAlignment = m_oldAlignment;
    m_node->ReloadEntity();
}

void ChangeHAlignmentAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeHAlignmentAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Alignment", "%s", magic_enum::enum_name(m_oldAlignment));
        ImGui::LabelText("New Alignment", "%s", magic_enum::enum_name(m_newAlignment));
    }
}
