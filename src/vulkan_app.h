#pragma once

#include "iapplication.h"
#include "layers/layer_stack.h"
#include "events/event.h"

#include "moth_ui/iimage_factory.h"
#include "moth_ui/ifont_factory.h"
#include "moth_ui/irenderer.h"

#include "vulkan/vulkan_context.h"
#include "vulkan/vulkan_graphics.h"
#include "vulkan/vulkan_swapchain.h"

namespace backend::vulkan {
    class Application : public IApplication, public moth_ui::EventListener {
    public:
        Application();
        virtual ~Application();

        int Run() override;
        void Stop() override { m_running = false; }

        bool OnEvent(moth_ui::Event const& event) override;

        void SetWindowTitle(std::string const& title) override;

        nlohmann::json& GetPersistentState() override { return m_persistentState; }

        backend::IGraphicsContext& GetGraphics() override { return *m_graphics; }

    protected:
        bool Initialise();

        void Update();
        void Draw();
        void Shutdown();

    private:
        static int constexpr INIT_WINDOW_WIDTH = 1280;
        static int constexpr INIT_WINDOW_HEIGHT = 960;

        moth_ui::IntVec2 m_windowPos = { -1, -1 };
        int m_windowWidth = 0;
        int m_windowHeight = 0;
        bool m_windowMaximized = false;

        bool m_running = false;
        bool m_paused = false;
        std::chrono::milliseconds m_updateTicks;
        std::chrono::time_point<std::chrono::steady_clock> m_lastUpdateTicks;

        moth_ui::IntVec2 m_gameWindowPos;

        std::unique_ptr<LayerStack> m_layerStack;

        std::string m_imguiSettingsPath;
        std::filesystem::path m_persistentFilePath;
        nlohmann::json m_persistentState;
        static char const* const IMGUI_FILE;
        static char const* const PERSISTENCE_FILE;

        std::unique_ptr<moth_ui::IImageFactory> m_imageFactory;
        std::unique_ptr<moth_ui::IFontFactory> m_fontFactory;
        std::unique_ptr<moth_ui::IRenderer> m_uiRenderer;

        std::unique_ptr<Graphics> m_graphics;

        GLFWwindow* m_glfwWindow = nullptr;

        std::unique_ptr<Context> m_context;
        bool m_vkSwapChainrebuild = false;

        bool OnWindowSizeEvent(EventWindowSize const& event);
        bool OnQuitEvent(EventQuit const& event);

        VkSurfaceKHR m_customVkSurface;

        void ImGuiInit();

        void OnResize();
    };
}
