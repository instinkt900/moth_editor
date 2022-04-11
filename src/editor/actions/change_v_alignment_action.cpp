#include "common.h"
#include "change_v_alignment_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_text.h"

ChangeVAlignmentAction::ChangeVAlignmentAction(std::shared_ptr<moth_ui::Node> node, moth_ui::TextVertAlignment oldAlignment, moth_ui::TextVertAlignment newAlignment)
    : m_node(node)
    , m_oldAlignment(oldAlignment)
    , m_newAlignment(newAlignment) {
}

ChangeVAlignmentAction::~ChangeVAlignmentAction() {
}

void ChangeVAlignmentAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_verticalAlignment = m_newAlignment;
    m_node->ReloadEntity();
}

void ChangeVAlignmentAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(m_node->GetLayoutEntity());
    layoutEntity->m_verticalAlignment = m_oldAlignment;
    m_node->ReloadEntity();
}

void ChangeVAlignmentAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeVAlignmentAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Alignment", "%s", magic_enum::enum_name(m_oldAlignment));
        ImGui::LabelText("New Alignment", "%s", magic_enum::enum_name(m_newAlignment));
    }
}
