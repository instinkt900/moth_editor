#include "common.h"
#include "add_keyframe_action.h"
#include "moth/ui/layout/layout_entity.h"
#include "moth/ui/animation/keyframe.h"

AddKeyframeAction::AddKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, moth::ui::KeyframeValue value, moth::ui::InterpType interp)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , value(value)
    , m_interp(interp) {
}

AddKeyframeAction::~AddKeyframeAction() {
}

void AddKeyframeAction::Do() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto& keyframe = track->GetOrCreateKeyframe(m_frameNo);
    keyframe.value = value;
    keyframe.interpType = m_interp;
}

void AddKeyframeAction::Undo() {
    auto& track = m_entity->m_tracks.at(m_target);
    track->DeleteKeyframe(m_frameNo);
}

void AddKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("AddKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        ImGui::LabelText("Value", "%f", value);
    }
}
