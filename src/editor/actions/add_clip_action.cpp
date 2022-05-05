#include "common.h"
#include "add_clip_action.h"
#include "moth_ui/layout/layout_entity_group.h"

AddClipAction::AddClipAction(std::shared_ptr<moth_ui::LayoutEntityGroup> entity, moth_ui::AnimationClip clip)
    : m_entity(entity)
    , m_clip(clip) {
}

AddClipAction::~AddClipAction() {
}

void AddClipAction::Do() {
    auto& animationClips = m_entity->m_clips;
    animationClips.push_back(std::make_unique<moth_ui::AnimationClip>(m_clip));
}

void AddClipAction::Undo() {
    auto& animationClips = m_entity->m_clips;
    auto it = std::find_if(std::begin(animationClips), std::end(animationClips), [this](auto& target) {
        return *target == m_clip;
    });
    if (std::end(animationClips) != it) {
        animationClips.erase(it);
    }
}

void AddClipAction::OnImGui() {
    if (ImGui::CollapsingHeader("AddClipAction")) {
        //ImGui::LabelText("Frame", "%d", m_frameNo);
        //if (m_oldValue.index() == 0) {
        //    ImGui::LabelText("Old Value", "%f", std::get<float>(m_oldValue));
        //} else {
        //    ImGui::LabelText("Old Value", "%s", std::get<std::string>(m_oldValue).c_str());
        //}
    }
}
