#pragma once

#include "moth_ui/itarget.h"
#include "vulkan_subimage.h"
#include "vulkan_fence.h"
#include "vulkan_renderpass.h"

namespace backend::vulkan {
    class CommandBuffer;

    class Framebuffer : public moth_ui::ITarget {
    public:
        Framebuffer(Context& context, uint32_t width, uint32_t height, VkImage image, VkImageView view, VkFormat format, VkRenderPass renderPass, uint32_t swapchainIndex);
        Framebuffer(Context& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkRenderPass renderPass);
        virtual ~Framebuffer();

        moth_ui::IntVec2 GetDimensions() const override;

        //void BeginPass(RenderPass& renderPass);
        //void EndPass();
        //void Submit();

        VkFramebuffer GetVkFramebuffer() const { return m_vkFramebuffer; }
        CommandBuffer& GetCommandBuffer() { return *m_commandBuffer; }
        Fence& GetFence() const { return *m_fence; }
        VkSemaphore GetAvailableSemaphore() const { return m_imageAvailableSemaphore; }
        VkSemaphore GetRenderFinishedSemaphore() const { return m_renderFinishedSemaphore; }
        moth_ui::IImage* GetImage() override { return m_image.get(); }
        uint32_t GetSwapchainIndex() const { return m_swapchainIndex; }
        VkExtent2D GetVkExtent() const;
        VkFormat GetVkFormat() const;
        Image& GetVkImage();

    protected:
        Context& m_context;
        VkFramebuffer m_vkFramebuffer = VK_NULL_HANDLE;
        std::unique_ptr<CommandBuffer> m_commandBuffer;
        std::unique_ptr<SubImage> m_image;

        std::unique_ptr<Fence> m_fence;
        VkSemaphore m_imageAvailableSemaphore;
        VkSemaphore m_renderFinishedSemaphore;

        uint32_t m_swapchainIndex = 0;

        void CreateFramebufferResource(Context& context, VkRenderPass renderPass);
    };
}
