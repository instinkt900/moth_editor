#pragma once

#include "moth_ui/animation/animation_clip.h"

#include <variant>

namespace anim_intent {
    // Selects a clip. If additive (Ctrl held), preserves existing selection;
    // otherwise clears prior selection first unless the clip was already
    // selected. Single intent so Apply owns the click-policy in one place.
    struct ClickClip { moth_ui::AnimationClip* clip; bool additive; };

    // Marks the current click as handled so the panel-level "unhandled click in
    // track area" path doesn't fall through to starting a box-select.
    struct ConsumeClick {};

    // Start dragging a clip. `handle` is a ClipDragHandle bitmask
    // (Left | Right | Center).
    struct BeginClipDrag { int handle; float mouseX; };

    // Open the clip right-click context menu, anchored at the given frame.
    struct OpenClipPopup { int atFrame; };
}

using AnimationIntent = std::variant<
    anim_intent::ClickClip,
    anim_intent::ConsumeClick,
    anim_intent::BeginClipDrag,
    anim_intent::OpenClipPopup
>;
