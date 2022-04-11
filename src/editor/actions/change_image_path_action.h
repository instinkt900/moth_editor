#pragma once

#include "editor_action.h"

class ChangeImagePathAction : public IEditorAction {
public:
    ChangeImagePathAction(std::shared_ptr<moth_ui::Node> node, std::string const& oldPath, std::string const& newPath);
    virtual ~ChangeImagePathAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    std::string m_oldPath;
    std::string m_newPath;
};
