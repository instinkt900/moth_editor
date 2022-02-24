#pragma once

#include "editor_action.h"

namespace ui {
    class Node;
    class Group;

    class DeleteAction : public IEditorAction {
    public:
        DeleteAction(std::shared_ptr<Node> deletedNode, std::shared_ptr<Group> parentNode);
        virtual ~DeleteAction();

        void Do() override;
        void Undo() override;

        void OnImGui() override;
    protected:
        std::shared_ptr<Node> m_deletedNode;
        std::shared_ptr<Group> m_parentNode;
    };
}
