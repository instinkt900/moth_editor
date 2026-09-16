#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_track.h"

class AddEventAction : public IEditorAction {
public:
    AddEventAction(std::shared_ptr<moth::ui::LayoutEntityGroup> group, int frame, std::string const& name);
    ~AddEventAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntityGroup> m_group;
    int m_frame;
    std::string m_name;
};
