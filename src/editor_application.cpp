#include "common.h"
#include "editor_application.h"
#include "editor/editor_layer.h"
#include <canyon/platform/window.h>
#include <canyon/utils/vector_serialization.h>

char const* const EditorApplication::IMGUI_FILE = "imgui.ini";
char const* const EditorApplication::PERSISTENCE_FILE = "editor.json";
EditorApplication* g_App;

EditorApplication::EditorApplication(canyon::platform::IPlatform& platform)
    : Application(platform, "Moth UI Tool", 640, 480) {
    m_imguiSettingsPath = (std::filesystem::current_path() / IMGUI_FILE).string();
    m_persistentFilePath = std::filesystem::current_path() / PERSISTENCE_FILE;
    std::ifstream persistenceFile(m_persistentFilePath.string());
    if (persistenceFile.is_open()) {
        try {
            persistenceFile >> m_persistentState;
        } catch (std::exception&) {
        }

        if (!m_persistentState.is_null()) {
            auto const oldPos = m_mainWindowPosition;
            auto const oldWidth = m_mainWindowWidth;
            auto const oldHeight = m_mainWindowHeight;
            m_mainWindowPosition = m_persistentState.value("window_pos", m_mainWindowPosition);
            m_mainWindowWidth = m_persistentState.value("window_width", m_mainWindowWidth);
            m_mainWindowHeight = m_persistentState.value("window_height", m_mainWindowHeight);
            m_mainWindowMaximized = m_persistentState.value("window_maximized", m_mainWindowMaximized);
            if (m_mainWindowPosition.x <= 0 || m_mainWindowPosition.y <= 0) {
                m_mainWindowPosition = oldPos;
            }
            if (m_mainWindowWidth <= 0) {
                m_mainWindowWidth = oldWidth;
            }
            if (m_mainWindowHeight <= 0) {
                m_mainWindowHeight = oldHeight;
            }
        }
    }

    g_App = this;
}

EditorApplication::~EditorApplication() {
    std::ofstream ofile(m_persistentFilePath.string());
    if (ofile.is_open()) {
        m_persistentState["current_path"] = std::filesystem::current_path().string();
        m_persistentState["window_pos"] = m_mainWindowPosition;
        m_persistentState["window_width"] = m_mainWindowWidth;
        m_persistentState["window_height"] = m_mainWindowHeight;
        m_persistentState["window_maximized"] = m_mainWindowMaximized;
        ofile << m_persistentState;
    }

    g_App = nullptr;
}

void EditorApplication::PostCreateWindow() {
    if (m_persistentState.contains("current_path")) {
        std::string const currentPath = m_persistentState["current_path"];
        try {
            std::filesystem::current_path(currentPath);
        } catch (std::exception&) {
            // ...
        }
    }

    auto& layerStack = m_window->GetLayerStack();
    layerStack.SetEventListener(this);
    layerStack.PushLayer(std::make_unique<EditorLayer>(m_window->GetMothContext(), m_window->GetGraphics()));
}
