#pragma once

#include "vulkan_context.h"
#include "vulkan_framebuffer.h"
#include "vulkan_renderpass.h"

namespace backend::vulkan {
    class Swapchain {
    public:
        Swapchain(Context& context, RenderPass& renderPass, VkSurfaceKHR surface, VkExtent2D extent);
        ~Swapchain();

        VkExtent2D GetExtent() const { return m_extent; }

        Framebuffer* GetNextFramebuffer();

        VkSwapchainKHR GetVkSwapchain() const { return m_vkSwapchain; }

        uint32_t GetImageCount() const { return m_imageCount; }

    private:
        Context& m_context;
        VkExtent2D m_extent;
        VkSwapchainKHR m_vkSwapchain;
        std::vector<std::unique_ptr<Framebuffer>> m_framebuffers;
        uint32_t m_currentFrame = 0;
        uint32_t m_imageCount = 0;
    };
}
