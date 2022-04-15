#include "common.h"
#include "add_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"

AddKeyframeAction::AddKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo, moth_ui::KeyframeValue value)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , m_value(value) {
}

AddKeyframeAction::~AddKeyframeAction() {
}

void AddKeyframeAction::Do() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto& keyframe = track->GetOrCreateKeyframe(m_frameNo);
    keyframe.m_value = m_value;
}

void AddKeyframeAction::Undo() {
    auto& track = m_entity->m_tracks.at(m_target);
    track->DeleteKeyframe(m_frameNo);
}

void AddKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("AddKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        if (m_value.index() == 0) {
            ImGui::LabelText("Value", "%f", std::get<float>(m_value));
        } else {
            ImGui::LabelText("Value", "%s", std::get<std::string>(m_value).c_str());
        }
    }
}
