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

#### #2 — Wrong frame calculated for keyframe right-click when scrolled ✓ FIXED
**Location:** `editor_panel_animation.cpp:879`

`trackOffset` is `-m_minFrame * m_framePixelWidth` — always negative when scrolled. The
correct inverse formula to recover a frame from mouse position is `+ -trackOffset`. Every
other usage (current frame click line 354, clips line 539, events line 670) uses
`+ -trackOffset` correctly. The keyframe right-click path used `+ trackOffset` (wrong sign),
causing the added keyframe to land at the wrong frame whenever the timeline was scrolled or
zoomed.

**Fix applied:** Changed line 879 to use `+ -rowDimensions.trackOffset` to match all other
call sites. Consider extracting a shared `MousePosToFrame()` helper to prevent future
divergence (see #9).

---

#### #3 — `FrameCount()` off-by-one
**Location:** `moth_ui/include/moth_ui/animation/animation_clip.h:21`

Clips are treated as inclusive on both ends — the panel renders them with `m_endFrame + 1`
(line 471). But `FrameCount()` returns `m_endFrame - m_startFrame`, which is one less than
the actual frame count. A clip spanning frames 10–15 displays as 6 frames wide but
`FrameCount()` returns 5. Any playback code consuming `FrameCount()` will stop one frame early.

**Fix:** Either change `FrameCount()` to return `m_endFrame - m_startFrame + 1`, or change the
clip semantics to be exclusive on the end (remove the `+ 1` in the panel render and audit all
uses). Pick one and apply consistently.

---

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

### MEDIUM

#### #5 — Clip drag handle bit encoding is accidentally correct but fragile
**Location:** `editor_panel_animation.cpp:486–489` (rect array) and `522` (handle assignment)

The three drag handle rects are indexed 0/1/2. Adding 1 gives `m_clipDragHandle` values of
1/2/3. These happen to work with the downstream `& 1` / `& 2` bitmask checks (left=1,
right=2, center=3), but only by coincidence of the arithmetic. If the array order or the
increment changes, the bitmask logic silently breaks.

**Fix:** Replace `j + 1` with explicit bitmask values, or define an enum:
```cpp
enum { kHandleLeft = 1, kHandleRight = 2, kHandleCenter = 3 };
// assign directly instead of using j + 1
```

---

#### #6 — Click consumption flag is inverted and order-dependent
**Location:** `editor_panel_animation.cpp:1140` and `1197`

`m_clickConsumed` is set to `true` when the mouse is *outside* the track area or the button
is not a left-click — the opposite of what the name implies. The flag is set and then
immediately tested in a `if (!m_clickConsumed)` a few lines later, making the logic
order-dependent and hard to follow.

**Fix:** Rename to something unambiguous (e.g. `m_clickWasInTrackArea`), store the positive
condition, and invert at the use sites. Consider replacing with a local variable scoped to the
input-handling block so the ordering dependency is obvious.

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

---

#### #8 — Inconsistent selection lookup patterns
**Location:** `editor_panel_animation.cpp:150–157` (clip lookup) vs `225–236` (keyframe lookup)

Clip selection uses a hand-written `for` loop; keyframe selection uses `ranges::find_if`.
Both do the same thing. The inconsistency adds cognitive overhead when reading or modifying
either.

**Fix:** Standardise on one pattern. `ranges::find_if` is more expressive; convert the clip
lookup to match.

---

#### #9 — Frame-from-mouse-position calculation duplicated 4+ times ✓ FIXED

Extracted `MousePosToFrame(float mouseX, float trackMinX)` to the header. All four call
sites (lines 354, 539, 670, 879) now use the helper.

---

#### #10 — Hardcoded colour values throughout
**Location:** `editor_panel_animation.cpp:257, 466, 600–601, 805–806, 839, 1021` and others

Colours are inline hex literals (`0xFF3A3636`, `0xFF00CCAA`, `0xFF13BDF3`, etc.) scattered
throughout the file. Changing the theme requires hunting down every occurrence.

**Fix:** Extract to named constants at the top of the file (or a shared theme header):
```cpp
static constexpr ImU32 kRowColorOdd    = 0xFF3A3636;
static constexpr ImU32 kRowColorEven   = 0xFF413D3D;
static constexpr ImU32 kKeyframeColor  = 0xFF13BDF3;
static constexpr ImU32 kKeyframeSelectedColor = 0xFF00CCAA;
// etc.
```
