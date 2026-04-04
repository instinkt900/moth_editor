#include "common.h"
#include "move_discrete_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"
#include "moth_ui/animation/discrete_animation_track.h"

MoveDiscreteKeyframeAction::MoveDiscreteKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int initialFrame, int finalFrame)
    : m_entity(entity)
    , m_target(target)
    , m_initialFrame(initialFrame)
    , m_finalFrame(finalFrame) {
}

MoveDiscreteKeyframeAction::~MoveDiscreteKeyframeAction() = default;

void MoveDiscreteKeyframeAction::Do() {
    m_didMove = false;
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it == m_entity->m_discreteTracks.end()) {
        return;
    }
    auto& track = it->second;
    auto* src = track.GetKeyframe(m_initialFrame);
    if (src == nullptr) {
        return;
    }
    m_movedValue = *src;

    auto* dst = track.GetKeyframe(m_finalFrame);
    if (dst != nullptr) {
        m_displacedValue = *dst;
        m_hadDisplaced = true;
    } else {
        m_hadDisplaced = false;
    }

    track.DeleteKeyframe(m_initialFrame);
    track.GetOrCreateKeyframe(m_finalFrame) = m_movedValue;
    m_didMove = true;
}

void MoveDiscreteKeyframeAction::Undo() {
    if (!m_didMove) {
        return;
    }
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it == m_entity->m_discreteTracks.end()) {
        return;
    }
    auto& track = it->second;
    track.DeleteKeyframe(m_finalFrame);
    track.GetOrCreateKeyframe(m_initialFrame) = m_movedValue;
    if (m_hadDisplaced) {
        track.GetOrCreateKeyframe(m_finalFrame) = m_displacedValue;
    }
}

void MoveDiscreteKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("MoveDiscreteKeyframeAction")) {
        ImGui::LabelText("InitialFrame", "%d", m_initialFrame);
        ImGui::LabelText("FinalFrame", "%d", m_finalFrame);
    }
}
