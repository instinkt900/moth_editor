#include "common.h"
#include "editor_application.h"
#include "editor/editor_layer.h"
#include <moth/graphics/platform/window.h>
#include <moth/graphics/platform/glfw/glfw_window.h>
#include <moth/graphics/graphics/surface_context.h>

namespace {
    char const* const kPersistenceFile = "editor.json";
}

char const* const EditorApplication::IMGUI_FILE = "imgui.ini";
char const* const EditorApplication::PERSISTENCE_FILE = kPersistenceFile;

// Must outlive ImGui context destruction, which happens in Application::~Application()
// (base class dtor) after EditorApplication's members are already destroyed.
static std::string s_imguiSettingsPath;

namespace {
    int constexpr kDefaultWidth = 1920;
    int constexpr kDefaultHeight = 1080;

    // Application takes the window size as a constructor argument, which runs before
    // the constructor body could read the file, so the state is loaded on first use.
    nlohmann::json const& PersistedState() {
        static nlohmann::json const state = [] {
            nlohmann::json loaded;
            std::ifstream file((std::filesystem::current_path() / kPersistenceFile).string());
            if (file.is_open()) {
                try {
                    file >> loaded;
                } catch (std::exception&) {
                }
            }
            return loaded.is_object() ? loaded : nlohmann::json::object();
        }();
        return state;
    }

    int PersistedDimension(char const* key, int fallback) {
        int const value = PersistedState().value(key, fallback);
        return value > 0 ? value : fallback;
    }
}

EditorApplication::EditorApplication(moth::gfx::platform::IPlatform& platform)
    : Application(platform, "Moth UI Tool",
                  PersistedDimension("window_width", kDefaultWidth),
                  PersistedDimension("window_height", kDefaultHeight))
    , m_persistentState(PersistedState()) {
    s_imguiSettingsPath = std::filesystem::absolute(std::filesystem::current_path() / IMGUI_FILE).string();
    m_persistentFilePath = std::filesystem::current_path() / PERSISTENCE_FILE;
}

void EditorApplication::Shutdown() {
    // The ImGui context goes with the window that Application destroys after this.
    imgui_ext::SetImGuiContext(nullptr);

    if (auto const* const uiWindow = GetUiWindow()) {
        // A maximized window reports its maximized size, which would be restored as
        // the un-maximized size next run, so only record a size while it is not.
        m_persistentState["window_maximized"] = uiWindow->IsMaximized();
        if (!uiWindow->IsMaximized()) {
            m_persistentState["window_pos"] = uiWindow->GetPosition();
            m_persistentState["window_width"] = uiWindow->GetWidth();
            m_persistentState["window_height"] = uiWindow->GetHeight();
        }
    }
}

EditorApplication::~EditorApplication() {
    std::ofstream ofile(m_persistentFilePath.string());
    if (ofile.is_open()) {
        m_persistentState["current_path"] = std::filesystem::current_path().string();
        ofile << m_persistentState;
    }
}

void EditorApplication::PostCreateWindow() {
    ImGui::GetIO().IniFilename = s_imguiSettingsPath.c_str();
    imgui_ext::SetImGuiContext(&GetUiWindow()->GetImGuiContext());

    // Application only takes a size, and moth::gfx::platform::Window exposes position
    // and maximized state read-only, so the rest of the restore goes through GLFW.
    if (auto* const glfwWindow = dynamic_cast<moth::gfx::platform::glfw::Window*>(GetWindow())) {
        auto const position = m_persistentState.value("window_pos", moth::core::IntVec2{ -1, -1 });
        if (position.x > 0 && position.y > 0) {
            glfwSetWindowPos(glfwWindow->GetGLFWWindow(), position.x, position.y);
        }
        if (m_persistentState.value("window_maximized", false)) {
            glfwMaximizeWindow(glfwWindow->GetGLFWWindow());
        }
    }
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
        auto& assetContext = GetUiWindow()->GetSurfaceContext().GetAssetContext();
        if (auto texture = assetContext.TextureFromPixels(kSize, kSize, pixels.data())) {
            texture->SetFilter(moth::gfx::TextureFilter::Nearest, moth::gfx::TextureFilter::Nearest);
            GetUiWindow()->GetWindow().GetTextureFactory().SetFallbackTexture(std::move(texture));
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

    auto& uiWindow = *GetUiWindow();
    uiWindow.GetWindow().AddEventListener(this);
    uiWindow.PushLayer(std::make_unique<EditorLayer>(uiWindow.GetMothContext(), uiWindow.GetGraphics(), uiWindow.GetWindow().GetDevice(), uiWindow.GetSurfaceContext().GetAssetContext(), this));
}
