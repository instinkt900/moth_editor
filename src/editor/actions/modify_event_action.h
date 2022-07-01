#pragma once

#include "editor_action.h"
#include "moth_ui/ui_fwd.h"
#include "moth_ui/animation_track.h"

class ModifyEventAction : public IEditorAction {
public:
    ModifyEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationEvent const& oldValues, moth_ui::AnimationEvent const& newValues);
    virtual ~ModifyEventAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_group;
    moth_ui::AnimationEvent m_initialValues;
    moth_ui::AnimationEvent m_finalValues;
    std::optional<moth_ui::AnimationEvent> m_replacedEvent;
};
