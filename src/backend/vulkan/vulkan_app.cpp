#include "common.h"
#include "vulkan_app.h"
#include "editor/editor_layer.h"

#include "vulkan_image_factory.h"
#include "vulkan_font_factory.h"
#include "vulkan_ui_renderer.h"
#include "vulkan_graphics.h"

#include "moth_ui/context.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"

#include <iostream>

#include <glm/glm.hpp>

namespace backend::vulkan {
    char const* const Application::PERSISTENCE_FILE = "editor.json";

    void checkVkResult(VkResult err) {
        CHECK_VK_RESULT(err);
    }

    Application::Application()
        : m_windowWidth(INIT_WINDOW_WIDTH)
        , m_windowHeight(INIT_WINDOW_HEIGHT) {
        m_updateTicks = std::chrono::milliseconds(1000 / 60);

        m_persistentFilePath = std::filesystem::current_path() / PERSISTENCE_FILE;
        std::ifstream persistenceFile(m_persistentFilePath.string());
        if (persistenceFile.is_open()) {
            try {
                persistenceFile >> m_persistentState;
            } catch (std::exception) {
            }

            if (!m_persistentState.is_null()) {
                m_windowPos = m_persistentState.value("window_pos", m_windowPos);
                m_windowWidth = m_persistentState.value("window_width", m_windowWidth);
                m_windowHeight = m_persistentState.value("window_height", m_windowHeight);
                m_windowMaximized = m_persistentState.value("window_maximized", m_windowMaximized);
            }
        }
    }

    Application::~Application() {
        std::ofstream ofile(m_persistentFilePath.string());
        if (ofile.is_open()) {
            m_persistentState["current_path"] = std::filesystem::current_path().string();
            m_persistentState["window_pos"] = m_windowPos;
            m_persistentState["window_width"] = m_windowWidth;
            m_persistentState["window_height"] = m_windowHeight;
            m_persistentState["window_maximized"] = m_windowMaximized;
            ofile << m_persistentState;
        }
    }

    int Application::Run() {
        if (!Initialise()) {
            return 1;
        }

        m_running = true;
        m_lastUpdateTicks = std::chrono::steady_clock::now();

        while (m_running) {
            glfwPollEvents();

            if (glfwWindowShouldClose(m_glfwWindow)) {
                OnEvent(EventRequestQuit());
            }

            if (m_vkSwapChainrebuild) {
                int width, height;
                glfwGetFramebufferSize(m_glfwWindow, &width, &height);
                if (width > 0 && height > 0) {
                    assert(false && "TODO");
                }
            }

            Update();
            Draw();
        }

        Shutdown();

        return 0;
    }

    bool Application::Initialise() {
        if (!glfwInit()) {
            return false;
        }

        if (!glfwVulkanSupported()) {
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_glfwWindow = glfwCreateWindow(m_windowWidth, m_windowHeight, "UI Tool", nullptr, nullptr);
        glfwSetWindowUserPointer(m_glfwWindow, this);

        if (m_windowPos.x != -1 && m_windowPos.y != -1) {
            glfwSetWindowPos(m_glfwWindow, m_windowPos.x, m_windowPos.y);
        }

        glfwSetWindowPosCallback(m_glfwWindow, [](GLFWwindow* window, int xpos, int ypos) {
            Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            app->m_windowMaximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
            if (!app->m_windowMaximized) {
                app->m_windowPos.x = xpos;
                app->m_windowPos.y = ypos;
            }
        });

        glfwSetWindowSizeCallback(m_glfwWindow, [](GLFWwindow* window, int width, int height) {
            Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
            app->m_windowMaximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
            if (!app->m_windowMaximized) {
                app->m_windowWidth = width;
                app->m_windowHeight = height;
            }
            app->OnResize();
        });

        if (m_windowMaximized) {
            glfwMaximizeWindow(m_glfwWindow);
        }

        m_context = std::make_unique<Context>();

        VkSurfaceKHR vkSurface;
        CHECK_VK_RESULT(glfwCreateWindowSurface(m_context->m_vkInstance, m_glfwWindow, nullptr, &vkSurface));
        m_customVkSurface = vkSurface;

        int width, height;
        glfwGetFramebufferSize(m_glfwWindow, &width, &height);

        m_graphics = std::make_unique<Graphics>(*m_context, m_customVkSurface, m_windowWidth, m_windowHeight);
        m_imageFactory = std::make_unique<ImageFactory>(*m_context, static_cast<Graphics&>(*m_graphics));
        m_fontFactory = std::make_unique<FontFactory>(*m_context, *m_graphics);
        m_uiRenderer = std::make_unique<UIRenderer>(*m_graphics);
        auto uiContext = std::make_shared<moth_ui::Context>(m_imageFactory.get(), m_fontFactory.get(), m_uiRenderer.get());
        moth_ui::Context::SetCurrentContext(uiContext);

        SubImage::s_graphicsContext = m_graphics.get();

        if (m_persistentState.contains("current_path")) {
            std::string const currentPath = m_persistentState["current_path"];
            try {
                std::filesystem::current_path(currentPath);
            } catch (std::exception) {
                // ...
            }
        }

        ImGuiInit();
        
        m_layerStack = std::make_unique<LayerStack>(m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight);
        m_layerStack->SetEventListener(this);
        m_layerStack->PushLayer(std::make_unique<EditorLayer>());

        return true;
    }

    bool Application::OnEvent(moth_ui::Event const& event) {
        moth_ui::EventDispatch dispatch(event);
        dispatch.Dispatch(this, &Application::OnWindowSizeEvent);
        dispatch.Dispatch(this, &Application::OnQuitEvent);
        dispatch.Dispatch(m_layerStack.get());
        return dispatch.GetHandled();
    }

    void Application::SetWindowTitle(std::string const& title) {
        glfwSetWindowTitle(m_glfwWindow, title.c_str());
    }

    void Application::Update() {
        auto const nowTicks = std::chrono::steady_clock::now();
        auto deltaTicks = std::chrono::duration_cast<std::chrono::milliseconds>(nowTicks - m_lastUpdateTicks);
        while (deltaTicks > m_updateTicks) {
            if (!m_paused) {
                m_layerStack->Update(static_cast<uint32_t>(m_updateTicks.count()));
            }
            m_lastUpdateTicks += m_updateTicks;
            deltaTicks -= m_updateTicks;
        }
    }

    void Application::Draw() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        m_graphics->Begin();
        m_layerStack->Draw();
        ImGui::Render();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
        if (ImDrawData* drawData = ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(drawData, m_graphics->GetCurrentCommandBuffer()->GetVkCommandBuffer());
        }
        m_graphics->End();
    }

    void Application::Shutdown() {
        vkDeviceWaitIdle(m_context->m_vkDevice);
        m_layerStack.reset(); // force layers to cleanup before we destroy all the devices.
        m_uiRenderer.reset();
        m_fontFactory.reset();
        m_imageFactory.reset();
        m_graphics.reset();
        m_windowMaximized = glfwGetWindowAttrib(m_glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroySurfaceKHR(m_context->m_vkInstance, m_customVkSurface, nullptr);
        glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
        m_context.reset();
    }

    bool Application::OnWindowSizeEvent(EventWindowSize const& event) {
        m_windowWidth = event.GetWidth();
        m_windowHeight = event.GetHeight();
        m_layerStack->SetWindowSize({ m_windowWidth, m_windowHeight });
        return true;
    }

    bool Application::OnQuitEvent(EventQuit const& event) {
        m_running = false;
        return true;
    }

    void Application::ImGuiInit() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplGlfw_InitForVulkan(m_glfwWindow, true);
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = m_context->m_vkInstance;
        initInfo.PhysicalDevice = m_context->m_vkPhysicalDevice;
        initInfo.Device = m_context->m_vkDevice;
        initInfo.QueueFamily = m_context->m_vkQueueFamily;
        initInfo.Queue = m_context->m_vkQueue;
        initInfo.DescriptorPool = m_context->m_vkDescriptorPool;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = m_graphics->GetSwapchain().GetImageCount();
        initInfo.ImageCount = m_graphics->GetSwapchain().GetImageCount();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = checkVkResult;
        ImGui_ImplVulkan_Init(&initInfo, m_graphics->GetRenderPass().GetRenderPass());

        // create the font texture
        {
            VkCommandPool commandPool = m_context->m_vkCommandPool;
            CHECK_VK_RESULT(vkResetCommandPool(m_context->m_vkDevice, commandPool, 0));
            VkCommandBuffer commandBuffer = m_context->beginSingleTimeCommands();
            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
            m_context->endSingleTimeCommands(commandBuffer);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }

    void Application::OnResize() {
        m_graphics->OnResize(m_customVkSurface, m_windowWidth, m_windowHeight);
    }
}
