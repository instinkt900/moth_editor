#include "example_application.h"
#include <moth_graphics/platform/glfw/glfw_platform.h>

int main(int argc, char** argv) {
    auto platform = std::make_unique<moth_graphics::platform::glfw::Platform>();
    platform->Startup();
    ExampleApplication application(*platform);
    application.Init();
    application.Run();
    platform->Shutdown();
}
