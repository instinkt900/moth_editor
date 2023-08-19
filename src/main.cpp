#include "common.h"
#include "sdl/sdl_application.h"
#include "vulkan/vulkan_application.h"

backend::IApplication* g_App = nullptr;

int main(int argc, char** argv) {
    //std::unique_ptr<backend::IApplication> application = backend::sdl::CreateApplication();
    std::unique_ptr<backend::IApplication> application = backend::vulkan::CreateApplication();
    g_App = application.get();
    return application->Run();
}
