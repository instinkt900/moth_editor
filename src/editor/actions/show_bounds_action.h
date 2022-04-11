#pragma once

#include "editor_action.h"

class ShowBoundsAction : public IEditorAction {
public:
    ShowBoundsAction(std::shared_ptr<moth_ui::Node> node, bool visible);
    virtual ~ShowBoundsAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    bool m_visible;
};
