#pragma once

#include "editor_action.h"
#include "moth_ui/moth_ui_fwd.h"
#include "moth_ui/animation/animation_track.h"

#include <string>

class MoveDiscreteKeyframeAction : public IEditorAction {
public:
    MoveDiscreteKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int initialFrame, int finalFrame);
    ~MoveDiscreteKeyframeAction() override;

    void Do() override;
    void Undo() override;
    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntity> m_entity;
    moth_ui::AnimationTrack::Target m_target;
    int m_initialFrame;
    int m_finalFrame;
    std::string m_movedValue;
    std::string m_displacedValue; ///< Value that was at finalFrame before the move (for undo).
    bool m_hadDisplaced = false;
};
