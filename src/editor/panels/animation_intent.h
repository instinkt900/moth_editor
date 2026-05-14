#pragma once

#include "moth_ui/animation/animation_clip.h"
#include "moth_ui/animation/animation_marker.h"
#include "moth_ui/animation/animation_track.h"
#include "moth_ui/utils/interp.h"

#include <memory>
#include <string>
#include <variant>

namespace moth_ui {
    class Node;
    class LayoutEntity;
}

namespace anim_intent {
    // Selects a clip. If additive (Ctrl held), preserves existing selection;
    // otherwise clears prior selection first unless the clip was already
    // selected. Single intent so Apply owns the click-policy in one place.
    struct ClickClip { moth_ui::AnimationClip* clip; bool additive; };

    // Selects an event marker. If additive (Ctrl held), preserves existing
    // selection; otherwise clears prior selection unconditionally before
    // selecting (note: the policy differs from ClickClip).
    struct ClickEvent { moth_ui::AnimationMarker* event; bool additive; };

    // Marks the current click as handled so the panel-level "unhandled click in
    // track area" path doesn't fall through to starting a box-select.
    struct ConsumeClick {};

    // Start dragging a clip. `handle` is a ClipDragHandle bitmask
    // (Left | Right | Center).
    struct BeginClipDrag { int handle; float mouseX; };

    // Start dragging an event marker. Events are single-frame so there is no
    // resize handle.
    struct BeginEventDrag { float mouseX; };

    // Open the clip right-click context menu, anchored at the given frame.
    struct OpenClipPopup { int atFrame; };

    // Open the event right-click context menu, anchored at the given frame.
    struct OpenEventPopup { int atFrame; };

    // Adds a new clip starting at the given frame. Apply fills in the default
    // name/length so the intent stays a pure user-gesture record.
    struct AddClip { int atFrame; };

    // Adds a new empty-named event marker at the given frame.
    struct AddEvent { int atFrame; };

    // Deletes whatever is currently in the selection set (clips, events, or
    // keyframes — routed through EditorPanelAnimation::DeleteSelections).
    struct DeleteSelections {};

    // Commits an in-progress clip edit (from the popup's Edit menu) as a
    // ModifyClipAction. `reference` must still exist in m_clips when applied.
    struct CommitClipEdit { moth_ui::AnimationClip* reference; moth_ui::AnimationClip newValue; };

    // Commits an in-progress event edit as a ModifyEventAction.
    struct CommitEventEdit { moth_ui::AnimationMarker* reference; moth_ui::AnimationMarker newValue; };

    // Click on a child label in the track header column: toggles entity
    // selection (and clears prior selection when !additive). The label-drag
    // source bookkeeping (m_labelDragSourceIdx) stays direct view state.
    struct ClickChildLabel { std::shared_ptr<moth_ui::Node> child; bool additive; };

    // Select a continuous-track keyframe. The expanded flag selects between
    // ClearSelections (expanded) and FilterKeyframeSelections (collapsed) when
    // the keyframe wasn't already selected and !additive.
    struct ClickKeyframe { std::shared_ptr<moth_ui::LayoutEntity> entity; moth_ui::AnimationTrack::Target target; int frame; bool additive; bool expanded; };

    // Same as ClickKeyframe but for discrete-track keyframes.
    struct ClickDiscreteKeyframe { std::shared_ptr<moth_ui::LayoutEntity> entity; moth_ui::AnimationTrack::Target target; int frame; bool additive; bool expanded; };

    // Start dragging the current keyframe selection. altDrag seeds the
    // duplicate-on-drag flag with the alt-modifier state at gesture start.
    struct BeginKeyframeDrag { float mouseX; bool altDrag; };

    // Open the keyframe right-click context menu. isDiscrete picks the discrete
    // vs continuous branch inside the popup; target == Unknown means the click
    // hit a collapsed main row.
    struct OpenKeyframePopup { int childIndex; moth_ui::AnimationTrack::Target target; bool isDiscrete; int atFrame; };

    // Commit a child-label drag-to-reorder gesture as a ChangeIndexAction.
    struct CommitLabelReorder { std::shared_ptr<moth_ui::Node> node; int sourceIdx; int newIndex; };

    // Add a new discrete keyframe at the popup's frame. Apply re-reads the
    // track to seed the value from whatever was active there.
    struct AddDiscreteKeyframe { std::shared_ptr<moth_ui::LayoutEntity> entity; moth_ui::AnimationTrack::Target target; int frame; };

    // Add one or more continuous keyframes. target == Unknown means the click
    // was on a collapsed main row — Apply expands it to every continuous
    // target without an existing keyframe at this frame.
    struct AddContinuousKeyframes { std::shared_ptr<moth_ui::LayoutEntity> entity; moth_ui::AnimationTrack::Target target; int frame; };

    // Replace a discrete keyframe's value (uses AddDiscreteKeyframeAction —
    // the action overwrites when a keyframe already exists at the frame).
    struct SetDiscreteKeyframeValue { std::shared_ptr<moth_ui::LayoutEntity> entity; moth_ui::AnimationTrack::Target target; int frame; std::string value; };

    // Change the interpType of every continuous keyframe in the current
    // selection set, as one composite undoable action.
    struct CommitInterpChange { moth_ui::InterpType interpType; };
}

using AnimationIntent = std::variant<
    anim_intent::ClickClip,
    anim_intent::ClickEvent,
    anim_intent::ConsumeClick,
    anim_intent::BeginClipDrag,
    anim_intent::BeginEventDrag,
    anim_intent::OpenClipPopup,
    anim_intent::OpenEventPopup,
    anim_intent::AddClip,
    anim_intent::AddEvent,
    anim_intent::DeleteSelections,
    anim_intent::CommitClipEdit,
    anim_intent::CommitEventEdit,
    anim_intent::ClickChildLabel,
    anim_intent::ClickKeyframe,
    anim_intent::ClickDiscreteKeyframe,
    anim_intent::BeginKeyframeDrag,
    anim_intent::OpenKeyframePopup,
    anim_intent::CommitLabelReorder,
    anim_intent::AddDiscreteKeyframe,
    anim_intent::AddContinuousKeyframes,
    anim_intent::SetDiscreteKeyframeValue,
    anim_intent::CommitInterpChange
>;
