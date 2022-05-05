#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation_clip.h"

class DeleteClipAction : public IEditorAction {
public:
    DeleteClipAction(std::shared_ptr<moth_ui::LayoutEntityGroup> entity, moth_ui::AnimationClip clip);
    virtual ~DeleteClipAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_entity;
    moth_ui::AnimationClip m_clip;
};
