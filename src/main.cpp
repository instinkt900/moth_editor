#include "common.h"
#include "backends/sdl/sdl_app.h"
#include "backends/vulkan/vulkan_app.h"

IApp* g_App = nullptr;

int main(int argc, char** argv) {
    //SDLApp application;
    VulkanApp application;
    g_App = &application;
    return application.Run();
}
