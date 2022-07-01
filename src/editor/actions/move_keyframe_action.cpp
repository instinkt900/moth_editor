#include "common.h"
#include "move_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"
#include "../imgui_ext_inspect.h"

MoveKeyframeAction::MoveKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int initialFrame, int finalFrame)
    : m_entity(entity)
    , m_target(target)
    , m_initialFrame(initialFrame)
    , m_finalFrame(finalFrame) {
}

MoveKeyframeAction::~MoveKeyframeAction() {
}

void MoveKeyframeAction::Do() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto keyframe = track->GetKeyframe(m_initialFrame);
    if (auto replacedKeyframe = track->GetKeyframe(m_finalFrame)) {
        m_replacedKeyframe = *replacedKeyframe;
        track->DeleteKeyframe(m_finalFrame);
    }
    keyframe->m_frame = m_finalFrame;
    track->SortKeyframes();
}

void MoveKeyframeAction::Undo() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto keyframe = track->GetKeyframe(m_finalFrame);
    keyframe->m_frame = m_initialFrame;
    if (m_replacedKeyframe.has_value()) {
        auto replacedKeyframe = track->GetOrCreateKeyframe(m_finalFrame);
        replacedKeyframe = m_replacedKeyframe.value();
    }
    track->SortKeyframes();
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
