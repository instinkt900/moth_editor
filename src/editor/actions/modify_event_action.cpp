#include "common.h"
#include "modify_event_action.h"
#include "moth_ui/layout/layout_entity_group.h"

ModifyEventAction::ModifyEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationMarker const& oldValues, moth_ui::AnimationMarker const& newValues)
    : m_group(group)
    , m_initialValues(oldValues)
    , m_finalValues(newValues) {
}

ModifyEventAction::~ModifyEventAction() {
}

void ModifyEventAction::Do() {
    auto const it = ranges::find_if(m_group->m_events, [&](auto const& event) {
        return event->frame == m_initialValues.frame;
    });

    if (it == ranges::end(m_group->m_events)) {
        return;
    }
    moth_ui::AnimationMarker* targetEvent = it->get();

    if (m_initialValues.frame != m_finalValues.frame) {
        auto const replaceIt = ranges::find_if(m_group->m_events, [&](auto const& event) {
            return event->frame == m_finalValues.frame;
        });
        if (std::end(m_group->m_events) != replaceIt) {
            m_replacedEvent = **replaceIt;
            m_group->m_events.erase(replaceIt);
        }
    }

    *targetEvent = m_finalValues;
}

void ModifyEventAction::Undo() {
    auto const it = ranges::find_if(m_group->m_events, [&](auto const& event) {
        return event->frame == m_finalValues.frame;
    });
    if (it == ranges::end(m_group->m_events)) {
        return;
    }
    moth_ui::AnimationMarker* targetEvent = it->get();
    *targetEvent = m_initialValues;

    if (m_replacedEvent.has_value()) {
        m_group->m_events.push_back(std::make_unique<moth_ui::AnimationMarker>(m_replacedEvent.value()));
    }
}

void ModifyEventAction::OnImGui() {
    if (ImGui::CollapsingHeader("ModifyEventAction")) {
    }
}
