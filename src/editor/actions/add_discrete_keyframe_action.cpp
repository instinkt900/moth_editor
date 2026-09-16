#include "common.h"
#include "add_discrete_keyframe_action.h"
#include "moth/ui/layout/layout_entity.h"
#include "moth/ui/animation/discrete_animation_track.h"

AddDiscreteKeyframeAction::AddDiscreteKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity, moth::ui::AnimationTrack::Target target, int frameNo, std::string value)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , value(std::move(value)) {
}

AddDiscreteKeyframeAction::~AddDiscreteKeyframeAction() = default;

void AddDiscreteKeyframeAction::Do() {
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it == m_entity->m_discreteTracks.end()) {
        auto [newIt, ok] = m_entity->m_discreteTracks.emplace(m_target, moth::ui::DiscreteAnimationTrack(m_target));
        it = newIt;
    }
    auto* existing = it->second.GetKeyframe(m_frameNo);
    if (existing != nullptr) {
        m_hadPrevious = true;
        m_previousValue = *existing;
    } else {
        m_hadPrevious = false;
        m_previousValue.clear();
    }
    it->second.GetOrCreateKeyframe(m_frameNo) = value;
}

void AddDiscreteKeyframeAction::Undo() {
    auto it = m_entity->m_discreteTracks.find(m_target);
    if (it == m_entity->m_discreteTracks.end()) {
        return;
    }
    if (m_hadPrevious) {
        it->second.GetOrCreateKeyframe(m_frameNo) = m_previousValue;
    } else {
        it->second.DeleteKeyframe(m_frameNo);
    }
}

void AddDiscreteKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("AddDiscreteKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        ImGui::LabelText("Value", "%s", value.c_str());
    }
}
