# TODO

## 1.0.0

### Animation Event Parameters

**Effort:** Small–Medium

Animation events can be placed on the timeline and fire as named callbacks at runtime, but there
is no way to attach a data payload in the editor. The `AnimationEvent` type in moth_ui needs to
be checked for parameter support first — if it exists, the editor just needs a small addition to
the event edit popup. If it doesn't, the type and serialisation need extending before the editor
work can begin.

---

## 1.x

### Easing Curve Editor

**Effort:** Large

Currently only discrete interpolation types are selectable per keyframe (e.g. linear, step). A
bezier easing curve editor — inline in the animation panel or as a popup when editing a keyframe
— would give much finer control over animation feel. Requires a bezier curve widget, mapping
control point positions to interpolation parameters, and changes to how animation tracks store
and evaluate keyframe easing.

---

### Align and Distribute

**Effort:** Small–Medium

No tools for aligning multiple selected nodes to each other (left edge, center, right edge, top,
bottom) or distributing them evenly. These are standard in any layout tool and are especially
useful when setting up grids or lists of elements. Each alignment is a small set of property
changes wrapped in a composite undo action; the main work is the menu/toolbar UI and ensuring
multi-select is respected.

---


### Asset Panel Image Preview

**Effort:** Small

Images in the asset list are shown only as filenames. A thumbnail preview — either as a tooltip
on hover or as a small inline image — would make it much easier to find the right asset without
memorising file names. Images are already loaded into the graphics backend; rendering a scaled
version in a tooltip via `imgui_ext::Image` should be straightforward.
