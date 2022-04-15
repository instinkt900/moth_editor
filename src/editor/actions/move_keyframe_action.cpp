#include "common.h"
#include "move_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"
#include "moth_ui/utils/imgui_ext_inspect.h"

MoveKeyframeAction::MoveKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int initialFrame, int finalFrame, std::optional<moth_ui::Keyframe> replacedKeyframe)
    : m_entity(entity)
    , m_target(target)
    , m_initialFrame(initialFrame)
    , m_finalFrame(finalFrame)
    , m_replacedKeyframe(replacedKeyframe) {
}

MoveKeyframeAction::~MoveKeyframeAction() {
}

void MoveKeyframeAction::Do() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto keyframe = track->GetOrCreateKeyframe(m_initialFrame);
    if (auto replacedKeyframe = track->GetKeyframe(m_finalFrame)) {
        m_replacedKeyframe = *replacedKeyframe;
        *replacedKeyframe = keyframe;
    } else {
        auto& targetKeyframe = track->GetOrCreateKeyframe(m_finalFrame);
        targetKeyframe = keyframe;
    }
    track->DeleteKeyframe(m_initialFrame);
}

void MoveKeyframeAction::Undo() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto& targetKeyframe = track->GetOrCreateKeyframe(m_initialFrame); // should add
    auto& movingKeyframe = track->GetOrCreateKeyframe(m_finalFrame);   // should exist
    targetKeyframe.m_value = movingKeyframe.m_value;
    if (m_replacedKeyframe.has_value()) {
        movingKeyframe = m_replacedKeyframe.value();
    } else {
        track->DeleteKeyframe(m_finalFrame);
    }
}

void MoveKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("MoveKeyframeAction")) {
        ImGui::LabelText("Initial Frame", "%d", m_initialFrame);
        ImGui::LabelText("Final Frame", "%d", m_finalFrame);
        if (m_replacedKeyframe.has_value()) {
            imgui_ext::Inspect("Replaced keyframe", m_replacedKeyframe.value());
        }
    }
}
