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
Like with the library build this should install the editor in the given `install_path` folder.

#### Visual Studio
First install the dependencies using conan.
```
conan install . --build=missing -s build_type=Debug [--profile <profile>]
```
Then open up the solution file in project/vs and build as normal.

#### VSCode
Using the CMake tools extension in VSCode makes this very similar to the Visual Studio procedure.
First install the dependencies using conan.
```
conan install . --build=missing -s build_type=Debug [--profile <profile>]
```
Then open up the workspace file in project/vscode. The CMake extension should pick up the project and allow you to configure/build. If you wish to debug you will have to setup your own launch configuration.

### Using the editor
TODO
