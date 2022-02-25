#pragma once

#include "editor_action.h"

class AddAction : public IEditorAction {
public:
    AddAction(std::shared_ptr<moth_ui::Node> newNode, std::shared_ptr<moth_ui::Group> parentNode);
    virtual ~AddAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_newNode;
    std::shared_ptr<moth_ui::Group> m_parentNode;
};
