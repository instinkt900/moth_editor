#pragma once

#include "moth/bridge/application.h"
#include <nlohmann/json.hpp>

class EditorApplication : public moth::bridge::Application
{
public:
    EditorApplication(moth::gfx::platform::IPlatform& platform);
    ~EditorApplication() override;


    nlohmann::json& GetPersistentState() { return m_persistentState; }

private:
    void PostCreateWindow() override;
    // Captures the window geometry while the window is still alive: Application::Run
    // destroys it after calling this, and the destructor writes the file.
    void Shutdown() override;

    // Must outlive ImGui context (destroyed in base class dtor after derived members).
    // Stored as a file-scope static in editor_application.cpp.
    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const IMGUI_FILE;
    static char const* const PERSISTENCE_FILE;
};
