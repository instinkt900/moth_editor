#include "common.h"
#include "vulkan_app.h"
#include "editor/editor_layer.h"

#include "vulkan_image_factory.h"
#include "font_factory_vulkan.h"
#include "ui_renderer_vulkan.h"
#include "vulkan_graphics.h"

#include "moth_ui/context.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"

#include <iostream>

#include <glm/glm.hpp>

namespace backend::vulkan {
    char const* const Application::PERSISTENCE_FILE = "editor.json";

#define CHECK_VK_RESULT(expr)                                                               \
    {                                                                                       \
        VkResult result_ = expr;                                                            \
        if (result_ != VK_SUCCESS) {                                                        \
            spdlog::error("File: {} Line: {} {} = {}", __FILE__, __LINE__, #expr, result_); \
            abort();                                                                        \
        }                                                                                   \
    }

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
                    ImGui_ImplVulkan_SetMinImageCount(m_imMinImageCount);
                    ImGui_ImplVulkanH_CreateOrResizeWindow(m_context->m_vkInstance, m_context->m_vkPhysicalDevice, m_context->m_vkDevice, &m_imWindowData, m_context->m_vkQueueFamily, nullptr, width, height, m_imMinImageCount);
                    m_imWindowData.FrameIndex = 0;
                    m_vkSwapChainrebuild = false;
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
        //InitVulkanWindow(vkSurface, width, height);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        m_graphics = std::make_unique<Graphics>(*m_context, m_customVkSurface, m_windowWidth, m_windowHeight);
        m_imageFactory = std::make_unique<ImageFactory>(*m_context, static_cast<Graphics&>(*m_graphics));
        m_fontFactory = std::make_unique<FontFactory>();
        m_uiRenderer = std::make_unique<UIRenderer>();
        auto uiContext = std::make_shared<moth_ui::Context>(m_imageFactory.get(), m_fontFactory.get(), m_uiRenderer.get());
        moth_ui::Context::SetCurrentContext(uiContext);

        if (m_persistentState.contains("current_path")) {
            std::string const currentPath = m_persistentState["current_path"];
            try {
                std::filesystem::current_path(currentPath);
            } catch (std::exception) {
                // ...
            }
        }

        //ImGuiInit();
        CustomInit();

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
        //ImGui_ImplVulkan_NewFrame();
        //ImGui_ImplGlfw_NewFrame();
        //ImGui::NewFrame();

        //m_layerStack->Draw();

        //ImGui::Render();
        //ImDrawData* drawData = ImGui::GetDrawData();
        //VulkanFrameRender(&m_imWindowData, drawData);

        //if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //    ImGui::UpdatePlatformWindows();
        //    ImGui::RenderPlatformWindowsDefault();
        //}

        //VulkanFramePresent(&m_imWindowData);

        CustomFrameRender();
    }

    void Application::Shutdown() {
        m_layerStack.reset(); // force layers to cleanup before we destroy all the devices.
        vkDeviceWaitIdle(m_context->m_vkDevice);
        m_graphics.reset();
        m_windowMaximized = glfwGetWindowAttrib(m_glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
        ImGui_ImplVulkanH_DestroyWindow(m_context->m_vkInstance, m_context->m_vkDevice, &m_imWindowData, nullptr);
        //ImGui_ImplVulkan_Shutdown();
        //ImGui_ImplGlfw_Shutdown();
        //ImGui::DestroyContext();
        //vkDestroyShaderModule(m_context->m_vkDevice, m_vkVertShaderModule, nullptr);
        //vkDestroyShaderModule(m_context->m_vkDevice, m_vkFragShaderModule, nullptr);
        //vkDestroyPipelineLayout(m_context->m_vkDevice, m_vkPipelineLayout, nullptr);
        //vkDestroyPipeline(m_context->m_vkDevice, m_imagePipeline, nullptr);
        //vkDestroyRenderPass(m_context->m_vkDevice, m_vkRenderPass, nullptr);
        m_context.reset();
        glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
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

    void Application::InitVulkanWindow(VkSurfaceKHR vkSurface, int width, int height) {
        m_imWindowData.Surface = vkSurface;

        VkBool32 result;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_context->m_vkPhysicalDevice, m_context->m_vkQueueFamily, m_imWindowData.Surface, &result);
        if (result != VK_TRUE) {
            std::cerr << "No WSI support\n";
            exit(-1);
        }

        VkFormat const requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_B8G8R8_UNORM };
        VkColorSpaceKHR const requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        m_imWindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(m_context->m_vkPhysicalDevice, m_imWindowData.Surface, requestSurfaceImageFormat, static_cast<size_t>(IM_ARRAYSIZE(requestSurfaceImageFormat)), requestSurfaceColorSpace);

        VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
        m_imWindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(m_context->m_vkPhysicalDevice, m_imWindowData.Surface, presentModes, IM_ARRAYSIZE(presentModes));

        ImGui_ImplVulkanH_CreateOrResizeWindow(m_context->m_vkInstance, m_context->m_vkPhysicalDevice, m_context->m_vkDevice, &m_imWindowData, m_context->m_vkQueueFamily, nullptr, width, height, m_imMinImageCount);
    }

    void Application::ImGuiInit() {
        ImGui_ImplGlfw_InitForVulkan(m_glfwWindow, true);
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = m_context->m_vkInstance;
        initInfo.PhysicalDevice = m_context->m_vkPhysicalDevice;
        initInfo.Device = m_context->m_vkDevice;
        initInfo.QueueFamily = m_context->m_vkQueueFamily;
        initInfo.Queue = m_context->m_vkQueue;
        initInfo.DescriptorPool = m_context->m_vkDescriptorPool;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = m_imMinImageCount;
        initInfo.ImageCount = m_imWindowData.ImageCount;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = checkVkResult;
        ImGui_ImplVulkan_Init(&initInfo, m_imWindowData.RenderPass);

        // create the font texture
        {
            VkCommandPool commandPool = m_imWindowData.Frames[m_imWindowData.FrameIndex].CommandPool;
            VkCommandBuffer commandBuffer = m_imWindowData.Frames[m_imWindowData.FrameIndex].CommandBuffer;
            CHECK_VK_RESULT(vkResetCommandPool(m_context->m_vkDevice, commandPool, 0));
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CHECK_VK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

            VkSubmitInfo endInfo{};
            endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            endInfo.commandBufferCount = 1;
            endInfo.pCommandBuffers = &commandBuffer;
            CHECK_VK_RESULT(vkEndCommandBuffer(commandBuffer));
            CHECK_VK_RESULT(vkQueueSubmit(m_context->m_vkQueue, 1, &endInfo, VK_NULL_HANDLE));
            CHECK_VK_RESULT(vkDeviceWaitIdle(m_context->m_vkDevice));

            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }

    void Application::VulkanFrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData) {
        VkResult result;

        VkSemaphore imageAcquiredSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
        VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        result = vkAcquireNextImageKHR(m_context->m_vkDevice, wd->Swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &wd->FrameIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            m_vkSwapChainrebuild = true;
            return;
        }
        CHECK_VK_RESULT(result);

        ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
        {
            CHECK_VK_RESULT(vkWaitForFences(m_context->m_vkDevice, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
            CHECK_VK_RESULT(vkResetFences(m_context->m_vkDevice, 1, &fd->Fence));
        }

        {
            CHECK_VK_RESULT(vkResetCommandPool(m_context->m_vkDevice, fd->CommandPool, 0));
            VkCommandBufferBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            CHECK_VK_RESULT(vkBeginCommandBuffer(fd->CommandBuffer, &info));
        }

        {
            VkRenderPassBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = wd->RenderPass;
            info.framebuffer = fd->Framebuffer;
            info.renderArea.extent.width = wd->Width;
            info.renderArea.extent.height = wd->Height;
            info.clearValueCount = 1;
            info.pClearValues = &wd->ClearValue;
            vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }

        ImGui_ImplVulkan_RenderDrawData(drawData, fd->CommandBuffer);

        vkCmdEndRenderPass(fd->CommandBuffer);

        {
            VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo info{};
            info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &imageAcquiredSemaphore;
            info.pWaitDstStageMask = &waitStageFlags;
            info.commandBufferCount = 1;
            VkCommandBuffer cmdBuffers[] = { fd->CommandBuffer };
            info.pCommandBuffers = cmdBuffers;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &renderCompleteSemaphore;
            CHECK_VK_RESULT(vkEndCommandBuffer(fd->CommandBuffer));
            CHECK_VK_RESULT(vkQueueSubmit(m_context->m_vkQueue, 1, &info, fd->Fence));
        }
    }

    void Application::VulkanFramePresent(ImGui_ImplVulkanH_Window* wd) {
        if (m_vkSwapChainrebuild) {
            return;
        }

        VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &renderCompleteSemaphore;
        info.swapchainCount = 1;
        info.pSwapchains = &wd->Swapchain;
        info.pImageIndices = &wd->FrameIndex;
        VkResult result = vkQueuePresentKHR(m_context->m_vkQueue, &info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            m_vkSwapChainrebuild = true;
            return;
        }
        CHECK_VK_RESULT(result);
        wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
    }

    void Application::CustomInit() {
        m_imageFactory->LoadTexturePack("D:/Development/ChristmasProject2021/resources/packs/packed_0.png");
        m_testTarget = m_graphics->CreateTarget(500, 500);
    }

    void Application::CustomFrameRender() {
        // test draw
        m_graphics->Begin();

        m_graphics->SetTarget(m_testTarget.get());
        m_graphics->SetBlendMode(EBlendMode::None);
        m_graphics->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
        m_graphics->Clear();
        m_graphics->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
        m_graphics->DrawLineF({ 500.0f, 0.0f }, { 0.0f, 500.0f });
        m_graphics->SetTarget(nullptr);

        moth_ui::FloatRect rect;

        //rect = moth_ui::MakeRect(10.0f, 10.0f, 320.0f, 320.0f);
        //m_graphics->SetBlendMode(EBlendMode::Blend);
        //m_graphics->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        //m_graphics->DrawRectF(rect);

        m_graphics->SetBlendMode(EBlendMode::None);
        m_graphics->SetColor({ 0.3f, 0.3f, 0.3f, 1.0f });
        m_graphics->Clear();

        rect = moth_ui::MakeRect(25.0f, 25.0f, 550.0f, 550.0f);
        m_graphics->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
        m_graphics->DrawRectF(rect);

        m_graphics->SetBlendMode(EBlendMode::Add);

        rect = moth_ui::MakeRect(50.0f, 50.0f, 300.0f, 300.0f);
        m_graphics->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        m_graphics->DrawFillRectF(rect);

        rect = moth_ui::MakeRect(250.0f, 50.0f, 300.0f, 300.0f);
        m_graphics->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f });
        m_graphics->DrawFillRectF(rect);

        rect = moth_ui::MakeRect(150.0f, 250.0f, 300.0f, 300.0f);
        m_graphics->SetColor({ 0.0f, 0.0f, 1.0f, 1.0f });
        m_graphics->DrawFillRectF(rect);

        m_graphics->SetBlendMode(EBlendMode::Blend);

        m_graphics->SetColor({ 0.0f, 1.0f, 1.0f, 0.5f });
        m_graphics->DrawLineF({ 20.0f, 10.0f }, { 560.0f, 600.0f });

        auto image = m_imageFactory->GetImage("D:\\Development\\ChristmasProject2021\\resources\\images\\laser.png");
        auto irect = moth_ui::MakeRect(300, 300, 300, 300);
        m_graphics->SetBlendMode(EBlendMode::Blend);
        m_graphics->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        auto const srcRect = moth_ui::MakeRect(63, 60, 50, 50);
        m_graphics->DrawImage(*image, &srcRect, &irect);

        irect = moth_ui::MakeRect(10, 10, 300, 300);
        m_graphics->DrawImage(*m_testTarget->GetImage(), nullptr, &irect);

        m_graphics->End();
    }
}
