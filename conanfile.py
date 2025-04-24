from conan import ConanFile
from conan.tools.cmake import cmake_layout

class MothUIEditor(ConanFile):
    name = "moth_ui_editor"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "MSBuildToolchain", "MSBuildDeps"

    def configure(self):
        if self.settings.os == "Linux":
            self.options["libpng"].shared = True

    def requirements(self):
        self.requires("canyon/0.2.0")

        if self.settings.os == "Windows":
            self.requires("libgettext/0.21", override=True)
            self.requires("harfbuzz/8.3.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.27.0]")

    def layout(self):
        cmake_layout(self)

