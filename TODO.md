# TODO

## Action System (`src/editor/actions/`)

---

### MEDIUM

#### #2 — Entity tree diverges from node tree after undoing a reorder
**Location:** `change_index_action.cpp:40`

In `ChangeIndexAction::Undo()`, the entity tree restoration reads
`parentEntityChildren[m_oldIndex]` to find the entity to re-insert — but after `Do()` that
entity is at `m_newIndex`, not `m_oldIndex`. The node tree is restored correctly (it holds
`m_node` directly), but the entity tree ends up with the entity at `m_oldIndex` duplicated
and the moved entity lost. This causes the entity tree and node tree to diverge silently,
leading to save corruption or rendering inconsistencies.

**STR:**
1. Create two or more nodes in a layout.
2. Reorder them via drag-and-drop in the layers panel (fires `ChangeIndexAction::Do`).
3. Undo the reorder (`Ctrl+Z`).
4. Save the layout and reopen — the node order will not match what is shown in the editor.

**Fix:** In `Undo()`, read from `m_newIndex` (where `Do()` placed the entity):
```cpp
auto oldEntity = parentEntityChildren[m_newIndex]; // was m_oldIndex
```

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

