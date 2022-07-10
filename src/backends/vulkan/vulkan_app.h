#pragma once

#include "../../iapp.h"
#include "layers/layer_stack.h"
#include "events/event.h"

#include "moth_ui/iimage_factory.h"
#include "moth_ui/ifont_factory.h"
#include "moth_ui/irenderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

class VulkanApp : public IApp, public moth_ui::EventListener {
public:
    VulkanApp();
    virtual ~VulkanApp();

    int Run() override;
    void Stop() override { m_running = false; }

    bool OnEvent(moth_ui::Event const& event) override;

    void SetWindowTitle(std::string const& title) override;

    nlohmann::json& GetPersistentState() override { return m_persistentState; }

    backend::IGraphics& GetGraphics() override { return *m_graphics; }

protected:
    bool Initialise();

    void Update();
    void Draw();
    void Shutdown();

private:
    static int constexpr INIT_WINDOW_WIDTH = 1280;
    static int constexpr INIT_WINDOW_HEIGHT = 960;

    int m_windowWidth = 0;
    int m_windowHeight = 0;

    bool m_running = false;
    bool m_paused = false;
    bool m_editorMode = false;
    std::chrono::milliseconds m_updateTicks;
    std::chrono::time_point<std::chrono::steady_clock> m_lastUpdateTicks;

    moth_ui::IntVec2 m_gameWindowPos;

    std::unique_ptr<LayerStack> m_layerStack;

    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const PERSISTENCE_FILE;

    std::unique_ptr<moth_ui::IImageFactory> m_imageFactory;
    std::unique_ptr<moth_ui::IFontFactory> m_fontFactory;
    std::unique_ptr<moth_ui::IRenderer> m_uiRenderer;

    std::unique_ptr<backend::IGraphics> m_graphics;

    // glfw stuff
    GLFWwindow* m_glfwWindow = nullptr;
    //

    // vulkan stuff
    VkAllocationCallbacks* m_vkAllocator = nullptr;
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t m_vkQueueFamily = static_cast<uint32_t>(-1);
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VkQueue m_vkQueue = VK_NULL_HANDLE;
    VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
    ImGui_ImplVulkanH_Window m_imWindowData;
    int m_imMinImageCount = 2;
    VkPipelineCache m_vkPipelineCache = VK_NULL_HANDLE;
    bool m_vkSwapChainrebuild = false;

    void InitVulkan();
    void InitVulkanWindow(VkSurfaceKHR vkSurface, int width, int height);
    void VulkanFrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData);
    void VulkanFramePresent(ImGui_ImplVulkanH_Window* wd);
    //

    bool OnWindowSizeEvent(EventWindowSize const& event);
    bool OnQuitEvent(EventQuit const& event);
};

