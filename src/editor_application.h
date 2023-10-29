#pragma once

#include "vulkan/vulkan_app.h"
#include "sdl/sdl_app.h"
#include "events/event.h"

#define USE_VULKAN
//#define USE_SDL

class EditorApplication
#if defined(USE_SDL)
    : public backend::sdl::Application
#elif defined(USE_VULKAN)
    : public backend::vulkan::Application
#endif
{
public:
    EditorApplication();
    virtual ~EditorApplication();

    nlohmann::json& GetPersistentState() { return m_persistentState; }

private:
    void SetupLayers() override;

    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const IMGUI_FILE;
    static char const* const PERSISTENCE_FILE;
};

extern EditorApplication* g_App;