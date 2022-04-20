#pragma once

#include "editor_panel.h"
#include "moth_ui/animation_track.h"
#include "moth_ui/animation_clip.h"
#include "moth_ui/utils/imgui_ext_focus.h"
#include "../keyframe_context.h"

class EditorPanelAnimation : public EditorPanel {
public:
    EditorPanelAnimation(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelAnimation() = default;

    std::vector<KeyframeContext>& GetSelectedKeyframes() { return m_selectedKeyframes; };

private:
    void DrawContents() override;

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
