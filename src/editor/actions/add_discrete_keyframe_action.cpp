#include "common.h"
#include "add_discrete_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"
#include "moth_ui/animation/discrete_animation_track.h"

AddDiscreteKeyframeAction::AddDiscreteKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo, std::string value)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , m_value(std::move(value)) {
}

AddDiscreteKeyframeAction::~AddDiscreteKeyframeAction() = default;

void AddDiscreteKeyframeAction::Do() {
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it == m_entity->m_discreteTracks.end()) {
        auto [newIt, ok] = m_entity->m_discreteTracks.emplace(m_target, moth_ui::DiscreteAnimationTrack(m_target));
        it = newIt;
    }
    it->second.GetOrCreateKeyframe(m_frameNo) = m_value;
}

void AddDiscreteKeyframeAction::Undo() {
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it != m_entity->m_discreteTracks.end()) {
        it->second.DeleteKeyframe(m_frameNo);
    }
}

void AddDiscreteKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("AddDiscreteKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        ImGui::LabelText("Value", "%s", m_value.c_str());
    }
}
