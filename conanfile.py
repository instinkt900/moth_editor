from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake
from conan.tools.files import load
from conan.tools.system.package_manager import Apt

class MothUIEditor(ConanFile):
    name = "moth_editor"

    license = "MIT"
    url = "https://github.com/instinkt900/moth_editor"
    description = "A visual layout and animation editor for the moth_toolkit UI module"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "MSBuildToolchain", "MSBuildDeps"
    exports_sources = "CMakeLists.txt", "version.txt", "src/*", "external/nativefiledialog/*"

    def set_version(self):
        if not self.version:
            self.version = load(self, "version.txt").strip()

    def requirements(self):
        # The toolkit modules the editor includes directly. moth_bridge pulls
        # core/gfx/ui with transitive_headers, but they are named here too because
        # the editor includes their headers rather than reaching them by accident.
        #
        # spdlog is no longer declared here. It used to be listed first to pin fmt
        # before moth_ui's range floated above it (see instinkt900/camina#392), but
        # moth_core now requires fmt/[~10.2] and spdlog/[~1.14] together, so the
        # conflict is resolved inside the module and an editor-side pin would only
        # fight it. spdlog reaches this build through moth_core.
        self.requires("moth_core/[>=0.1 <1]")
        self.requires("moth_ui/[>=2 <3]")
        self.requires("moth_graphics/[>=2 <3]")
        self.requires("moth_bridge/[>=0.1 <1]")
        self.requires("moth_packer/[>=2 <3]")

    def configure(self):
        # The texture packer panel collects images by walking moth::ui layout files,
        # and those collectors are compiled out unless moth_packer is built with UI
        # support (they are gated on MOTH_PACKER_HAS_UI in both the header and the
        # library). The option defaults to False, so it has to be asked for here.
        self.options["moth_packer"].with_ui = True

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
