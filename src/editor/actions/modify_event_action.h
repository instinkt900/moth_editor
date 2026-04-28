#pragma once

#include "editor_action.h"
#include "moth_ui/moth_ui_fwd.h"
#include "moth_ui/animation/animation_marker.h"

#include <optional>

class ModifyEventAction : public IEditorAction {
public:
    ModifyEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationMarker const& oldValues, moth_ui::AnimationMarker const& newValues);
    ~ModifyEventAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_group;
    moth_ui::AnimationMarker m_initialValues;
    moth_ui::AnimationMarker m_finalValues;
    std::optional<moth_ui::AnimationMarker> m_replacedEvent;
};
