#pragma once

#include "moth_ui/animation_track.h"
#include "keyframe_context.h"
#include "keyframe_widget.h"
#include "moth_ui/animation_clip.h"
#include "moth_ui/utils/imgui_ext_focus.h"
#include "moth_ui/ui_fwd.h"

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
    moth_ui::Group* m_group = nullptr;

    bool DrawWidget();
    char const* GetChildLabel(int index) const;
    char const* GetTrackLabel(moth_ui::AnimationTrack::Target target) const;

    imgui_ext::FocusGroupContext m_clipInputFocusContext;
    moth_ui::AnimationClip* m_targetClip = nullptr;
    moth_ui::AnimationClip m_preModifyClipValues;
    void BeginEditClip(moth_ui::AnimationClip& clip);
    void EndEditClip();

    bool m_clipWindowShown = true;
    void DrawSelectedClipWindow();

    std::vector<KeyframeContext> m_selectedKeyframes;
    KeyframeWidget m_keyframeWidget;
    moth_ui::AnimationClip* m_selectedClip = nullptr;

    void SelectKeyframe(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    void DeselectKeyframe(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    bool IsKeyframeSelected(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    void ClearSelectedKeyframes();
    void ClearNonMatchingKeyframes(std::shared_ptr<moth_ui::LayoutEntity> entity, int frameNo);

    void DeleteSelectedKeyframes();

    void EndMoveKeyframes();

    int m_minFrame = 0;
    int m_maxFrame = 100;
    int m_currentFrame = 0;
    int m_firstFrame = 0;

    std::vector<bool> m_childExpanded;
};
