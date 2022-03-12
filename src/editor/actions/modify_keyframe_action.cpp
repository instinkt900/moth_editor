#include "common.h"
#include "modify_keyframe_action.h"
#include "moth_ui/layout/layout_entity.h"

ModifyKeyframeAction::ModifyKeyframeAction(std::shared_ptr<moth_ui::LayoutEntity> entity,
                                           moth_ui::AnimationTrack::Target target,
                                           int frameNo,
                                           moth_ui::KeyframeValue oldValue,
                                           moth_ui::KeyframeValue newValue,
                                           moth_ui::InterpType oldInterp,
                                           moth_ui::InterpType newInterp)
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
    auto& track = m_entity->GetAnimationTracks().at(m_target);
    auto keyframe = track->GetKeyframe(m_frameNo);
    keyframe->m_value = m_newValue;
    keyframe->m_interpType = m_newInterp;
}

void ModifyKeyframeAction::Undo() {
    auto& track = m_entity->GetAnimationTracks().at(m_target);
    auto keyframe = track->GetKeyframe(m_frameNo);
    keyframe->m_value = m_oldValue;
    keyframe->m_interpType = m_oldInterp;
}

void ModifyKeyframeAction::OnImGui() {
    if (ImGui::CollapsingHeader("ModifyKeyframeAction")) {
        ImGui::LabelText("Frame", "%d", m_frameNo);
        if (m_oldValue.index() == 0) {
            ImGui::LabelText("Old Value", "%f", std::get<float>(m_oldValue));
        } else {
            ImGui::LabelText("Old Value", "%s", std::get<std::string>(m_oldValue).c_str());
        }
        if (m_newValue.index() == 0) {
            ImGui::LabelText("New Value", "%f", std::get<float>(m_newValue));
        } else {
            ImGui::LabelText("New Value", "%s", std::get<std::string>(m_newValue).c_str());
        }
        // TODO interp (maybe? this is mostly debug)
    }
}
