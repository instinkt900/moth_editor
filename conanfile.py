from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.files import load
from conan.tools.system.package_manager import Apt

class MothUIEditor(ConanFile):
    name = "moth_editor"

    license = "MIT"
    url = "https://github.com/instinkt900/moth_editor"
    description = "A visual layout and animation editor for moth_ui"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "MSBuildToolchain", "MSBuildDeps"
    exports_sources = "CMakeLists.txt", "version.txt", "src/*", "external/nativefiledialog/*"

    def set_version(self):
        self.version = load(self, "version.txt").strip()

    def requirements(self):
        self.requires("moth_graphics/0.7.0")
        self.requires("moth_packer/0.2.0")

    def system_requirements(self):
        if self.settings.os == "Linux":
            apt = Apt(self)
            apt.install(["libgtk-3-dev"])

    def build_requirements(self):
        self.tool_requires("cmake/3.27.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
