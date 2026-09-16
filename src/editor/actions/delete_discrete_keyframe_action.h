#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"

#include <string>

class DeleteDiscreteKeyframeAction : public IEditorAction {
public:
    DeleteDiscreteKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, std::string oldValue);
    ~DeleteDiscreteKeyframeAction() override;

    void Do() override;
    void Undo() override;
    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_frameNo;
    std::string m_oldValue;
};
