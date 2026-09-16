#pragma once

#include "editor_action.h"

class DeleteAction : public IEditorAction {
public:
    DeleteAction(std::shared_ptr<moth::ui::Node> deletedNode, std::shared_ptr<moth::ui::Group> parentNode);
    ~DeleteAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::Node> m_deletedNode;
    std::shared_ptr<moth::ui::Group> m_parentNode;
    int m_originalIndex = 0;
};
