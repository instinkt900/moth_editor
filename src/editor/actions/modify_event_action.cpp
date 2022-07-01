#include "common.h"
#include "modify_event_action.h"
#include "moth_ui/layout/layout_entity_group.h"

ModifyEventAction::ModifyEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationEvent const& oldValues, moth_ui::AnimationEvent const& newValues)
    : m_group(group)
    , m_initialValues(oldValues)
    , m_finalValues(newValues) {
}

ModifyEventAction::~ModifyEventAction() {
}

void ModifyEventAction::Do() {
    auto const it = ranges::find_if(m_group->m_events, [&](auto const& event) {
        return event->m_frame == m_initialValues.m_frame;
    });

    moth_ui::AnimationEvent* targetEvent = it->get();

    if (m_initialValues.m_frame != m_finalValues.m_frame) {
        auto const replaceIt = ranges::find_if(m_group->m_events, [&](auto const& event) {
            return event->m_frame == m_finalValues.m_frame;
        });
        if (std::end(m_group->m_events) != replaceIt) {
            m_replacedEvent = *(replaceIt->get());
            m_group->m_events.erase(replaceIt);
        }
    }

    *targetEvent = m_finalValues;
}

void ModifyEventAction::Undo() {
    auto const it = ranges::find_if(m_group->m_events, [&](auto const& event) {
        return event->m_frame == m_finalValues.m_frame;
    });
    moth_ui::AnimationEvent* targetEvent = it->get();
    *targetEvent = m_initialValues;

    if (m_replacedEvent.has_value()) {
        m_group->m_events.push_back(std::make_unique<moth_ui::AnimationEvent>(m_replacedEvent.value()));
    }
}

void ModifyEventAction::OnImGui() {
    if (ImGui::CollapsingHeader("ModifyEventAction")) {
    }
}
