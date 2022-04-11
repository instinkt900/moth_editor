#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"

class ChangeTextAction : public IEditorAction {
public:
    ChangeTextAction(std::shared_ptr<moth_ui::NodeText> node, std::string const& oldText, std::string const& newText);
    virtual ~ChangeTextAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::NodeText> m_node;
    std::string m_oldText;
    std::string m_newText;
};
