#include "common.h"
#include "delete_discrete_keyframe_action.h"
#include "moth/ui/layout/layout_entity.h"
#include "moth/ui/animation/discrete_animation_track.h"

DeleteDiscreteKeyframeAction::DeleteDiscreteKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, std::string oldValue)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , m_oldValue(std::move(oldValue)) {
}

DeleteDiscreteKeyframeAction::~DeleteDiscreteKeyframeAction() = default;

void DeleteDiscreteKeyframeAction::Do() {
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it != m_entity->m_discreteTracks.end()) {
        it->second.DeleteKeyframe(m_frameNo);
    }
}

void DeleteDiscreteKeyframeAction::Undo() {
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it == m_entity->m_discreteTracks.end()) {
        auto [newIt, ok] = m_entity->m_discreteTracks.emplace(m_target, moth::ui::DiscreteAnimationTrack(m_target));
        it = newIt;
    }
    it->second.GetOrCreateKeyframe(m_frameNo) = m_oldValue;
}

void DeleteDiscreteKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("DeleteDiscreteKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        ImGui::LabelText("OldValue", "%s", m_oldValue.c_str());
    }
}
