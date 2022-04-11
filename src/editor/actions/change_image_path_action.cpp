#include "common.h"
#include "change_image_path_action.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity_image.h"

ChangeImagePathAction::ChangeImagePathAction(std::shared_ptr<moth_ui::Node> node, std::string const& oldPath, std::string const& newPath)
    : m_node(node)
    , m_oldPath(oldPath)
    , m_newPath(newPath) {
}

ChangeImagePathAction::~ChangeImagePathAction() {
}

void ChangeImagePathAction::Do() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(m_node->GetLayoutEntity());
    layoutEntity->m_texturePath = m_newPath;
    m_node->ReloadEntity();
}

void ChangeImagePathAction::Undo() {
    auto layoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(m_node->GetLayoutEntity());
    layoutEntity->m_texturePath = m_oldPath;
    m_node->ReloadEntity();
}

void ChangeImagePathAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeImagePathAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Path", "%f", m_oldPath.c_str());
        ImGui::LabelText("New Path", "%f", m_newPath.c_str());
    }
}
