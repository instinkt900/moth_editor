#include "common.h"
#include "vulkan_swapchain.h"
#include "vulkan_utils.h"

namespace {
    VkExtent2D chooseSwapExtent(uint32_t width, uint32_t height, const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}

namespace backend::vulkan {
    Swapchain::Swapchain(Context& context, RenderPass& renderPass, VkSurfaceKHR surface, VkExtent2D extent)
        : m_context(context)
        , m_extent(extent) {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_context.m_vkPhysicalDevice, surface, &capabilities);
        extent = chooseSwapExtent(extent.width, extent.height, capabilities);

        const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        VkSurfaceFormatKHR surfaceFormat = Context::selectSurfaceFormat(m_context.m_vkPhysicalDevice, surface, requestSurfaceImageFormat, 4, requestSurfaceColorSpace);
        VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
        VkPresentModeKHR presentMode = Context::selectPresentMode(m_context.m_vkPhysicalDevice, surface, presentModes, 3);
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = Context::getMinImageCountFromPresentMode(presentMode);
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        CHECK_VK_RESULT(vkCreateSwapchainKHR(m_context.m_vkDevice, &createInfo, nullptr, &m_vkSwapchain));

        uint32_t imageCount;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        vkGetSwapchainImagesKHR(m_context.m_vkDevice, m_vkSwapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        swapchainImageViews.resize(imageCount);
        vkGetSwapchainImagesKHR(m_context.m_vkDevice, m_vkSwapchain, &imageCount, swapchainImages.data());

        for (size_t i = 0; i < swapchainImages.size(); ++i) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = surfaceFormat.format;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            CHECK_VK_RESULT(vkCreateImageView(m_context.m_vkDevice, &createInfo, nullptr, &swapchainImageViews[i]));
        }

        for (uint32_t i = 0; i < imageCount; ++i) {
            m_framebuffers.push_back(std::make_unique<Framebuffer>(m_context, extent.width, extent.height, swapchainImages[i], swapchainImageViews[i], surfaceFormat.format, renderPass.GetRenderPass(), i));
        }

        m_imageCount = imageCount;
    }

    Swapchain::~Swapchain() {
        vkDestroySwapchainKHR(m_context.m_vkDevice, m_vkSwapchain, nullptr);
    }

    Framebuffer* Swapchain::GetNextFramebuffer() {
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_context.m_vkDevice, m_vkSwapchain, UINT64_MAX, m_framebuffers[m_currentFrame]->GetAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            //m_vkSwapChainrebuild = true;
            return nullptr;
        }
        CHECK_VK_RESULT(result);
        m_currentFrame = (m_currentFrame + 1) % m_framebuffers.size();
        return m_framebuffers[imageIndex].get();
    }
}
