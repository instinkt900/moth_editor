#include "common.h"
#include "change_index_action.h"
#include "moth_ui/layout/layout_entity_group.h"
#include "moth_ui/group.h"

ChangeIndexAction::ChangeIndexAction(std::shared_ptr<moth_ui::Node> node, int oldIndex, int newIndex)
    : m_node(node)
    , m_oldIndex(oldIndex)
    , m_newIndex(newIndex) {
}

ChangeIndexAction::~ChangeIndexAction() {
}

void ChangeIndexAction::Do() {
    // update the node tree
    auto parentNode = m_node->GetParent();
    auto& parentChildren = parentNode->GetChildren();
    parentChildren.erase(std::next(std::begin(parentChildren), m_oldIndex));
    parentChildren.insert(std::next(std::begin(parentChildren), m_newIndex), m_node);

    // need to alter the layout entity trees too
    auto parentLayoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(parentNode->GetLayoutEntity());
    auto& parentEntityChildren = parentLayoutEntity->GetChildren();
    auto oldEntity = parentEntityChildren[m_oldIndex];
    parentEntityChildren.erase(std::next(std::begin(parentEntityChildren), m_oldIndex));
    parentEntityChildren.insert(std::next(std::begin(parentEntityChildren), m_newIndex), oldEntity);
}

void ChangeIndexAction::Undo() {
    // update the node tree
    auto parentNode = m_node->GetParent();
    auto& parentChildren = parentNode->GetChildren();
    parentChildren.erase(std::next(std::begin(parentChildren), m_newIndex));
    parentChildren.insert(std::next(std::begin(parentChildren), m_oldIndex), m_node);

    // need to alter the layout entity trees too
    auto parentLayoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(parentNode->GetLayoutEntity());
    auto& parentEntityChildren = parentLayoutEntity->GetChildren();
    auto oldEntity = parentEntityChildren[m_oldIndex];
    parentEntityChildren.erase(std::next(std::begin(parentEntityChildren), m_newIndex));
    parentEntityChildren.insert(std::next(std::begin(parentEntityChildren), m_oldIndex), oldEntity);
}

void ChangeIndexAction::OnImGui() {
    if (ImGui::CollapsingHeader("ChangeIndexAction")) {
        ImGui::LabelText("Node", "%p", m_node.get());
        ImGui::LabelText("Old Index", "%d", m_oldIndex);
        ImGui::LabelText("New Index", "%d", m_newIndex);
    }
}
