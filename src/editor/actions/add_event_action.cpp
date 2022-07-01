#include "common.h"
#include "add_event_action.h"
#include "moth_ui/layout/layout_entity_group.h"

AddEventAction::AddEventAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, int frame, std::string const& name)
    : m_group(group)
    , m_frame(frame)
    , m_name(name) {
}

AddEventAction::~AddEventAction() {
}

void AddEventAction::Do() {
    m_group->m_events.push_back(std::make_unique<moth_ui::AnimationEvent>(m_frame, m_name));
}

void AddEventAction::Undo() {
    auto const it = ranges::find_if(m_group->m_events, [&](auto& eventPtr) {
        return eventPtr->m_frame == m_frame && eventPtr->m_name == m_name;
    });
    if (std::end(m_group->m_events) != it) {
        m_group->m_events.erase(it);
    }
}

void AddEventAction::OnImGui() {
    if (ImGui::CollapsingHeader("AddEventAction")) {
        ImGui::LabelText("Frame", "%d", m_frame);
        ImGui::LabelText("Name", "%s", m_name.c_str());
    }
}
