#pragma once

#include "editor_action.h"

class SetImageScaleAction : public IEditorAction {
public:
    SetImageScaleAction(std::shared_ptr<moth_ui::Node> node, float oldScale, float newScale);
    virtual ~SetImageScaleAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    float m_oldScale;
    float m_newScale;
};
