#pragma once

#include "editor_action.h"

class ChangeIndexAction : public IEditorAction {
public:
    ChangeIndexAction(std::shared_ptr<moth_ui::Node> node, int oldIndex, int newIndex);
    virtual ~ChangeIndexAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    int m_oldIndex = -1;
    int m_newIndex = -1;
};
