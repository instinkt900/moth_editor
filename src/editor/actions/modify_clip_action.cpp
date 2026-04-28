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
        return clip->startFrame == m_initialValues.startFrame
            && clip->endFrame == m_initialValues.endFrame
            && clip->name == m_initialValues.name 
            && clip->loopType == m_initialValues.loopType
            && clip->fps == m_initialValues.fps;
        // clang-format on
    });
    
    if (it != ranges::end(m_group->m_clips)) {
        **it =m_finalValues;
    }
}

void ModifyClipAction::Undo() {
    auto const it = ranges::find_if(m_group->m_clips, [&](auto const& clip) {
        // clang-format off
        return clip->startFrame == m_finalValues.startFrame
            && clip->endFrame == m_finalValues.endFrame
            && clip->name == m_finalValues.name
            && clip->loopType == m_finalValues.loopType
            && clip->fps == m_finalValues.fps;
        // clang-format on
    });

    if (it != ranges::end(m_group->m_clips)) {
        **it =m_initialValues;
    }
}

void ModifyClipAction::OnImGui() {
    if (ImGui::CollapsingHeader("ModifyClipAction")) {
    }
}
