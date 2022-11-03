#pragma once

namespace backend::vulkan {
    class Context {
    public:
        Context();
        ~Context();

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        VkInstance m_vkInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_vkDebugMessenger = VK_NULL_HANDLE;
        VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
        uint32_t m_vkQueueFamily = static_cast<uint32_t>(-1);
        VkDevice m_vkDevice = VK_NULL_HANDLE;
        VkQueue m_vkQueue = VK_NULL_HANDLE;
        VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
        VkCommandPool m_vkCommandPool = VK_NULL_HANDLE;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_vkDeviceProperties;

        static VkSurfaceFormatKHR selectSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
        static VkPresentModeKHR selectPresentMode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const VkPresentModeKHR* request_modes, int request_modes_count);
        static int getMinImageCountFromPresentMode(VkPresentModeKHR present_mode);
    };
}
