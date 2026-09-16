#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"
#include "moth/ui/utils/interp.h"

class ModifyKeyframeAction : public IEditorAction {
public:
    ModifyKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity,
        moth::ui::AnimationTrack::Target target,
        int frameNo,
        moth::ui::KeyframeValue oldValue,
        moth::ui::KeyframeValue newValue,
        moth::ui::InterpType oldInterp,
        moth::ui::InterpType newInterp);
    ~ModifyKeyframeAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_frameNo;
    moth::ui::KeyframeValue m_oldValue;
    moth::ui::KeyframeValue m_newValue;
    moth::ui::InterpType m_oldInterp;
    moth::ui::InterpType m_newInterp;
};
