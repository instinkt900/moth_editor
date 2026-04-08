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

### Sprite Editor

**Effort:** Large

A dedicated visual editor for defining sprite animation data on top of a packed atlas. Works in
conjunction with moth_packer rather than replacing it — moth_packer handles image packing and
produces the atlas, the editor handles the animation layer on top.

Typical workflows:
- Run moth_packer to produce a baseline atlas and descriptor, then open it in the editor to
  define clips and pivot points.
- Build animation data from scratch in the editor against an existing atlas.

Core functionality:
- Load a packed atlas and its descriptor as produced by moth_packer.
- Define named animation clips as explicit ordered lists of frame indices (not ranges), supporting
  repeated frames and frame sharing between clips.
- Set per-frame pivot points interactively — click to place the pivot on the frame image — so
  that variable-size frames align correctly at runtime.
- Preview animations playing back in real time.
- Save/export the animation descriptor for consumption by moth_ui at runtime.

Blocked on the moth_ui sprite animation redesign (see moth_ui TODO and moth_packer TODO) since
the runtime data model needs to exist before the editor can target it.

**Stretch goal:** Since moth_packer is a library, the editor could optionally drive the packing
step directly — import loose frame images, trigger a pack, and produce the atlas without leaving
the tool. The command-line packer would remain available for automated/build-pipeline use.

---

### Asset Panel Image Preview

**Effort:** Small

Images in the asset list are shown only as filenames. A thumbnail preview — either as a tooltip
on hover or as a small inline image — would make it much easier to find the right asset without
memorising file names. Images are already loaded into the graphics backend; rendering a scaled
version in a tooltip via `imgui_ext::Image` should be straightforward.
