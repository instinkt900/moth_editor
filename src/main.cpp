#include "common.h"
#include "backends/sdl/sdl_app.h"

IApp* g_App = nullptr;

int main(int argc, char** argv) {
    SDLApp application;
    g_App = &application;
    return application.Run();
}
