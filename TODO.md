# TODO

## Action System (`src/editor/actions/`)

---

### MEDIUM

#### #3 — Null dereference in BoundsWidget::OnMouseUp when selection is cleared mid-drag
**Location:** `bounds_widget.cpp:125,142`

`OnMouseDown` sets `m_anchorTLPressed` / `m_anchorFillPressed` without checking `m_node`.
`OnMouseUp` checks `IsInRect(...) && m_anchorTLPressed` before calling
`m_node->GetLayoutRect()` and `m_node->GetParent()->GetScreenRect()` — but if
`SetSelection(nullptr)` was called between the down and up events (e.g. the canvas
deselected on a different code path), `m_node` will be null and the dereference crashes.

**STR:**
1. Click one of the anchor preset buttons in the bounds widget.
2. While holding the mouse button, trigger a selection clear (e.g. click on empty canvas
   area through keyboard shortcut or programmatic deselect).
3. Release the mouse — crash.

**Fix:** Add an early return at the top of `OnMouseUp` (and `OnMouseDown`):
```cpp
if (!m_node) return false;
```

#### #4 — ChangeValueAction holds a raw reference — dangling if owner is destroyed
**Location:** `editor_action.h:49`

`ChangeValueAction<T>` stores `T& m_valueRef` — a raw reference to the field being
animated. If the object that owns that field (a `LayoutEntity`, `Node`, etc.) is destroyed
while the action still lives on the undo stack, `Do()`/`Undo()` will write through a
dangling reference, causing UB or a silent corrupt write.

This typically only matters when the undo stack is *not* cleared on selection/layout
change; if `ClearEditActions()` is always called before the owner can be destroyed the
risk is lower, but it is an implicit coupling that could break silently.

**STR:**
1. Modify a property on a node (pushes a `ChangeValueAction`).
2. Delete the node without triggering an undo-stack clear.
3. Undo — writes through a stale reference.

**Fix:** Replace `T& m_valueRef` with a `std::weak_ptr` or a stable-address wrapper, or
ensure `ClearEditActions()` is always called whenever any owned entity is destroyed.

