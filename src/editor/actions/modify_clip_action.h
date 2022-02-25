#pragma once

#include "editor_action.h"
#include "moth_ui/animation_clip.h"

class ModifyClipAction : public IEditorAction {
public:
    ModifyClipAction(moth_ui::AnimationClip* target, moth_ui::AnimationClip const& initialValues);
    virtual ~ModifyClipAction() = default;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

private:
    moth_ui::AnimationClip* m_target;
    moth_ui::AnimationClip m_initialValues;
    moth_ui::AnimationClip m_finalValues;
};
