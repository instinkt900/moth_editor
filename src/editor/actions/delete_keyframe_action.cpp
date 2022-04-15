#include "common.h"
#include "delete_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"

DeleteKeyframeAction::DeleteKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo, moth_ui::KeyframeValue oldValue)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , m_oldValue(oldValue) {
}

DeleteKeyframeAction::~DeleteKeyframeAction() {
}

void DeleteKeyframeAction::Do() {
    auto& track = m_entity->m_tracks.at(m_target);
    track->DeleteKeyframe(m_frameNo);
}

void DeleteKeyframeAction::Undo() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto& keyframe = track->GetOrCreateKeyframe(m_frameNo);
    keyframe.m_value = m_oldValue;
}

void DeleteKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("DeleteKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        if (m_oldValue.index() == 0) {
            ImGui::LabelText("Old Value", "%f", std::get<float>(m_oldValue));
        } else {
            ImGui::LabelText("Old Value", "%s", std::get<std::string>(m_oldValue).c_str());
        }
    }
}
