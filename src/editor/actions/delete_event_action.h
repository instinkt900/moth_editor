#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation_track.h"

class DeleteEventAction : public IEditorAction {
public:
    DeleteEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationEvent const& event);
    virtual ~DeleteEventAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_group;
    moth_ui::AnimationEvent m_event;
};
