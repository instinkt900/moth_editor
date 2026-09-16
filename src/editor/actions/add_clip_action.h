#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_clip.h"

class AddClipAction : public IEditorAction {
public:
    AddClipAction(std::shared_ptr<moth::ui::LayoutEntityGroup> entity, moth::ui::AnimationClip clip);
    ~AddClipAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntityGroup> m_entity;
    moth::ui::AnimationClip m_clip;
};
