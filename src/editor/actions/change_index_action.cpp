#include "common.h"
#include "change_index_action.h"
#include "moth_ui/layout/layout_entity_group.h"
#include "moth_ui/nodes/group.h"

ChangeIndexAction::ChangeIndexAction(std::shared_ptr<moth_ui::Node> node, int oldIndex, int newIndex)
    : m_node(node)
    , m_oldIndex(oldIndex)
    , m_newIndex(newIndex) {
}

ChangeIndexAction::~ChangeIndexAction() {
}

void ChangeIndexAction::Do() {
    if (m_node == nullptr) { return; }
    auto* parentNode = m_node->GetParent();
    if (parentNode == nullptr) { return; }

    // update the node tree
    auto& parentChildren = parentNode->GetChildren();
    if (m_oldIndex < 0 || static_cast<size_t>(m_oldIndex) >= parentChildren.size()) { return; }
    if (m_newIndex < 0 || static_cast<size_t>(m_newIndex) >= parentChildren.size()) { return; }
    parentChildren.erase(std::next(std::begin(parentChildren), m_oldIndex));
    parentChildren.insert(std::next(std::begin(parentChildren), m_newIndex), m_node);

    // need to alter the layout entity trees too
    auto parentLayoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(parentNode->GetLayoutEntity());
    auto& parentEntityChildren = parentLayoutEntity->m_children;
    if (static_cast<size_t>(m_oldIndex) >= parentEntityChildren.size()) { return; }
    auto oldEntity = parentEntityChildren[m_oldIndex];
    parentEntityChildren.erase(std::next(std::begin(parentEntityChildren), m_oldIndex));
    parentEntityChildren.insert(std::next(std::begin(parentEntityChildren), m_newIndex), oldEntity);
}

void ChangeIndexAction::Undo() {
    if (m_node == nullptr) { return; }
    auto* parentNode = m_node->GetParent();
    if (parentNode == nullptr) { return; }

    // update the node tree
    auto& parentChildren = parentNode->GetChildren();
    if (m_newIndex < 0 || static_cast<size_t>(m_newIndex) >= parentChildren.size()) { return; }
    if (m_oldIndex < 0 || static_cast<size_t>(m_oldIndex) >= parentChildren.size()) { return; }
    parentChildren.erase(std::next(std::begin(parentChildren), m_newIndex));
    parentChildren.insert(std::next(std::begin(parentChildren), m_oldIndex), m_node);

    // need to alter the layout entity trees too
    auto parentLayoutEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(parentNode->GetLayoutEntity());
    auto& parentEntityChildren = parentLayoutEntity->m_children;
    if (static_cast<size_t>(m_newIndex) >= parentEntityChildren.size()) { return; }
    auto oldEntity = parentEntityChildren[m_newIndex];
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
