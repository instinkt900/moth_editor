#include "common.h"
#include "delete_action.h"
#include "moth_ui/layout/layout_entity_group.h"
#include "moth_ui/group.h"

DeleteAction::DeleteAction(std::shared_ptr<moth_ui::Node> deletedNode, std::shared_ptr<moth_ui::Group> parentNode)
    : m_deletedNode(deletedNode)
    , m_parentNode(parentNode) {
}

DeleteAction::~DeleteAction() {
}

void DeleteAction::Do() {
    // remove the node instances
    m_parentNode->RemoveChild(m_deletedNode);

    // separate the layout entity trees
    auto parentLayoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_parentNode->GetLayoutEntity());
    auto layoutEntity = m_deletedNode->GetLayoutEntity();
    auto it = std::find_if(std::begin(parentLayoutEntity->m_children), std::end(parentLayoutEntity->m_children), [&layoutEntity](auto& child) { return child == layoutEntity; });
    if (std::end(parentLayoutEntity->m_children) != it) {
        parentLayoutEntity->m_children.erase(it);
    }
    layoutEntity->m_parent = nullptr;
}

void DeleteAction::Undo() {
    // need to merge the layout entity trees too
    auto parentLayoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_parentNode->GetLayoutEntity());
    auto layoutEntity = m_deletedNode->GetLayoutEntity();
    parentLayoutEntity->m_children.push_back(layoutEntity);
    layoutEntity->m_parent = parentLayoutEntity.get();

    // merge the actual node instances
    m_parentNode->AddChild(m_deletedNode);
}

void DeleteAction::OnImGui() {
    if (ImGui::CollapsingHeader("DeleteAction")) {
        ImGui::LabelText("Deleted Node", "%p", m_deletedNode.get());
        ImGui::LabelText("Parent Node", "%p", m_parentNode.get());
    }
}
