from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.files import load

class MothUIEditor(ConanFile):
    name = "moth_ui_editor"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "MSBuildToolchain", "MSBuildDeps"
    exports_sources = "CMakeLists.txt", "version.txt", "src/*", "external/nativefiledialog/*", "external/stb/*"

    def set_version(self):
        self.version = load(self, "version.txt").strip()

    def requirements(self):
        self.requires("canyon/0.5.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.27.0]")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
