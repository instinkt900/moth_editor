# Moth UI Editor

[![Build Tests](https://github.com/instinkt900/moth_editor/actions/workflows/build-tests.yml/badge.svg)](https://github.com/instinkt900/moth_editor/actions/workflows/build-tests.yml)
[![Release](https://github.com/instinkt900/moth_editor/actions/workflows/release.yml/badge.svg)](https://github.com/instinkt900/moth_editor/actions/workflows/release.yml)

A visual layout and animation editor for [moth_ui](https://github.com/instinkt900/moth_ui). Provides a Flash-like authoring environment for building UI layouts and keyframe animations, targeting the moth_ui runtime.

![Editor Screenshot](https://github.com/instinkt900/moth_ui/assets/35185578/a8779a2b-978e-450a-b80a-b0dad4f06306)

---

## Features

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

---

## Requirements

- CMake 3.27+
- Conan 2.x
- Python 3 (for the Conan venv)
- A C++17 compiler (GCC 11+, Clang 14+, MSVC 2022)

**Linux system libraries** (must come from the system package manager, not Conan):
```bash
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libglfw3-dev
```

---

## Building

### Set up Conan (first time only)

**Windows:**
```bat
python -m venv .venv
.\.venv\Scripts\activate.bat
pip install conan setuptools
```

**Linux:**
```bash
python3 -m venv .venv
source ./.venv/bin/activate
pip install conan
```

### Build

```bash
conan install . --build=missing -s build_type=Release
cmake --preset conan-default
cmake --build --preset conan-release
```

### Install

```bash
cmake --install build --config Release --prefix=<install_path>
```

This copies the editor binary and required assets into `<install_path>`.

---

## Releases

Pre-built binaries for Windows and Linux are attached to each [GitHub Release](https://github.com/instinkt900/moth_editor/releases).
Releases are created automatically when `version.txt` is updated on master.

---

## Related Projects

| Project | Description |
|---|---|
| [moth_ui](https://github.com/instinkt900/moth_ui) | Core UI library — nodes, layout, animation, events |
| [canyon](https://github.com/instinkt900/canyon) | Graphics and application framework (SDL2 / Vulkan) |
