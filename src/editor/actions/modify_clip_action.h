#pragma once

#include "editor_action.h"
#include "moth_ui/animation_clip.h"

class ModifyClipAction : public IEditorAction {
public:
    ModifyClipAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationClip const& oldValues, moth_ui::AnimationClip const& newValues);
    virtual ~ModifyClipAction() = default;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

private:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_group;
    moth_ui::AnimationClip m_initialValues;
    moth_ui::AnimationClip m_finalValues;
};
