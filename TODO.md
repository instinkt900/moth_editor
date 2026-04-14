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

### Sprite Editor — moth_packer Integration

**Effort:** Medium

The sprite editor is complete. It loads a packed atlas and descriptor, supports defining named
clips as ordered frame sequences with per-frame pivot points, plays back animations in real time,
and saves the descriptor back to disk with full undo/redo.

The outstanding work is integration with moth_packer to drive the packing step from inside the
editor so that a pre-packed atlas is no longer a prerequisite. Two pieces are required:

**Editor canvas:** the sprite editor canvas currently renders a single atlas image. It needs to
accept multiple loose source images — imported via file dialog or drag-and-drop — and display
them on a working canvas before any packing has occurred. Each image on the canvas represents
one or more frames, and the user should be able to assign clip membership, frame order, and
pivot hints per image at this stage. This is the missing authoring step between "I have some
PNGs" and "I have a descriptor".

**moth_packer in-memory API:** once the canvas images and their metadata are ready, the editor
needs to call moth_packer without touching the filesystem — passing the in-memory pixel buffers
and accumulated metadata, and receiving a packed atlas buffer and typed descriptor in return.
moth_packer does not yet expose this interface (tracked in its own TODO). The repack flow is
also needed: when the user edits an already-packed sheet (adding or removing frames), the
existing atlas and descriptor should be fed back through the packer to produce a re-optimised
atlas with updated rects, preserving all clip and pivot data.

The command-line packer would remain available for build-pipeline use.

---

### Texture Packer — Interactive Canvas

**Effort:** Medium

The texture packer editor canvas currently shows the packed atlas as a static image. It should
receive the same interactive treatment the sprite editor canvas recently got: click and drag a
frame rect to move it, drag an edge or corner handle to resize it, with eight handle squares
drawn on the selected frame (four corners and four edge midpoints) and zoom-invariant hit radii.
Cursor should change on hover to the appropriate resize arrow. Changes should commit through the
existing undo stack.

---

### Asset Panel Image Preview

**Effort:** Small

Images in the asset list are shown only as filenames. A thumbnail preview — either as a tooltip
on hover or as a small inline image — would make it much easier to find the right asset without
memorising file names. Images are already loaded into the graphics backend; rendering a scaled
version in a tooltip via `imgui_ext::Image` should be straightforward.
