#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"

#include <string>

class MoveDiscreteKeyframeAction : public IEditorAction {
public:
    MoveDiscreteKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int initialFrame, int finalFrame);
    ~MoveDiscreteKeyframeAction() override;

    void Do() override;
    void Undo() override;
    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntity> m_entity;
    moth::ui::AnimationTrack::Target m_target;
    int m_initialFrame;
    int m_finalFrame;
    std::string m_movedValue;
    std::string m_displacedValue; ///< Value that was at finalFrame before the move (for undo).
    bool m_hadDisplaced = false;
    bool m_didMove = false; ///< Set to true only when Do() successfully performed the move.
};
