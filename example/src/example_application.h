#pragma once

#include "canyon/platform/application.h"
#include <nlohmann/json.hpp>

class ExampleApplication : public canyon::platform::Application
{
public:
    explicit ExampleApplication(canyon::platform::IPlatform& platform);
    virtual ~ExampleApplication();

    nlohmann::json& GetPersistentState() { return m_persistentState; }

private:
    void PostCreateWindow() override;

    std::filesystem::path m_imguiSettingsPath;
    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    canyon::IntVec2 m_windowPos;
    int m_windowWidth;
    int m_windowHeight;
    bool m_windowMaximized;

    static char const* const IMGUI_FILE;
    static char const* const PERSISTENCE_FILE;
};

extern ExampleApplication* g_App;
