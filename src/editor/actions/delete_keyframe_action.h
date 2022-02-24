#pragma once

#include "editor_action.h"
#include "uilib/ui_fwd.h"
#include "uilib/animation_track.h"

namespace ui {
    class DeleteKeyframeAction : public IEditorAction {
    public:
        DeleteKeyframeAction(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo, KeyframeValue oldValue);
        virtual ~DeleteKeyframeAction();

        void Do() override;
        void Undo() override;

        void OnImGui() override;

    protected:
        std::shared_ptr<LayoutEntity> m_entity;
        AnimationTrack::Target m_target;
        int m_frameNo;
        KeyframeValue m_oldValue;
    };
}
