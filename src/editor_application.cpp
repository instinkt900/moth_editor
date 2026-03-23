#include "common.h"
#include "editor_application.h"
#include "editor/editor_layer.h"
#include <moth_graphics/platform/window.h>
#include <moth_graphics/graphics/surface_context.h>

char const* const EditorApplication::IMGUI_FILE = "imgui.ini";
char const* const EditorApplication::PERSISTENCE_FILE = "editor.json";

EditorApplication::EditorApplication(moth_graphics::platform::IPlatform& platform)
    : Application(platform, "Moth UI Tool", 1920, 1080) {
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
}

void EditorApplication::PostCreateWindow() {
    {
        constexpr int kSize = 64;
        constexpr int kTile = 8;
        std::vector<uint8_t> pixels(static_cast<std::size_t>(kSize * kSize * 4));
        for (int row = 0; row < kSize; ++row) {
            for (int col = 0; col < kSize; ++col) {
                bool const isLight = ((row / kTile) + (col / kTile)) % 2 == 0;
                std::size_t const idx = static_cast<std::size_t>((row * kSize + col) * 4);
                pixels[idx + 0] = isLight ? uint8_t{ 0xFF } : uint8_t{ 0x33 }; // R
                pixels[idx + 1] = isLight ? uint8_t{ 0x00 } : uint8_t{ 0x33 }; // G
                pixels[idx + 2] = isLight ? uint8_t{ 0xFF } : uint8_t{ 0x33 }; // B
                pixels[idx + 3] = uint8_t{ 0xFF };                              // A
            }
        }
        auto& assetContext = m_window->GetSurfaceContext().GetAssetContext();
        if (auto texture = assetContext.TextureFromPixels(kSize, kSize, pixels.data())) {
            texture->SetFilter(moth_graphics::graphics::TextureFilter::Nearest, moth_graphics::graphics::TextureFilter::Nearest);
            m_window->GetImageFactory().SetFallbackImage(std::shared_ptr<moth_graphics::graphics::ITexture>(std::move(texture)));
        }
    }

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
    layerStack.PushLayer(std::make_unique<EditorLayer>(m_window->GetMothContext(), m_window->GetGraphics(), this));
}
