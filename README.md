# Moth UI Editor
![Windows Editor Build Status](https://github.com/instinkt900/moth_ui/actions/workflows/cmake-build-editor-win.yml/badge.svg) 
![Linux Editor Build Status](https://github.com/instinkt900/moth_ui/actions/workflows/cmake-build-editor-linux.yml/badge.svg)  

A custom tool for building UI layouts with Moth UI. This edior provides a flash like interface that allows you to lay out and preview UI layouts and animations. Utilizing anchoring you can design interfaces for many screen sizes as well as allow animated behaviours using keyframes on the timeline.

![Screenshot 2023-10-14 141636](https://github.com/instinkt900/moth_ui/assets/35185578/a8779a2b-978e-450a-b80a-b0dad4f06306)

### Building

#### Installing conan
```windows
python -m venv .venv
.\.venv\Scripts\activate.bat
pip install conan
pip install setuptools
```

```linux
python3 -m venv .venv
source ./.venv/bin/activate
pip install conan
```

#### Conan

Building the editor from the command line is simplest using conan to install the dependencies and cmake to build.
```
conan install . --build=missing -s build_type=Release [--profile <profile>]
cmake --preset conan-default
cmake --build --preset conan-release
cmake --install build --config Release --prefix=<install_path>
```
This should install the editor in the given `install_path` folder.

### Using the editor
TODO
