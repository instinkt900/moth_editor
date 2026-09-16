#pragma once

#include "editor_action.h"

class AddAction : public IEditorAction {
public:
    AddAction(std::shared_ptr<moth::ui::Node> newNode, std::shared_ptr<moth::ui::Group> parentNode);
    ~AddAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::Node> m_newNode;
    std::shared_ptr<moth::ui::Group> m_parentNode;
};
