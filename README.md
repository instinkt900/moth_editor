# moth_editor

[![Build Tests](https://github.com/instinkt900/moth_editor/actions/workflows/build-tests.yml/badge.svg)](https://github.com/instinkt900/moth_editor/actions/workflows/build-tests.yml)
[![Release](https://github.com/instinkt900/moth_editor/actions/workflows/upload-package.yml/badge.svg)](https://github.com/instinkt900/moth_editor/actions/workflows/upload-package.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A visual layout and animation editor for [moth_ui](https://github.com/instinkt900/moth_ui). Provides a Flash-like authoring environment for building UI layouts and keyframe animations, targeting the moth_ui runtime.

![Editor Screenshot](https://github.com/instinkt900/moth_ui/assets/35185578/a8779a2b-978e-450a-b80a-b0dad4f06306)

---

## Table of Contents

- [Overview](#overview)
  - [AI Disclosure](#ai-disclosure)
- [Architecture](#architecture)
  - [Panels](#panels)
  - [Undo / Redo](#undo--redo)
  - [Persistent state](#persistent-state)
- [Dependencies](#dependencies)
- [Building](#building)
  - [Prerequisites](#prerequisites)
  - [Linux](#linux)
  - [Windows](#windows)
- [Installing / Publishing](#installing--publishing)
- [Related Projects](#related-projects)
- [License](#license)

---

## Overview

moth_editor provides:

- **Canvas editor** — drag, resize, and arrange UI nodes visually
- **Anchor system** — pin elements to parent edges or corners for multi-resolution layouts
- **Animation timeline** — add keyframes per property, define named clips, and set loop behaviour
- **Animation events** — place frame-triggered events on the timeline for runtime callbacks
- **Preview panel** — play back animations inside the editor at runtime speed
- **Properties panel** — inspect and edit node properties numerically
- **Layers panel** — manage node hierarchy, visibility, and lock state
- **Asset list** — browse and assign images and fonts from the project directory
- **Texture packer** — pack referenced images into atlases for optimised runtime loading
- **Undo / redo** — full action history with per-action inspection panel

### AI Disclosure

AI agents (primarily Claude) are used as tools in this project for tasks such as refactoring, documentation writing, and test implementation. The architecture, design decisions, and direction of the project are human-driven. This is not a vibe-coded project.

---

## Architecture

### Panels

The editor is composed of ImGui panels located in `src/editor/panels/`:

- `EditorPanelCanvas` — viewport for selecting, moving, and resizing nodes
- `EditorPanelAnimation` — keyframe timeline with clip and event management
- `EditorPanelPreview` — live animation playback at runtime speed
- `EditorPanelProperties` — numeric editing of the selected node's properties
- `EditorPanelElements` — node hierarchy with visibility and lock controls
- `EditorPanelAssetList` — image and font browser for the project directory
- `EditorPanelFonts` — font size management
- `EditorPanelConfig` — canvas and animation global settings
- `EditorPanelUndoStack` — per-action undo history inspector

### Undo / Redo

All mutations go through `IEditorAction` subclasses in `src/editor/actions/`. Each action implements `Do()` and `Undo()`. Related mutations within a single user gesture are aggregated into a `CompositeAction` so that a single undo step reverses the whole gesture.

### Persistent state

- `editor.json` — window geometry, panel layout, canvas colors, and animation settings
- `imgui.ini` — Dear ImGui docking and window positions

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| canyon ≥ 0.5.0 | Conan | Graphics and application framework |
| nativefiledialog | Conan | Native file picker dialogs |
| GTK3 | System (Linux) | Required by nativefiledialog on Linux |

---

## Building

### Prerequisites

Set up a Python virtual environment and install Conan:

```bash
# Linux / macOS
python3 -m venv .venv
source .venv/bin/activate
pip install conan

# Windows (PowerShell)
python3 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install conan
```

Conan profiles for both platforms are provided in `conan/profiles/`.

### Linux

GTK3 must come from the system package manager (required by nativefiledialog):

```bash
sudo apt install libgtk-3-dev
```

SDL2, GLFW, FreeType, and HarfBuzz are pulled in transitively via canyon and also require system packages — see the [canyon README](https://github.com/instinkt900/canyon#linux) for the full list.

```bash
conan install . --profile conan/profiles/linux_profile --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release
```

For a Debug build replace `Release` / `conan-release` with `Debug` / `conan-debug`.

### Windows

```bash
conan install . --profile conan/profiles/windows_profile --build=missing -s build_type=Release
cmake --preset conan-default
cmake --build --preset conan-release
```

---

## Installing / Publishing

To install the editor binary and assets locally:

```bash
cmake --install build --config Release --prefix=<install_path>
```

Pre-built binaries for Windows and Linux are attached to each [GitHub Release](https://github.com/instinkt900/moth_editor/releases). Releases are created automatically when `version.txt` is updated on master.

---

## Related Projects

| Project | Description |
|---|---|
| [moth_ui](https://github.com/instinkt900/moth_ui) | Core UI library — node graph, keyframe animation, and event system |
| [canyon](https://github.com/instinkt900/canyon) | Graphics and application framework (SDL2 / Vulkan) that moth_editor runs on |

---

## License

MIT
