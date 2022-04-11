#pragma once

#include "editor_action.h"

class SetVisibleAction : public IEditorAction {
public:
    SetVisibleAction(std::shared_ptr<moth_ui::Node> node, bool visible);
    virtual ~SetVisibleAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    bool m_visible;
};
