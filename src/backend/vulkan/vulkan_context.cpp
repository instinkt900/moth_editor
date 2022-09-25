#include "common.h"
#include "vulkan_context.h"

#define CHECK_VK_RESULT(expr)                                                               \
    {                                                                                       \
        VkResult result_ = expr;                                                            \
        if (result_ != VK_SUCCESS) {                                                        \
            spdlog::error("File: {} Line: {} {} = {}", __FILE__, __LINE__, #expr, result_); \
            abort();                                                                        \
        }                                                                                   \
    }

namespace {
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

    char const* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData) {
        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            //spdlog::info("Validation Layer: {}", pCallbackData->pMessage);
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
}

namespace backend::vulkan {
    Context::Context() {

        // create instance
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
            CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_vkInstance));
        }

        if (enableValidationLayers) {
            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            populateDebugMessengerCreateInfo(createInfo);
            CHECK_VK_RESULT(CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger));
        }

        // select device
        {
            uint32_t gpuCount;
            CHECK_VK_RESULT(vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, nullptr));
            std::vector<VkPhysicalDevice> gpus(gpuCount);
            CHECK_VK_RESULT(vkEnumeratePhysicalDevices(m_vkInstance, &gpuCount, gpus.data()));

            uint32_t selectedGpu = 0;
            for (uint32_t i = 0; i < gpuCount; ++i) {
                VkPhysicalDeviceFeatures features;
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(gpus[i], &properties);
                vkGetPhysicalDeviceFeatures(gpus[i], &features);
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.samplerAnisotropy == VK_TRUE) {
                    selectedGpu = i;
                    break;
                }
            }

            m_vkPhysicalDevice = gpus[selectedGpu];
        }

        // queue family
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

        // device
        {
            int deviceExtensionCount = 1;
            float const queuePriority[] = { 1.0f };
            VkDeviceQueueCreateInfo queueInfo[1] = {};
            queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo[0].queueFamilyIndex = m_vkQueueFamily;
            queueInfo[0].queueCount = 1;
            queueInfo[0].pQueuePriorities = queuePriority;
            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = IM_ARRAYSIZE(queueInfo);
            createInfo.pQueueCreateInfos = queueInfo;
            createInfo.enabledExtensionCount = deviceExtensionCount;
            createInfo.ppEnabledExtensionNames = deviceExtensions;
            createInfo.pEnabledFeatures = &deviceFeatures;
            CHECK_VK_RESULT(vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice));
            vkGetDeviceQueue(m_vkDevice, m_vkQueueFamily, 0, &m_vkQueue);
        }

        // descriptor pool
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
            CHECK_VK_RESULT(vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool));
        }

        // command pool
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = m_vkQueueFamily;

            CHECK_VK_RESULT(vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool));
        }

        // allocator
        {
            VmaAllocatorCreateInfo allocatorCreateInfo{};
            allocatorCreateInfo.instance = m_vkInstance;
            allocatorCreateInfo.physicalDevice = m_vkPhysicalDevice;
            allocatorCreateInfo.device = m_vkDevice;
            vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator);
        }
    }

    Context::~Context() {
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
        }
        vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
        vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
        vmaDestroyAllocator(m_vmaAllocator);
        vkDestroyDevice(m_vkDevice, nullptr);
        vkDestroyInstance(m_vkInstance, nullptr);
    }

    VkCommandBuffer Context::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void Context::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_vkQueue);

        vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
    }

    void Context::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image, VmaAllocation& allocation) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        CHECK_VK_RESULT(vmaCreateImage(m_vmaAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr));
    }

    VkShaderModule Context::createShaderModule(std::vector<char> const& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<uint32_t const*>(code.data());

        VkShaderModule shaderModule;
        CHECK_VK_RESULT(vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }

    VkImageView Context::createImageView(VkImage image, VkFormat format) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        CHECK_VK_RESULT(vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView));

        return imageView;
    }

    VkSampler Context::createSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;

        // todo can probably cache this
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &properties);

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        CHECK_VK_RESULT(vkCreateSampler(m_vkDevice, &samplerInfo, nullptr, &sampler));

        return sampler;
    }

    VkSurfaceFormatKHR Context::selectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space) {
        uint32_t avail_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &avail_count, NULL);
        ImVector<VkSurfaceFormatKHR> avail_format;
        avail_format.resize((int)avail_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &avail_count, avail_format.Data);

        // First check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
        if (avail_count == 1) {
            if (avail_format[0].format == VK_FORMAT_UNDEFINED) {
                VkSurfaceFormatKHR ret;
                ret.format = request_formats[0];
                ret.colorSpace = request_color_space;
                return ret;
            } else {
                // No point in searching another format
                return avail_format[0];
            }
        } else {
            // Request several formats, the first found will be used
            for (int request_i = 0; request_i < request_formats_count; request_i++)
                for (uint32_t avail_i = 0; avail_i < avail_count; avail_i++)
                    if (avail_format[avail_i].format == request_formats[request_i] && avail_format[avail_i].colorSpace == request_color_space)
                        return avail_format[avail_i];

            // If none of the requested image formats could be found, use the first available
            return avail_format[0];
        }
    }

    VkPresentModeKHR Context::selectPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkPresentModeKHR* request_modes, int request_modes_count) {
        // Request a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is mandatory
        uint32_t avail_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &avail_count, NULL);
        ImVector<VkPresentModeKHR> avail_modes;
        avail_modes.resize((int)avail_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &avail_count, avail_modes.Data);

        for (int request_i = 0; request_i < request_modes_count; request_i++)
            for (uint32_t avail_i = 0; avail_i < avail_count; avail_i++)
                if (request_modes[request_i] == avail_modes[avail_i])
                    return request_modes[request_i];

        return VK_PRESENT_MODE_FIFO_KHR; // Always available
    }

    int Context::getMinImageCountFromPresentMode(VkPresentModeKHR present_mode) {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return 3;
        if (present_mode == VK_PRESENT_MODE_FIFO_KHR || present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            return 2;
        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return 1;
        IM_ASSERT(0);
        return 1;
    }
}
