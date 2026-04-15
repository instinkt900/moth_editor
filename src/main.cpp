#include "common.h"
#include "editor_application.h"
#include <moth_graphics/platform/iplatform.h>
#include <moth_graphics/platform/glfw/glfw_platform.h>
#include <moth_graphics/platform/sdl/sdl_platform.h>

int main(int argc, char** argv) {
    bool useVulkan = false;
    bool enableViewports = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view const arg(argv[i]);
        if (arg == "--vulkan") { useVulkan = true; }
        else if (arg == "--viewports") { enableViewports = true; }
    }

    if (enableViewports && !useVulkan) {
        spdlog::warn("--viewports has no effect without --vulkan (SDL2 renderer does not support multi-viewports)");
        enableViewports = false;
    }

    std::unique_ptr<moth_graphics::platform::IPlatform> platform;
    if (useVulkan) {
        platform = std::make_unique<moth_graphics::platform::glfw::Platform>();
    } else {
        platform = std::make_unique<moth_graphics::platform::sdl::Platform>();
    }
    platform->Startup();
    EditorApplication app(*platform);
    app.SetImGuiViewportsEnabled(enableViewports);
    app.Init();
    app.Run();
    platform->Shutdown();
}
