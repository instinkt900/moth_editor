#pragma once

#include "editor_panel.h"
#include "moth_ui/animation_track.h"
#include "moth_ui/animation_clip.h"
#include "../imgui_ext_focus.h"
#include "../keyframe_context.h"
#include "imgui_internal.h"

// highly adapted from ImSequencer
// https://github.com/CedricGuillemet/ImGuizmo

struct ClipContext {
    moth_ui::AnimationClip* clip = nullptr;
    moth_ui::AnimationClip mutableValue;
};

struct EventContext {
    moth_ui::AnimationEvent* event = nullptr;
    moth_ui::AnimationEvent mutableValue;
};

struct KeyframeContext {
    std::shared_ptr<moth_ui::LayoutEntity> entity;
    moth_ui::AnimationTrack::Target target = moth_ui::AnimationTrack::Target::Unknown;
    int mutableFrame = -1;
    moth_ui::Keyframe* current = nullptr;
};

class EditorPanelAnimation : public EditorPanel {
public:
    EditorPanelAnimation(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelAnimation() = default;

    void OnLayoutLoaded() override;

    //std::vector<KeyframeContext>& GetSelectedKeyframes() { return m_selectedKeyframes; };

private:
    void DrawContents() override;

    std::shared_ptr<moth_ui::Group> m_group = nullptr;

    struct RowOptions {
        bool expandable = false;
        bool expanded = false;
        bool indented = false;
        std::optional<ImU32> colorOverride;

        RowOptions& Expandable(bool exp) {
            expandable = exp;
            return *this;
        }

        RowOptions& Expanded(bool exp) {
            expanded = exp;
            return *this;
        }

        RowOptions& Indented(bool indent) {
            indented = indent;
            return *this;
        }

        RowOptions& ColorOverride(ImU32 const& col) {
            colorOverride = col;
            return *this;
        }
    };

    struct RowDimensions {
        ImRect rowBounds;
        ImRect buttonBounds;
        ImRect labelBounds;
        ImRect trackBounds;
        float trackOffset;
    };

    using ElementContext = std::variant<ClipContext, EventContext, KeyframeContext>;
    std::vector<ElementContext> m_selections;

    void ClearSelections();
    void DeleteSelections();

    void SelectClip(moth_ui::AnimationClip* clip);
    void DeselectClip(moth_ui::AnimationClip* clip);
    bool IsClipSelected(moth_ui::AnimationClip* clip);
    ClipContext* GetSelectedClipContext(moth_ui::AnimationClip* clip);

    void SelectEvent(moth_ui::AnimationEvent* event);
    void DeselectEvent(moth_ui::AnimationEvent* event);
    bool IsEventSelected(moth_ui::AnimationEvent* event);
    EventContext* GetSelectedEventContext(moth_ui::AnimationEvent* event);

    void SelectKeyframe(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    void DeselectKeyframe(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    bool IsKeyframeSelected(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    KeyframeContext* GetSelectedKeyframeContext(std::shared_ptr<moth_ui::LayoutEntity> entity, moth_ui::AnimationTrack::Target target, int frameNo);
    void FilterKeyframeSelections(std::shared_ptr<moth_ui::LayoutEntity> entity, int frameNo);

    void UpdateMouseDragging();

    bool m_mouseDragging = false;
    float m_mouseDragStartX = 0.0f;
    int m_clipDragSection = -1;
    int m_clickedFrame = -1;
    bool m_pendingClearSelection = false;

    template <typename T>
    struct EditContext {
        EditContext(T* value)
            : mutableValue(*value)
            , reference(value) {}
        bool HasChanged() const { return mutableValue != *reference; }
        T mutableValue;
        T* reference;
    };

    std::optional<EditContext<moth_ui::AnimationClip>> m_pendingClipEdit;
    std::optional<EditContext<moth_ui::AnimationEvent>> m_pendingEventEdit;

    bool DrawClipPopup();
    bool DrawEventPopup();
    bool DrawKeyframePopup();

    RowDimensions AddRow(char const* label, RowOptions const& rowOptions);
    void DrawFrameRow();
    void DrawClipRow();
    void DrawEventsRow();
    void DrawChildTrack(int childIndex, std::shared_ptr<moth_ui::Node> child);
    void DrawTrackRows();
    void DrawHorizScrollBar();
    void DrawCursor();
    int CalcNumRows() const;

    void DrawWidget();
    char const* GetChildLabel(int index) const;
    char const* GetTrackLabel(moth_ui::AnimationTrack::Target target) const;

    int m_minFrame = 0;
    int m_maxFrame = 100;
    int m_currentFrame = 0;
    float m_firstVisibleFrame = 0.0f;

    float m_framePixelWidth = 10.f;

    float const m_rowHeight = 20;
    float const m_legendWidth = 200;

    bool m_movingScrollBar = false;

    ImVec2 m_panningViewSource;
    float m_panningViewFrame;
    bool m_movingCurrentFrame = false;
    int m_movingEntry = -1;

    bool m_hScrollGrabbedRight = false;
    bool m_hScrollGrabbedLeft = false;
    float const m_hScrollMinSize = 44.0f;




    int m_clickedChildIdx = -1;
    moth_ui::AnimationTrack::Target m_clickedChildTarget = moth_ui::AnimationTrack::Target::Unknown;

    bool IsAnyPopupOpen() const;

    static inline char const* const KeyframePopupName = "keyframe_popup";
    static inline char const* const ClipPopupName = "clip_popup";
    static inline char const* const EventPopupName = "event_popup";

    std::vector<bool> m_childExpanded;

    struct TrackExtraData {
        std::weak_ptr<moth_ui::Node> ptr;
        bool expanded;
    };
    std::map<void*, TrackExtraData> m_extraData;

    bool IsExpanded(std::shared_ptr<moth_ui::Node> child) const;
    void SetExpanded(std::shared_ptr<moth_ui::Node> child, bool expanded);
    void SanitizeExtraData();

    ImDrawList* m_drawList = nullptr;
    ImRect m_windowBounds;
    ImRect m_panelBounds;
    int m_rowCounter = 0;
};
