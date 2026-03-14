# TODO

## Animation Panel (`src/editor/panels/editor_panel_animation.cpp`)

---

### CRITICAL

#### #1 — Null pointer dereference in box selection
**Location:** `editor_panel_animation.cpp:834`

`KeyframeContext` is pushed to `m_pendingBoxSelections` with only three fields initialised.
The fourth field `current` is left as `nullptr`. When pending selections are committed,
code at line 1268 dereferences `keyframeContext->current->m_frame`, causing a crash after
any box-select followed by a drag.

**Fix:** Pass `keyframe` as the fourth argument:
```cpp
m_pendingBoxSelections.push_back(KeyframeContext{ childEntity, target, keyframe->m_frame, keyframe });
```

---

### HIGH

#### #4 — Use-after-free risk with pending popup edits
**Location:** `editor_panel_animation.cpp:432–439` (clips) and `584–591` (events)

`m_pendingClipEdit` and `m_pendingEventEdit` hold raw pointers to clips/events and are
committed the frame *after* the popup closes (the commit block sits outside the
`ImGui::BeginPopup` guard). If the underlying clip or event is deleted while the popup is open
(e.g. via undo or another panel), the pointer is dangling when it is committed next frame.

**Fix:** Either commit/discard inside the popup block before it closes, or add a validity check
before dereferencing — confirm the pointer still exists in the entity's track/event list before
committing the edit.

---

### MINOR / MAINTAINABILITY

#### #7 — Box selection threshold too small
**Location:** `editor_panel_animation.cpp:1164`

```cpp
if (m_selectBox.GetArea() > 30) {
    m_boxSelecting = true;
}
```
A 30-pixel area (e.g. 6×5 pixels) triggers on very short drags, causing users to accidentally
enter box-select mode when they meant to click a keyframe.

**Fix:** Raise the threshold (e.g. 100–200 pixels area, or switch to a minimum side-length
check like `width > 5 && height > 5`) to match typical drag-detection heuristics.
