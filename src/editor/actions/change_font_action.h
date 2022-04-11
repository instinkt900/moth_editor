#pragma once

#include "editor_action.h"

class ChangeFontAction : public IEditorAction {
public:
    ChangeFontAction(std::shared_ptr<moth_ui::Node> node, std::string const& oldFont, std::string const& newFont);
    virtual ~ChangeFontAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    std::string m_oldFont;
    std::string m_newFont;
};
