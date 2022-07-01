#include "common.h"
#include "delete_event_action.h"
#include "moth_ui/layout/layout_entity_group.h"

DeleteEventAction::DeleteEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationEvent const& event)
    : m_group(group)
    , m_event(event) {
}

DeleteEventAction::~DeleteEventAction() {
}

void DeleteEventAction::Do() {
    auto const it = ranges::find_if(m_group->m_events, [&](auto& eventPtr) {
        return eventPtr->m_frame == m_event.m_frame && eventPtr->m_name == m_event.m_name;
    });
    if (std::end(m_group->m_events) != it) {
        m_group->m_events.erase(it);
    }
}

void DeleteEventAction::Undo() {
    m_group->m_events.push_back(std::make_unique<moth_ui::AnimationEvent>(m_event));
}

void DeleteEventAction::OnImGui() {
    if (ImGui::CollapsingHeader("DeleteEventAction")) {
        ImGui::LabelText("Frame", "%d", m_event.m_frame);
        ImGui::LabelText("Name", "%s", m_event.m_name.c_str());
    }
}
