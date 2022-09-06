#include "common.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

namespace backend::vulkan {
    CommandBuffer::CommandBuffer(Context& context)
        : m_context(context) {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool = m_context.m_vkCommandPool;
        info.commandBufferCount = 1;
        CHECK_VK_RESULT(vkAllocateCommandBuffers(m_context.m_vkDevice, &info, &m_vkCommandBuffer));
    }

    CommandBuffer::~CommandBuffer() {
        vkFreeCommandBuffers(m_context.m_vkDevice, m_context.m_vkCommandPool, 1, &m_vkCommandBuffer);
    }

    void CommandBuffer::BeginRecord() {
        assert(!m_recording && "Already recording");

        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CHECK_VK_RESULT(vkBeginCommandBuffer(m_vkCommandBuffer, &info));
        m_recording = true;
    }

    void CommandBuffer::EndRecord() {
        assert(m_recording && "Not recording");

        CHECK_VK_RESULT(vkEndCommandBuffer(m_vkCommandBuffer));
        m_recording = false;
    }

    void CommandBuffer::Submit(VkFence fence, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) {
        if (m_recording) {
            EndRecord();
        }

        VkPipelineStageFlags waitStageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &m_vkCommandBuffer;

        if (waitSemaphore) {
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &waitSemaphore;
        }
        if (signalSemaphore) {
            info.signalSemaphoreCount = 1;
            info.pSignalSemaphores = &signalSemaphore;
        }
        if (signalSemaphore && waitSemaphore) {
            info.pWaitDstStageMask = waitStageFlags;
        }

        vkQueueSubmit(m_context.m_vkQueue, 1, &info, fence);
    }

    void CommandBuffer::SubmitAndWait() {
        Submit(VK_NULL_HANDLE);
        vkQueueWaitIdle(m_context.m_vkQueue);
    }

    void CommandBuffer::Reset() {
        vkResetCommandBuffer(m_vkCommandBuffer, 0);
    }

    void CommandBuffer::TransitionImageLayout(Image& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        assert(m_recording);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.GetVkImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.levelCount = 1;

        VkPipelineStageFlags source_stage = VK_PIPELINE_STAGE_NONE;
        VkPipelineStageFlags destination_stage = VK_PIPELINE_STAGE_NONE;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        vkCmdPipelineBarrier(m_vkCommandBuffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void CommandBuffer::CopyBufferToImage(Image& image, Buffer& buffer) {
        assert(m_recording);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { image.GetVkExtent().width, image.GetVkExtent().height, 1 };

        vkCmdCopyBufferToImage(m_vkCommandBuffer, buffer.GetVKBuffer(), image.GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    void CommandBuffer::BeginRenderPass(RenderPass& renderPass, Framebuffer& framebuffer) {
        VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderPass.GetRenderPass();
        info.framebuffer = framebuffer.GetVkFramebuffer();
        info.renderArea.extent.width = framebuffer.GetDimensions().x;
        info.renderArea.extent.height = framebuffer.GetDimensions().y;
        info.clearValueCount = 1;
        info.pClearValues = &clearColor;
        vkCmdBeginRenderPass(m_vkCommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void CommandBuffer::EndRenderPass() {
        vkCmdEndRenderPass(m_vkCommandBuffer);
    }

    void CommandBuffer::SetViewport(VkViewport viewport) {
        vkCmdSetViewport(m_vkCommandBuffer, 0, 1, &viewport);
    }

    void CommandBuffer::SetScissor(VkRect2D scissor) {
        vkCmdSetScissor(m_vkCommandBuffer, 0, 1, &scissor);
    }

    void CommandBuffer::BindDescriptorSet(Shader& shader, VkDescriptorSet descriptorSet) {
        vkCmdBindDescriptorSets(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader.m_pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    }

    void CommandBuffer::BindPipeline(Pipeline& pipeline) {
        vkCmdBindPipeline(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);
    }

    void CommandBuffer::PushConstants(Shader& shader, VkShaderStageFlagBits stageFlags, size_t dataSize, void const* data) {
        vkCmdPushConstants(m_vkCommandBuffer, shader.m_pipelineLayout, stageFlags, 0, static_cast<uint32_t>(dataSize), data);
    }

    void CommandBuffer::BindVertexBuffer(Buffer& buffer) {
        VkBuffer vertexBuffers[] = { buffer.GetVKBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, vertexBuffers, offsets);
    }

    void CommandBuffer::Draw(uint32_t vertexCount, uint32_t offset) {
        vkCmdDraw(m_vkCommandBuffer, vertexCount, 1, offset, 0);
    }
}
