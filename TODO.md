# TODO

## Action System (`src/editor/actions/`)

---

### MEDIUM

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

