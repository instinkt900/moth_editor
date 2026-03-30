# Moth Editor

[![Build Tests](https://github.com/instinkt900/moth_editor/actions/workflows/build-test.yml/badge.svg)](https://github.com/instinkt900/moth_editor/actions/workflows/build-test.yml)
[![Release](https://github.com/instinkt900/moth_editor/actions/workflows/upload-release.yml/badge.svg)](https://github.com/instinkt900/moth_editor/actions/workflows/upload-release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A visual layout and animation editor for [moth_ui](https://github.com/instinkt900/moth_ui). Design UI layouts and keyframe animations in a Flash-like authoring environment, then load them directly into your moth_ui application at runtime.

![Editor Screenshot](https://github.com/instinkt900/moth_ui/assets/35185578/a8779a2b-978e-450a-b80a-b0dad4f06306)

---

## Table of Contents

- [Features](#features)
  - [AI Disclosure](#ai-disclosure)
- [Building](#building)
  - [Prerequisites](#prerequisites)
  - [Linux](#linux)
  - [Windows](#windows)
- [Related Projects](#related-projects)
- [License](#license)

---

## Features

**Layout editing:** place and arrange UI elements on a canvas with mouse-driven move and resize controls. Elements can be anchored to parent edges or corners so layouts adapt cleanly across different resolutions without manual adjustment.

**Keyframe animation:** animate any property over time using a timeline editor. Keyframes can be added, moved, and deleted per property, and multiple named clips can be defined within a single layout for things like idle, hover, and transition states.

**Animation events:** attach named events to specific frames on the timeline. These fire as callbacks in the running application, making it easy to synchronise sound, logic, or state changes to an animation without hard-coding timings.

**Live preview:** play back animations inside the editor at runtime speed to see exactly how they will look in the application before exporting.

**Texture packing:** pack all images referenced by a layout into texture atlases in one step, reducing draw calls and load times in the runtime application.

**Autosave:** automatically save the current layout on a configurable interval. Keeps a configurable number of versioned copies and only runs when there are unsaved changes.

**Crash recovery:** a recovery snapshot is written on every edit when there are unsaved changes. If the editor exits unexpectedly, the snapshot is detected on the next launch and you are prompted to restore or discard it.

### AI Disclosure

AI agents (primarily Claude) are used as tools in this project for tasks such as refactoring, documentation writing, and test implementation. The architecture, design decisions, and direction of the project are human-driven. This is not a vibe-coded project.

---

## Building

Pre-built binaries for Windows and Linux are attached to each [GitHub Release](https://github.com/instinkt900/moth_editor/releases) if you'd rather not build from source.

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

**C++17 is required.** A `.conan/profile` is provided that sets `compiler.cppstd=17` and configures Conan to install system packages automatically (`tools.system.package_manager:mode=install`). This profile is used in CI and can be used directly or as a reference when building locally.

### Linux

Several system packages are required on Linux. GTK3 is needed by nativefiledialog; SDL2, GLFW, FreeType, and HarfBuzz are pulled in transitively via moth_graphics (see the [moth_graphics README](https://github.com/instinkt900/moth_graphics#linux) for background on why these must come from the system).

Using `.conan/profile`, Conan will install these automatically via `apt`:

```bash
conan install . -pr .conan/profile -s build_type=Release --build=missing
cmake --preset conan-release
cmake --build --preset conan-release
```

If you'd rather install them yourself first:

```bash
sudo apt install libgtk-3-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libglfw3-dev libfreetype-dev libharfbuzz-dev
```

### Windows

```bash
conan install . -pr .conan/profile -s build_type=Release --build=missing
cmake --preset conan-default
cmake --build --preset conan-release
```

---

## Related Projects

| Project | Description |
|---|---|
| [moth_ui](https://github.com/instinkt900/moth_ui) | Core UI library: node graph, keyframe animation, and event system |
| [moth_graphics](https://github.com/instinkt900/moth_graphics) | Graphics and application framework built on moth_ui: SDL2 and Vulkan backends, window management, and a layer stack |
| moth_editor | *(this project)* Visual layout and animation editor for creating moth_ui layout files |
| [moth_packer](https://github.com/instinkt900/moth_packer) | Command-line texture atlas packer for images and moth_ui layouts |

---

## License

MIT
