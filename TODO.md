# TODO

## 1.x

### Animation Event Parameters

**Effort:** Small–Medium

Animation events can be placed on the timeline and fire as named callbacks at runtime, but there
is no way to attach a data payload in the editor. The `AnimationEvent` type in moth_ui needs to
be checked for parameter support first — if it exists, the editor just needs a small addition to
the event edit popup. If it doesn't, the type and serialisation need extending before the editor
work can begin.

---

### Easing Curve Editor

**Effort:** Large

Currently only discrete interpolation types are selectable per keyframe (e.g. linear, step). A
bezier easing curve editor — inline in the animation panel or as a popup when editing a keyframe
— would give much finer control over animation feel. Requires a bezier curve widget, mapping
control point positions to interpolation parameters, and changes to how animation tracks store
and evaluate keyframe easing.

---

### Font Previews

**Effort:** Small–Medium

Font names alone give no visual feedback. Two related improvements:

1. **Fonts dialog preview.** The Fonts dialog (`Edit > Fonts`) should display a rendered sample of
   the currently selected font (e.g. "AaBbCc 0123") below or beside the font list.

2. **Font picker on text nodes.** The font dropdown in the properties panel for text nodes shows
   only font names. Ideally each entry would render in its own typeface (requires custom
   `ImGui::Selectable` drawing with per-item font push/pop, which is non-trivial). As an
   alternative, a "Browse…" button could open a font picker popup that shows a rendered sample for
   each registered font, letting the user pick visually rather than by name.

---

### Animation Panel Selection / Context Menu Bugs

**Effort:** Small

Two related issues in the animation panel:

1. **Right-click on the clips row shows wrong context menu when a keyframe is selected.** If a
   keyframe is currently selected and the user right-clicks on the clips row (not on a keyframe),
   the selection is not cleared, so the context menu shows "Delete keyframe" instead of
   "Add clip". The keyframe selection should be cleared when clicking on the clips row itself.

2. **Left-clicking on clip or event rows does not clear keyframe selection.** A left-click on a
   clip row or event row should deselect any active keyframe, but the selection persists.

Both issues stem from the same root cause: clicks on non-keyframe rows in the timeline do not
clear the keyframe selection state.

---

### Multi-Selection Property Editing

**Effort:** Medium

When multiple nodes are selected, the properties panel only reflects and edits the single
(last-selected) node. Changes should apply to all selected nodes. This requires the properties
panel to either show a merged/aggregate view of the selection, or apply each property change as a
composite undo action across all selected nodes. Common behaviour is to show the value of the
focused node and apply any edits to the whole selection.

---

### Align and Distribute

**Effort:** Small–Medium

No tools for aligning multiple selected nodes to each other (left edge, center, right edge, top,
bottom) or distributing them evenly. These are standard in any layout tool and are especially
useful when setting up grids or lists of elements. Each alignment is a small set of property
changes wrapped in a composite undo action; the main work is the menu/toolbar UI and ensuring
multi-select is respected.

