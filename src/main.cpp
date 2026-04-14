#include "common.h"
#include "editor_application.h"
#include <moth_graphics/platform/glfw/glfw_platform.h>
#include <moth_graphics/platform/sdl/sdl_platform.h>

int main(int argc, char** argv) {
    auto platform = std::make_unique<moth_graphics::platform::glfw::Platform>();
    // auto platform = std::make_unique<moth_graphics::platform::sdl::Platform>();
    platform->Startup();
    EditorApplication app(*platform);
    app.Init();
    app.Run();
    platform->Shutdown();
}
