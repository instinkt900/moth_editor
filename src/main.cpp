#include "common.h"
#include "backend/sdl/sdl_app.h"
#include "backend/vulkan/vulkan_app.h"

backend::IApplication* g_App = nullptr;

int main(int argc, char** argv) {
    //backend::sdl::Application application;
    backend::vulkan::Application application;
    g_App = &application;
    return application.Run();
}
