#pragma once

#include "editor_action.h"
#include "moth_ui/moth_ui_fwd.h"
#include "moth_ui/animation/animation_marker.h"

class DeleteEventAction : public IEditorAction {
public:
    DeleteEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationMarker const& event);
    ~DeleteEventAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_group;
    moth_ui::AnimationMarker m_event;
};
