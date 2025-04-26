#include "example_application.h"
#include <canyon/platform/glfw/glfw_platform.h>

int main(int argc, char** argv) {
    auto platform = std::make_unique<canyon::platform::glfw::Platform>();
    platform->Startup();
    ExampleApplication application(*platform);
    application.Init();
    application.Run();
    platform->Shutdown();
}
