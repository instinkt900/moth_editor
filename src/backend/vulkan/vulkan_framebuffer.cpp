#include "common.h"
#include "vulkan_framebuffer.h"
#include "vulkan_utils.h"
#include "vulkan_command_buffer.h"

namespace backend::vulkan {
    Framebuffer::Framebuffer(Context& context, uint32_t width, uint32_t height, VkImage image, VkImageView view, VkFormat format, VkRenderPass renderPass, uint32_t swapchainIndex)
        : m_context(context)
        , m_swapchainIndex(swapchainIndex) {
        m_commandBuffer = std::make_unique<CommandBuffer>(context);
        m_fence = std::make_unique<Fence>(context);

        auto imagePtr = std::make_unique<Image>(context, image, view, VkExtent2D{ width, height }, format, false);
        moth_ui::IntVec2 dim = { width, height };
        moth_ui::IntRect rec = { { 0, 0 }, { width, height } };
        m_image = std::make_unique<SubImage>(std::move(imagePtr), dim, rec);
        CreateFramebufferResource(context, renderPass);
    }

    Framebuffer::Framebuffer(Context& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkRenderPass renderPass)
        : m_context(context){
        m_commandBuffer = std::make_unique<CommandBuffer>(context);
        m_fence = std::make_unique<Fence>(context);

        auto image = std::make_unique<Image>(context, width, height, format, tiling, usage);
        moth_ui::IntVec2 dim = { width, height };
        moth_ui::IntRect rec = { { 0, 0 }, { width, height } };
        m_image = std::make_unique<SubImage>(std::move(image), dim, rec);
        CreateFramebufferResource(context, renderPass);
    }

    Framebuffer::~Framebuffer() {
        vkDestroySemaphore(m_context.m_vkDevice, m_imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(m_context.m_vkDevice, m_renderFinishedSemaphore, nullptr);
        vkDestroyFramebuffer(m_context.m_vkDevice, m_vkFramebuffer, nullptr);
    }

    moth_ui::IntVec2 Framebuffer::GetDimensions() const {
        return { m_image->m_texture->GetVkExtent().width, m_image->m_texture->GetVkExtent().height };
    }

    VkExtent2D Framebuffer::GetVkExtent() const {
        return m_image->m_texture->GetVkExtent();
    }

    VkFormat Framebuffer::GetVkFormat() const {
        return m_image->m_texture->GetVkFormat();
    }

    Image& Framebuffer::GetVkImage() {
        return *m_image->m_texture;
    }

    void Framebuffer::CreateFramebufferResource(Context& context, VkRenderPass renderPass) {
        VkImageView attachments[] = { m_image->m_texture->GetVkView() };
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.pNext = nullptr;
        info.renderPass = renderPass;
        info.pAttachments = attachments;
        info.attachmentCount = 1;
        info.width = m_image->GetWidth();
        info.height = m_image->GetHeight();
        info.layers = 1;
        CHECK_VK_RESULT(vkCreateFramebuffer(context.m_vkDevice, &info, nullptr, &m_vkFramebuffer));

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        CHECK_VK_RESULT(vkCreateSemaphore(m_context.m_vkDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore));
        CHECK_VK_RESULT(vkCreateSemaphore(m_context.m_vkDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore));
    }
}
