#include "common.h"
#include "editor_application.h"
#include <canyon/platform/glfw/glfw_platform.h>
#include <canyon/platform/sdl/sdl_platform.h>

int main(int argc, char** argv) {
    auto platform = std::make_unique<canyon::platform::glfw::Platform>();
    // auto platform = std::make_unique<canyon::platform::sdl::Platform>();
    platform->Startup();
    EditorApplication app(*platform);
    app.Init();
    app.Run();
    platform->Shutdown();
}
