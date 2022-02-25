#pragma once

#include "editor_action.h"

class DeleteAction : public IEditorAction {
public:
    DeleteAction(std::shared_ptr<moth_ui::Node> deletedNode, std::shared_ptr<moth_ui::Group> parentNode);
    virtual ~DeleteAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_deletedNode;
    std::shared_ptr<moth_ui::Group> m_parentNode;
};
