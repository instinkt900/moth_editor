#pragma once

#include "editor_action.h"
#include "uilib/ui_fwd.h"
#include "uilib/animation_track.h"

namespace ui {
    class ModifyKeyframeAction : public IEditorAction {
    public:
        ModifyKeyframeAction(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo, KeyframeValue oldValue, KeyframeValue newValue);
        virtual ~ModifyKeyframeAction();

        void Do() override;
        void Undo() override;

        void OnImGui() override;

    protected:
        std::shared_ptr<LayoutEntity> m_entity;
        AnimationTrack::Target m_target;
        int m_frameNo;
        KeyframeValue m_oldValue;
        KeyframeValue m_newValue;
    };
}
