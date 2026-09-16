#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_marker.h"

class DeleteEventAction : public IEditorAction {
public:
    DeleteEventAction(std::shared_ptr<moth::ui::LayoutEntityGroup> group, moth::ui::AnimationMarker const& event);
    ~DeleteEventAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntityGroup> m_group;
    moth::ui::AnimationMarker m_event;
};
