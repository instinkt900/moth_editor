#include "common.h"
#include "vulkan_app.h"
#include "editor/editor_layer.h"

#include "image_factory_vulkan.h"
#include "font_factory_vulkan.h"
#include "ui_renderer_vulkan.h"
#include "vulkan_graphics.h"

#include "moth_ui/context.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"

#include <iostream>

namespace backend::vulkan {
    char const* const Application::PERSISTENCE_FILE = "editor.json";

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

        m_graphics = std::make_unique<backend::vulkan::Graphics>();
        m_imageFactory = std::make_unique<ImageFactory>();
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

        m_layerStack = std::make_unique<LayerStack>(m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight);
        m_layerStack->SetEventListener(this);
        m_layerStack->PushLayer(std::make_unique<EditorLayer>());

        return true;
    }

    std::vector<char const*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool const enableValidationLayers =
#ifdef NDEBUG
        false
#else
        true
#endif
        ;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData) {
        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            spdlog::info("Validation Layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            spdlog::info("Validation Layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn("Validation Layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error("Validation Layer: {}", pCallbackData->pMessage);
            break;
        }

        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT const* pCreateInfo, VkAllocationCallbacks const* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks const* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    void Application::InitVulkan() {
        {
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

            createInfo.enabledLayerCount = 0;
            if (enableValidationLayers) {
                bool success = true;

                uint32_t layerCount;
                vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

                std::vector<VkLayerProperties> availableLayers(layerCount);
                vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

                for (char const* layerName : validationLayers) {
                    bool layerFound = false;

                    for (auto const& layerProperties : availableLayers) {
                        if (strcmp(layerName, layerProperties.layerName) == 0) {
                            layerFound = true;
                            break;
                        }
                    }

                    if (!layerFound) {
                        spdlog::error("Could not find validation layer {}.", layerName);
                        success = false;
                        break;
                    }
                }

                if (success) {
                    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                    createInfo.ppEnabledLayerNames = validationLayers.data();
                }

                VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
                populateDebugMessengerCreateInfo(debugCreateInfo);
                createInfo.pNext = &debugCreateInfo;
            }

            uint32_t glfwExtensionCount = 0;
            char const** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if (enableValidationLayers) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            CHECK_VK_RESULT(vkCreateInstance(&createInfo, m_vkAllocator, &m_vkInstance));
        }

        if (enableValidationLayers) {
            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            populateDebugMessengerCreateInfo(createInfo);
            if (VK_SUCCESS != CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, m_vkAllocator, &m_vkDebugMessenger)) {
                spdlog::error("Failed to set up the debug messenger!");
            }
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

    void Application::InitVulkanWindow(VkSurfaceKHR vkSurface, int width, int height) {
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

    void Application::VulkanFrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData) {
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
            info.pCommandBuffers = &fd->CommandBuffer;
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &renderCompleteSemaphore;
            CHECK_VK_RESULT(vkEndCommandBuffer(fd->CommandBuffer));
            CHECK_VK_RESULT(vkQueueSubmit(m_vkQueue, 1, &info, fd->Fence));
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
        VkResult result = vkQueuePresentKHR(m_vkQueue, &info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            m_vkSwapChainrebuild = true;
            return;
        }
        CHECK_VK_RESULT(result);
        wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;
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

    void Application::Shutdown() {
        vkDeviceWaitIdle(m_vkDevice);
        m_windowMaximized = glfwGetWindowAttrib(m_glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
        ImGui_ImplVulkanH_DestroyWindow(m_vkInstance, m_vkDevice, &m_imWindowData, m_vkAllocator);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, m_vkAllocator);
        }
        vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, m_vkAllocator);
        vkDestroyDevice(m_vkDevice, m_vkAllocator);
        vkDestroyInstance(m_vkInstance, m_vkAllocator);
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
}
