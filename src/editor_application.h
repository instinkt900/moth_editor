#pragma once

#include "moth_graphics/platform/application.h"
#include <nlohmann/json.hpp>

class EditorApplication : public moth_graphics::platform::Application
{
public:
    EditorApplication(moth_graphics::platform::IPlatform& platform);
    ~EditorApplication() override;


    nlohmann::json& GetPersistentState() { return m_persistentState; }

private:
    void PostCreateWindow() override;

    std::filesystem::path m_imguiSettingsPath;
    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const IMGUI_FILE;
    static char const* const PERSISTENCE_FILE;
};
