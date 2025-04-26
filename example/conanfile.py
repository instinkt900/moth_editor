from conan import ConanFile
from conan.tools.cmake import cmake_layout

class MothUIExample(ConanFile):
    name = "moth_ui_example"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "MSBuildToolchain", "MSBuildDeps"

    def requirements(self):
        self.requires("canyon/0.2.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.27.0]")

    def layout(self):
        cmake_layout(self)

