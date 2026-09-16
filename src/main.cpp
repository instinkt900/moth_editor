#include "common.h"
#include "editor_application.h"
#include <moth/graphics/platform/iplatform.h>
#include <moth/graphics/platform/glfw/glfw_platform.h>

int main(int argc, char** argv) {
    bool enableViewports = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view const arg(argv[i]);
        if (arg == "--viewports") {
            enableViewports = true;
        } else if (arg == "--vulkan") {
            // The toolkit dropped the SDL2 backend, so GLFW + Vulkan is the only
            // renderer. Accepted and ignored so existing shortcuts keep working.
            spdlog::warn("--vulkan is now the default and only backend; the flag has no effect");
        }
    }

    auto platform = std::make_unique<moth::gfx::platform::glfw::Platform>();
    platform->Startup();
    EditorApplication app(*platform);
    app.SetImGuiViewportsEnabled(enableViewports);
    app.Init();
    app.Run();
    platform->Shutdown();
}
