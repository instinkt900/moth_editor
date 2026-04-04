#pragma once

#include "editor_action.h"
#include "moth_ui/moth_ui_fwd.h"
#include "moth_ui/animation/animation_track.h"

#include <string>

class DeleteDiscreteKeyframeAction : public IEditorAction {
public:
    DeleteDiscreteKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo, std::string oldValue);
    ~DeleteDiscreteKeyframeAction() override;

    void Do() override;
    void Undo() override;
    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntity> m_entity;
    moth_ui::AnimationTrack::Target m_target;
    int m_frameNo;
    std::string m_oldValue;
};
