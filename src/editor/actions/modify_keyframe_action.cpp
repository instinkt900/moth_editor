#include "common.h"
#include "modify_keyframe_action.h"
#include "moth/ui/layout/layout_entity.h"
#include "moth/ui/animation/keyframe.h"

ModifyKeyframeAction::ModifyKeyframeAction(std::shared_ptr<moth::ui::LayoutEntity> entity,
                                           moth::ui::AnimationTrack::Target target,
                                           int frameNo,
                                           moth::ui::KeyframeValue oldValue,
                                           moth::ui::KeyframeValue newValue,
                                           moth::ui::InterpType oldInterp,
                                           moth::ui::InterpType newInterp)
    : m_entity(entity)
    , m_target(target)
    , m_frameNo(frameNo)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
    , m_oldInterp(oldInterp)
    , m_newInterp(newInterp) {
}

ModifyKeyframeAction::~ModifyKeyframeAction() {
}

void ModifyKeyframeAction::Do() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto* keyframe = track->GetKeyframe(m_frameNo);
    if (keyframe == nullptr) { return; }
    keyframe->value = m_newValue;
    keyframe->interpType = m_newInterp;
}

void ModifyKeyframeAction::Undo() {
    auto& track = m_entity->m_tracks.at(m_target);
    auto* keyframe = track->GetKeyframe(m_frameNo);
    if (keyframe == nullptr) { return; }
    keyframe->value = m_oldValue;
    keyframe->interpType = m_oldInterp;
}

void ModifyKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("ModifyKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        ImGui::LabelText("Old Value", "%f", m_oldValue);
        ImGui::LabelText("New Value", "%f", m_newValue);
        // TODO interp (maybe? this is mostly debug)
    }
}
