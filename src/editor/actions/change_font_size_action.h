#pragma once

#include "editor_action.h"

class ChangeFontSizeAction : public IEditorAction {
public:
    ChangeFontSizeAction(std::shared_ptr<moth_ui::Node> node, int oldSize, int newSize);
    virtual ~ChangeFontSizeAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    int m_oldSize;
    int m_newSize;
};
