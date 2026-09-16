#pragma once

#include "editor_action.h"
#include "moth/ui/animation/animation_clip.h"

class ModifyClipAction : public IEditorAction {
public:
    ModifyClipAction(std::shared_ptr<moth::ui::LayoutEntityGroup> group, moth::ui::AnimationClip const& oldValues, moth::ui::AnimationClip const& newValues);
    ~ModifyClipAction() override = default;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

private:
    std::shared_ptr<moth::ui::LayoutEntityGroup> m_group;
    moth::ui::AnimationClip m_initialValues;
    moth::ui::AnimationClip m_finalValues;
};
