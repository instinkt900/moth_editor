#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"

#include <string>

class AddDiscreteKeyframeAction : public IEditorAction {
public:
    AddDiscreteKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, std::string value);
    ~AddDiscreteKeyframeAction() override;

    void Do() override;
    void Undo() override;
    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_frameNo;
    std::string value;
    bool m_hadPrevious = false;  ///< True if a keyframe already existed at m_frameNo when Do() ran.
    std::string m_previousValue; ///< Value of the overwritten keyframe, used to restore on Undo().
};
