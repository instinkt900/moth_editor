#pragma once

#include "uilib/animation_track.h"
#include "keyframe_context.h"
#include "keyframe_widget.h"
#include "uilib/animation_clip.h"
#include "uilib/utils/imgui_ext_focus.h"
#include "uilib/ui_fwd.h"

namespace ui {
    class EditorLayer;
    class CompositeAction;

    class AnimationWidget {
    public:
        AnimationWidget(EditorLayer& editorLayer);

        void Draw();

        void OnUndo();
        void OnRedo();

    private:
        EditorLayer& m_editorLayer;
        Group* m_group = nullptr;

        bool DrawWidget();
        char const* GetChildLabel(int index) const;
        char const* GetTrackLabel(AnimationTrack::Target target) const;

        imgui_ext::FocusGroupContext m_clipInputFocusContext;
        AnimationClip* m_targetClip = nullptr;
        AnimationClip m_preModifyClipValues;
        void BeginEditClip(AnimationClip& clip);
        void EndEditClip();

        bool m_clipWindowShown = true;
        void DrawSelectedClipWindow();

        std::vector<KeyframeContext> m_selectedKeyframes;
        KeyframeWidget m_keyframeWidget;
        AnimationClip* m_selectedClip = nullptr;

        void SelectKeyframe(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo);
        void DeselectKeyframe(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo);
        bool IsKeyframeSelected(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo);
        void ClearSelectedKeyframes();
        void ClearNonMatchingKeyframes(std::shared_ptr<LayoutEntity> entity, int frameNo);

        void DeleteSelectedKeyframes();

        void EndMoveKeyframes();

        int m_minFrame = 0;
        int m_maxFrame = 100;
        int m_currentFrame = 0;
        int m_firstFrame = 0;

        std::vector<bool> m_childExpanded;
    };
}
