#pragma once

#include "editor_action.h"
#include "moth_ui/animation_clip.h"

namespace ui {
    class ModifyClipAction : public IEditorAction {
    public:
        ModifyClipAction(AnimationClip* target, AnimationClip const& initialValues);
        virtual ~ModifyClipAction() = default;

        void Do() override;
        void Undo() override;

        void OnImGui() override;

    private:
        AnimationClip* m_target;
        AnimationClip m_initialValues;
        AnimationClip m_finalValues;
    };
}
