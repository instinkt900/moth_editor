#include "common.h"
#include "modify_clip_action.h"
#include "moth_ui/layout/layout_entity_group.h"

ModifyClipAction::ModifyClipAction(std::shared_ptr<moth_ui::LayoutEntityGroup> group, moth_ui::AnimationClip const& oldValues, moth_ui::AnimationClip const& newValues)
    : m_group(group)
    , m_initialValues(oldValues)
    , m_finalValues(newValues) {
}

void ModifyClipAction::Do() {
    auto const it = ranges::find_if(m_group->m_clips, [&](auto const& clip) {
        // clang-format off
        return clip->m_startFrame == m_initialValues.m_startFrame
            && clip->m_endFrame == m_initialValues.m_endFrame
            && clip->m_name == m_initialValues.m_name 
            && clip->m_loopType == m_initialValues.m_loopType
            && clip->m_fps == m_initialValues.m_fps;
        // clang-format on
    });
    
    moth_ui::AnimationClip* targetClip = it->get();
    *targetClip = m_finalValues;
}

void ModifyClipAction::Undo() {
    auto const it = ranges::find_if(m_group->m_clips, [&](auto const& clip) {
        // clang-format off
        return clip->m_startFrame == m_finalValues.m_startFrame
            && clip->m_endFrame == m_finalValues.m_endFrame
            && clip->m_name == m_finalValues.m_name 
            && clip->m_loopType == m_finalValues.m_loopType
            && clip->m_fps == m_finalValues.m_fps;
        // clang-format on
    });

    moth_ui::AnimationClip* targetClip = it->get();
    *targetClip = m_initialValues;
}

void ModifyClipAction::OnImGui() {
    if (ImGui::CollapsingHeader("ModifyClipAction")) {
    }
}
