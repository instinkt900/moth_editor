#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation_track.h"

class MoveKeyframeAction : public IEditorAction {
public:
    MoveKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int initialFrame, int finalFrame);
    virtual ~MoveKeyframeAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntity> m_entity;
    moth_ui::AnimationTrack::Target m_target;
    int m_initialFrame;
    int m_finalFrame;
    std::optional<moth_ui::Keyframe> m_replacedKeyframe;
};
