#include "common.h"
#include "add_action.h"
#include "uilib/layout/layout_entity_group.h"
#include "uilib/group.h"

namespace ui {
    AddAction::AddAction(std::shared_ptr<Node> newNode, std::shared_ptr<Group> parentNode)
        : m_newNode(newNode)
        , m_parentNode(parentNode) {
    }

    AddAction::~AddAction() {
    }

    void AddAction::Do() {
        // need to merge the layout entity trees too
        auto parentLayoutEntity = std::static_pointer_cast<LayoutEntityGroup>(m_parentNode->GetLayoutEntity());
        auto layoutEntity = m_newNode->GetLayoutEntity();
        parentLayoutEntity->m_children.push_back(layoutEntity);
        layoutEntity->SetParent(parentLayoutEntity.get());

        // merge the actual node instances
        m_parentNode->AddChild(m_newNode);
    }

    void AddAction::Undo() {
        // remove the node instances
        m_parentNode->RemoveChild(m_newNode);

        // separate the layout entity trees
        auto parentLayoutEntity = std::static_pointer_cast<LayoutEntityGroup>(m_parentNode->GetLayoutEntity());
        auto layoutEntity = m_newNode->GetLayoutEntity();
        auto it = std::find_if(std::begin(parentLayoutEntity->m_children), std::end(parentLayoutEntity->m_children), [&layoutEntity](auto& child) { return child == layoutEntity; });
        if (std::end(parentLayoutEntity->m_children) != it) {
            parentLayoutEntity->m_children.erase(it);
        }
        layoutEntity->SetParent(nullptr);
    }

    void AddAction::OnImGui() {
        if (ImGui::CollapsingHeader("AddAction")) {
            ImGui::LabelText("New Node", "%p", m_newNode.get());
            ImGui::LabelText("Parent Node", "%p", m_parentNode.get());
        }
    }
}
