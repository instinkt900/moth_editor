from conan import ConanFile

class Editor(ConanFile):
	name = "Editor"
	version = "1.0"
	settings = "os", "compiler", "build_type", "arch"
	generators = "CMakeToolchain", "CMakeDeps"

	def requirements(self):
		self.requires("sdl/2.26.5")
		self.requires("sdl_image/2.0.5")
		self.requires("sdl_ttf/2.20.2")
		self.requires("nlohmann_json/3.11.2")
		self.requires("magic_enum/0.7.3")
		self.requires("range-v3/0.12.0")
		self.requires("fmt/10.1.1")
		self.requires("vulkan-headers/1.3.243.0")
		self.requires("vulkan-loader/1.3.243.0")
		self.requires("spdlog/1.12.0")
		self.requires("glfw/3.3.8")
		self.requires("vulkan-memory-allocator/3.0.1")
		self.requires("freetype/2.12.1")
		self.requires("glm/cci.20230113")
		self.requires("libwebp/1.3.2", override=True)
