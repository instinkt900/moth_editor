# TODO

## 1.0.0

### Canvas rotation handle
Rotation can be set numerically in the properties panel but there is no drag handle on the bounds widget. For a visual editor this is a notable gap. A circular arc handle (or a handle above the selection box, like most design tools) that maps mouse drag angle to the entity's rotation property would be the expected interaction.

**Effort:** Medium. The bounds widget already has a well-defined handle system (`BoundsHandle` base class, anchor/offset handle subclasses). A rotation handle subclass needs to be added, along with the drag-to-angle maths and a corresponding undoable action.

---

### Animation event parameters
Animation events can be placed on the timeline and will fire as named callbacks at runtime, but there is no way to attach a data payload in the editor. The underlying `AnimationEvent` type in moth_ui needs to be checked for parameter support; if it exists, the editor just needs UI for it (an editable field in the event edit popup). If it doesn't, the type and serialisation need extending first.

**Effort:** Small–medium depending on whether moth_ui already supports event parameters. UI side is a small addition to the existing event modify dialog.

---

### Auto-save / crash recovery
The editor has no auto-save. Any unsaved work is lost if the application crashes or is force-closed. A simple time-based auto-save (e.g. every 2–5 minutes) writing to a temporary file alongside the open layout, with a recovery prompt on next launch if the temp file is newer than the saved file, would cover the most important case.

**Effort:** Medium. Needs a timer, a temp-file naming convention, a recovery prompt on startup, and cleanup of the temp file on clean save/close.

---

## 1.x

### Easing curve editor
Currently only discrete interpolation types are selectable per keyframe (e.g. linear, step). A bezier easing curve editor — inline in the animation panel or as a popup when editing a keyframe — would give much finer control over animation feel. This is a significant UI addition.

**Effort:** Large. Requires a bezier curve widget, mapping control point positions to interpolation parameters, and changes to how the animation track stores and evaluates keyframe easing.

---

### Align and distribute
No tools for aligning multiple selected nodes to each other (left edge, centre, right edge, top, bottom) or distributing them evenly. These are standard in any layout tool and are especially useful when setting up grids or lists of elements.

**Effort:** Small–medium. Pure logic operating on selected entities' bounds; each alignment is a small set of property changes wrapped in a composite action. The main work is the menu/toolbar UI and making sure multi-select is respected.

---

### Deep sublayout overrides
Sublayout refs currently support property overrides one level deep — direct children of a ref can have properties overridden per instance, but if those children are themselves refs, their overrides are not reachable from the parent. This is a moth_ui limitation: `propertyOverrides` addresses children by flat index, so there is no way to express a path into a nested ref.

Fixing this would require a path-based addressing scheme (e.g. `childIndex/childIndex/...`) in moth_ui's serialize/deserialize logic for `LayoutEntityRef`, plus corresponding editor UI to allow selecting and editing properties on deeply nested ref children. Most useful if building UI kits where refs are nested multiple levels deep.

**Effort:** Medium–large, split across moth_ui (core data model and serialisation) and moth_editor (UI for selecting and editing overrides on nested children).

---

### Asset panel image preview
Images in the asset list are shown only as filenames. A thumbnail preview (either as a tooltip on hover or as a small inline image) would make it much easier to find the right asset without memorising file names.

**Effort:** Small. Images are already loaded into the graphics backend; rendering a scaled version in a tooltip via `imgui_ext::Image` should be straightforward.
