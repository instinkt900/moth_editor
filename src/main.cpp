#include "common.h"
#include "sdl_app.h"
#include "vulkan_app.h"

backend::IApplication* g_App = nullptr;

int main(int argc, char** argv) {
    //backend::sdl::Application application;
    backend::vulkan::Application application;
    g_App = &application;
    return application.Run();
}
