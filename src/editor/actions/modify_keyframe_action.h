#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation/animation_track.h"
#include "moth_ui/utils/interp.h"

class ModifyKeyframeAction : public IEditorAction {
public:
    ModifyKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity,
        moth_ui::AnimationTrack::Target target,
        int frameNo,
        moth_ui::KeyframeValue oldValue,
        moth_ui::KeyframeValue newValue,
        moth_ui::InterpType oldInterp,
        moth_ui::InterpType newInterp);
    virtual ~ModifyKeyframeAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntity> m_entity;
    moth_ui::AnimationTrack::Target m_target;
    int m_frameNo;
    moth_ui::KeyframeValue m_oldValue;
    moth_ui::KeyframeValue m_newValue;
    moth_ui::InterpType m_oldInterp;
    moth_ui::InterpType m_newInterp;
};
