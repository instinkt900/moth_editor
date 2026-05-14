#pragma once

#include "moth_ui/animation/animation_clip.h"
#include "moth_ui/animation/animation_marker.h"

#include <variant>

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
}

using AnimationIntent = std::variant<
    anim_intent::ClickClip,
    anim_intent::ClickEvent,
    anim_intent::ConsumeClick,
    anim_intent::BeginClipDrag,
    anim_intent::BeginEventDrag,
    anim_intent::OpenClipPopup,
    anim_intent::OpenEventPopup
>;
