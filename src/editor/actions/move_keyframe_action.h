#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"
#include "moth/ui/animation/keyframe.h"

#include <optional>

class MoveKeyframeAction : public IEditorAction {
public:
    MoveKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int initialFrame, int finalFrame);
    ~MoveKeyframeAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_initialFrame;
    int m_finalFrame;
    std::optional<moth::ui::Keyframe> m_replacedKeyframe;
};
