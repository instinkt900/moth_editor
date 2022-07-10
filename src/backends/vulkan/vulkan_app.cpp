#include "common.h"
#include "vulkan_app.h"
#include "editor/editor_layer.h"

#include "image_factory_vulkan.h"
#include "font_factory_vulkan.h"
#include "ui_renderer_vulkan.h"
#include "backends/vulkan/vulkan_graphics.h"

#include "moth_ui/context.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"

#include <iostream>

char const* const VulkanApp::PERSISTENCE_FILE = "editor.json";

#define CHECK_VK_RESULT(expr)                                                                                \
    {                                                                                                        \
        VkResult result_ = expr;                                                                             \
        if (result_ != VK_SUCCESS) {                                                                         \
            std::cerr << "File: " << __FILE__ << " Line: " << __LINE__ << #expr << " = " << result_ << "\n"; \
            abort();                                                                                         \
        }                                                                                                    \
    }

void checkVkResult(VkResult err) {
    CHECK_VK_RESULT(err);
}

VulkanApp::VulkanApp()
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

        m_windowWidth = m_persistentState.value("window_width", m_windowWidth);
        m_windowHeight = m_persistentState.value("window_height", m_windowHeight);
    }
}

VulkanApp::~VulkanApp() {
    std::ofstream ofile(m_persistentFilePath.string());
    if (ofile.is_open()) {
        m_persistentState["current_path"] = std::filesystem::current_path().string();
        m_persistentState["window_width"] = m_windowWidth;
        m_persistentState["window_height"] = m_windowHeight;
        ofile << m_persistentState;
    }
}

int VulkanApp::Run() {
    if (!Initialise()) {
        return 1;
    }

    m_running = true;
    m_lastUpdateTicks = std::chrono::steady_clock::now();

    while (m_running) {
        glfwPollEvents();

        if (m_vkSwapChainrebuild) {
            int width, height;
            glfwGetFramebufferSize(m_glfwWindow, &width, &height);
            if (width > 0 && height > 0) {
                ImGui_ImplVulkan_SetMinImageCount(m_imMinImageCount);
                ImGui_ImplVulkanH_CreateOrResizeWindow(m_vkInstance, m_vkPhysicalDevice, m_vkDevice, &m_imWindowData, m_vkQueueFamily, m_vkAllocator, width, height, m_imMinImageCount);
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

bool VulkanApp::Initialise() {
    if (!glfwInit()) {
        return false;
    }

    if (!glfwVulkanSupported()) {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_glfwWindow = glfwCreateWindow(640, 480, "UI Tool", nullptr, nullptr);

    InitVulkan();

    VkSurfaceKHR vkSurface;
    CHECK_VK_RESULT(glfwCreateWindowSurface(m_vkInstance, m_glfwWindow, m_vkAllocator, &vkSurface));

    int width, height;
    glfwGetFramebufferSize(m_glfwWindow, &width, &height);
    InitVulkanWindow(vkSurface, width, height);

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
    initInfo.Instance = m_vkInstance;
    initInfo.PhysicalDevice = m_vkPhysicalDevice;
    initInfo.Device = m_vkDevice;
    initInfo.QueueFamily = m_vkQueueFamily;
    initInfo.Queue = m_vkQueue;
    initInfo.PipelineCache = m_vkPipelineCache;
    initInfo.DescriptorPool = m_vkDescriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = m_imMinImageCount;
    initInfo.ImageCount = m_imWindowData.ImageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = m_vkAllocator;
    initInfo.CheckVkResultFn = checkVkResult;
    ImGui_ImplVulkan_Init(&initInfo, m_imWindowData.RenderPass);

    {
        VkCommandPool commandPool = m_imWindowData.Frames[m_imWindowData.FrameIndex].CommandPool;
        VkCommandBuffer commandBuffer = m_imWindowData.Frames[m_imWindowData.FrameIndex].CommandBuffer;
        CHECK_VK_RESULT(vkResetCommandPool(m_vkDevice, commandPool, 0));
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
        CHECK_VK_RESULT(vkQueueSubmit(m_vkQueue, 1, &endInfo, VK_NULL_HANDLE));
        CHECK_VK_RESULT(vkDeviceWaitIdle(m_vkDevice));

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    m_graphics = std::make_unique<backend::vulkan::VulkanGraphics>();
    m_imageFactory = std::make_unique<ImageFactoryVulkan>();
    m_fontFactory = std::make_unique<FontFactoryVulkan>();
    m_uiRenderer = std::make_unique<UIRendererVulkan>();
    auto uiContext = std::make_shared<moth_ui::Context>(m_imageFactory.get(), m_fontFactory.get(), m_uiRenderer.get());
    moth_ui::Context::SetCurrentContext(uiContext);

    if (m_persistentState.contains("current_path")) {
        std::string const currentPath = m_persistentState["current_path"];
        std::filesystem::current_path(currentPath);
    }

    m_layerStack = std::make_unique<LayerStack>(m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight);
    m_layerStack->SetEventListener(this);
    m_layerStack->PushLayer(std::make_unique<EditorLayer>());

    return true;
}

void VulkanApp::InitVulkan() {
    {
        uint32_t extensionsCount = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions;
        CHECK_VK_RESULT(vkCreateInstance(&createInfo, m_vkAllocator, &m_vkInstance));
    }

    {
        uint32_t gpuCount;
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, nullptr));
        std::vector<VkPhysicalDevice> gpus(gpuCount);
        CHECK_VK_RESULT(vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, gpus.data()));

        uint32_t selectedGpu = 0;
        for (uint32_t i = 0; i < gpuCount; ++i) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(gpus[i], &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                selectedGpu = i;
                break;
            }
        }

        m_vkPhysicalDevice = gpus[selectedGpu];
    }

    {
        uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueCount, queues.data());

        for (uint32_t i = 0; i < queueCount; ++i) {
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_vkQueueFamily = i;
                break;
            }
        }
    }

    {
        int deviceExtensionCount = 1;
        char const* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        float const queuePriority[] = { 1.0f };
        VkDeviceQueueCreateInfo queueInfo[1] = {};
        queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo[0].queueFamilyIndex = m_vkQueueFamily;
        queueInfo[0].queueCount = 1;
        queueInfo[0].pQueuePriorities = queuePriority;
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = IM_ARRAYSIZE(queueInfo);
        createInfo.pQueueCreateInfos = queueInfo;
        createInfo.enabledExtensionCount = deviceExtensionCount;
        createInfo.ppEnabledExtensionNames = deviceExtensions;
        CHECK_VK_RESULT(vkCreateDevice(m_vkPhysicalDevice, &createInfo, m_vkAllocator, &m_vkDevice));
        vkGetDeviceQueue(m_vkDevice, m_vkQueueFamily, 0, &m_vkQueue);
    }

    {
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
        };
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
        poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
        poolInfo.pPoolSizes = poolSizes;
        CHECK_VK_RESULT(vkCreateDescriptorPool(m_vkDevice, &poolInfo, m_vkAllocator, &m_vkDescriptorPool));
    }
}

void VulkanApp::InitVulkanWindow(VkSurfaceKHR vkSurface, int width, int height) {
    m_imWindowData.Surface = vkSurface;

    VkBool32 result;
    vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, m_vkQueueFamily, m_imWindowData.Surface, &result);
    if (result != VK_TRUE) {
        std::cerr << "No WSI support\n";
        exit(-1);
    }

    VkFormat const requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_B8G8R8_UNORM };
    VkColorSpaceKHR const requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    m_imWindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(m_vkPhysicalDevice, m_imWindowData.Surface, requestSurfaceImageFormat, static_cast<size_t>(IM_ARRAYSIZE(requestSurfaceImageFormat)), requestSurfaceColorSpace);

    VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
    m_imWindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(m_vkPhysicalDevice, m_imWindowData.Surface, presentModes, IM_ARRAYSIZE(presentModes));

    ImGui_ImplVulkanH_CreateOrResizeWindow(m_vkInstance, m_vkPhysicalDevice, m_vkDevice, &m_imWindowData, m_vkQueueFamily, m_vkAllocator, width, height, m_imMinImageCount);
}

void VulkanApp::VulkanFrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData) {
    VkResult result;

    VkSemaphore imageAcquiredSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore renderCompleteSemaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    result = vkAcquireNextImageKHR(m_vkDevice, wd->Swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_vkSwapChainrebuild = true;
        return;
    }
    CHECK_VK_RESULT(result);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        CHECK_VK_RESULT(vkWaitForFences(m_vkDevice, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
        CHECK_VK_RESULT(vkResetFences(m_vkDevice, 1, &fd->Fence));
    }

    {
        CHECK_VK_RESULT(vkResetCommandPool(m_vkDevice, fd->CommandPool, 0));
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CHECK_VK_RESULT(vkBeginCommandBuffer(fd->CommandBuffer, &info));
    }

    {
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
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
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &renderCompleteSemaphore;
        CHECK_VK_RESULT(vkEndCommandBuffer(fd->CommandBuffer));
        CHECK_VK_RESULT(vkQueueSubmit(m_vkQueue, 1, &info, fd->Fence));
    }
}

void VulkanApp::VulkanFramePresent(ImGui_ImplVulkanH_Window* wd) {
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
    VkResult result = vkQueuePresentKHR(m_vkQueue, &info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_vkSwapChainrebuild = true;
        return;
    }
    CHECK_VK_RESULT(result);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
}

bool VulkanApp::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &VulkanApp::OnWindowSizeEvent);
    dispatch.Dispatch(this, &VulkanApp::OnQuitEvent);
    dispatch.Dispatch(m_layerStack.get());
    return dispatch.GetHandled();
}

void VulkanApp::SetWindowTitle(std::string const& title) {
    //SDL_SetWindowTitle(m_glfwWindow, title.c_str());
}

void VulkanApp::Update() {
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

void VulkanApp::Draw() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_layerStack->Draw();

    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    VulkanFrameRender(&m_imWindowData, drawData);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    VulkanFramePresent(&m_imWindowData);
}

void VulkanApp::Shutdown() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_glfwWindow);
}

bool VulkanApp::OnWindowSizeEvent(EventWindowSize const& event) {
    m_windowWidth = event.GetWidth();
    m_windowHeight = event.GetHeight();
    m_layerStack->SetWindowSize({ m_windowWidth, m_windowHeight });
    return true;
}

bool VulkanApp::OnQuitEvent(EventQuit const& event) {
    m_running = false;
    return true;
}
