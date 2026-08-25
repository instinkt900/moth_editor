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
        if not self.version:
            self.version = load(self, "version.txt").strip()

    def requirements(self):
        # spdlog first, and declared here rather than taken through
        # moth_graphics. It pins one fmt exactly, and moth_ui asks for a range
        # that floats above that pin. Whichever of the two Conan resolves first
        # wins, so a graph that meets moth_ui first picks the newest fmt and
        # then conflicts with spdlog. Naming spdlog here puts its exact pin in
        # the graph before any range is resolved against it. The Camina engine
        # resolves for the same reason. See instinkt900/camina#392.
        self.requires("spdlog/[~1.17]")
        self.requires("moth_ui/[>=1.8 <2]")
        self.requires("moth_graphics/[>=1.3 <2]")
        self.requires("moth_packer/[>=1 <2]")

    def system_requirements(self):
        if self.settings.os == "Linux":
            apt = Apt(self)
            apt.install(["libgtk-3-dev"])

    def build_requirements(self):
        # A range, not an exact version. GitHub moved windows-latest to a
        # VS 2026 image, Conan then asks for the "Visual Studio 18 2026"
        # generator, and CMake 3.27 has no such generator: its list stops
        # at Visual Studio 17 2022. An exact pin here means a new runner
        # image breaks the build with nothing in this repository having
        # changed. See instinkt900/moth_packer, the 2026-08-24 upload run.
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
