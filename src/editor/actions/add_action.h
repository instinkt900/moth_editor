#pragma once

#include "editor_action.h"

namespace ui {
    class Node;
    class Group;

    class AddAction : public IEditorAction {
    public:
        AddAction(std::shared_ptr<Node> newNode, std::shared_ptr<Group> parentNode);
        virtual ~AddAction();

        void Do() override;
        void Undo() override;

        void OnImGui() override;
    protected:
        std::shared_ptr<Node> m_newNode;
        std::shared_ptr<Group> m_parentNode;
    };
}
