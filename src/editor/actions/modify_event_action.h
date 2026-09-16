#pragma once

#include "editor_action.h"
#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/animation/animation_marker.h"

#include <optional>

class ModifyEventAction : public IEditorAction {
public:
    ModifyEventAction(std::shared_ptr<moth::ui::LayoutEntityGroup> group, moth::ui::AnimationMarker const& oldValues, moth::ui::AnimationMarker const& newValues);
    ~ModifyEventAction() override;

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth::ui::LayoutEntityGroup> m_group;
    moth::ui::AnimationMarker m_initialValues;
    moth::ui::AnimationMarker m_finalValues;
    std::optional<moth::ui::AnimationMarker> m_replacedEvent;
};
