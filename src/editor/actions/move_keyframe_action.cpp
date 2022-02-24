#include "common.h"
#include "move_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"

namespace ui {
    MoveKeyframeAction::MoveKeyframeAction(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int initialFrame, int finalFrame, std::optional<KeyframeValue> replacedValue)
        : m_entity(entity)
        , m_target(target)
        , m_initialFrame(initialFrame)
        , m_finalFrame(finalFrame)
        , m_replacedValue(replacedValue) {
    }

    MoveKeyframeAction::~MoveKeyframeAction() {
    }

    void MoveKeyframeAction::Do() {
        auto& track = m_entity->GetAnimationTracks().at(m_target);
        auto keyframe = track->GetOrCreateKeyframe(m_initialFrame);
        if (auto replacedKeyframe = track->GetKeyframe(m_finalFrame)) {
            m_replacedValue = replacedKeyframe->m_value;
            replacedKeyframe->m_value = keyframe.m_value;
        } else {
            auto& targetKeyframe = track->GetOrCreateKeyframe(m_finalFrame);
            targetKeyframe.m_value = keyframe.m_value;
        }
        track->DeleteKeyframe(m_initialFrame);
    }

    void MoveKeyframeAction::Undo() {
        auto& track = m_entity->GetAnimationTracks().at(m_target);
        auto& targetKeyframe = track->GetOrCreateKeyframe(m_initialFrame); // should add
        auto& movingKeyframe = track->GetOrCreateKeyframe(m_finalFrame); // should exist
        targetKeyframe.m_value = movingKeyframe.m_value;
        if (m_replacedValue.has_value()) {
            movingKeyframe.m_value = m_replacedValue.value();
        } else {
            track->DeleteKeyframe(m_finalFrame);
        }
    }

    void MoveKeyframeAction::OnImGui() {
        if (ImGui::CollapsingHeader("MoveKeyframeAction")) {
            ImGui::LabelText("Initial Frame", "%d", m_initialFrame);
            ImGui::LabelText("Final Frame", "%d", m_finalFrame);
            if (m_replacedValue.has_value()) {
                if (m_replacedValue.value().index() == 0) {
                    ImGui::LabelText("Replaced Value", "%f", std::get<float>(m_replacedValue.value()));
                } else {
                    ImGui::LabelText("Replaced Value", "%s", std::get<std::string>(m_replacedValue.value()).c_str());
                }
            }
        }
    }
}
