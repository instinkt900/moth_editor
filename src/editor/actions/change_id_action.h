#pragma once

#include "editor_action.h"

class ChangeIdAction : public IEditorAction {
public:
    ChangeIdAction(std::shared_ptr<moth_ui::Node> node, std::string const& oldId, std::string const& newId);
    virtual ~ChangeIdAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    std::string m_oldId;
    std::string m_newId;
};
