#include "common.h"
#include "editor_application.h"
#include <moth_graphics/platform/iplatform.h>
#include <moth_graphics/platform/glfw/glfw_platform.h>
#include <moth_graphics/platform/sdl/sdl_platform.h>

int main(int argc, char** argv) {
    bool const useVulkan = [&]() {
        for (int i = 1; i < argc; ++i) {
            if (std::string_view(argv[i]) == "--vulkan") {
                return true;
            }
        }
        return false;
    }();

    std::unique_ptr<moth_graphics::platform::IPlatform> platform;
    if (useVulkan) {
        platform = std::make_unique<moth_graphics::platform::glfw::Platform>();
    } else {
        platform = std::make_unique<moth_graphics::platform::sdl::Platform>();
    }
    platform->Startup();
    EditorApplication app(*platform);
    app.Init();
    app.Run();
    platform->Shutdown();
}
