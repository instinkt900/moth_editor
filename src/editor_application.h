#pragma once

#include "canyon/platform/application.h"
#include <nlohmann/json.hpp>

class EditorApplication : public canyon::platform::Application
{
public:
    EditorApplication(canyon::platform::IPlatform& platform);
    virtual ~EditorApplication();


    nlohmann::json& GetPersistentState() { return m_persistentState; }

private:
    void PostCreateWindow() override;

    std::filesystem::path m_imguiSettingsPath;
    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const IMGUI_FILE;
    static char const* const PERSISTENCE_FILE;
};

extern EditorApplication* g_App;
