#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation_track.h"

class DeleteKeyframeAction : public IEditorAction {
public:
    DeleteKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo, moth_ui::KeyframeValue oldValue);
    virtual ~DeleteKeyframeAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntity> m_entity;
    moth_ui::AnimationTrack::Target m_target;
    int m_frameNo;
    moth_ui::KeyframeValue m_oldValue;
};
