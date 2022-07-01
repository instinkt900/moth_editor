#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation_track.h"

class AddEventAction : public IEditorAction {
public:
    AddEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, int frame, std::string const& name);
    virtual ~AddEventAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_group;
    int m_frame;
    std::string m_name;
};
