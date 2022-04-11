#pragma once

#include "editor_action.h"
#include "moth_ui/blend_mode.h"

class SetBlendModeAction : public IEditorAction {
public:
    SetBlendModeAction(std::shared_ptr<moth_ui::Node> node, moth_ui::BlendMode oldBlendMode, moth_ui::BlendMode newBlendMode);
    virtual ~SetBlendModeAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    moth_ui::BlendMode m_oldBlendMode;
    moth_ui::BlendMode m_newBlendMode;
};
