#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"
#include "moth/ui/utils/interp.h"

class AddKeyframeAction : public IEditorAction {
public:
    AddKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, moth::ui::KeyframeValue value, moth::ui::InterpType interp);
    ~AddKeyframeAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_frameNo;
    moth::ui::KeyframeValue value;
    moth::ui::InterpType m_interp;
};
