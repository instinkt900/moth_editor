#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"

class DeleteKeyframeAction : public IEditorAction {
public:
    DeleteKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, moth::ui::KeyframeValue oldValue);
    ~DeleteKeyframeAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_frameNo;
    moth::ui::KeyframeValue m_oldValue;
};
