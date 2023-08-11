#pragma once

#include "vulkan_context.h"
#include "vulkan_image.h"
#include "vulkan_buffer.h"
#include "vulkan_shader.h"
#include "vulkan_pipeline.h"
#include "vulkan_framebuffer.h"

namespace backend::vulkan {
    class CommandBuffer {
    public:
        CommandBuffer(Context& context);
        ~CommandBuffer();

        void BeginRecord();
        void EndRecord();
        void Submit(VkFence fence = VK_NULL_HANDLE, VkSemaphore waitSemaphore = VK_NULL_HANDLE, VkSemaphore signalSemaphore = VK_NULL_HANDLE);
        void SubmitAndWait();
        void Reset();

        void TransitionImageLayout(Image& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void CopyBufferToImage(Image& image, Buffer& buffer);

        void BeginRenderPass(RenderPass& renderPass, Framebuffer& frameBuffer);
        void EndRenderPass();

        void SetViewport(VkViewport viewport);
        void SetScissor(VkRect2D scissor);
        void BindDescriptorSet(Shader& shader, VkDescriptorSet descriptorSet, uint32_t index);
        void BindPipeline(Pipeline const& pipeline);
        void PushConstants(Shader& shader, VkShaderStageFlagBits stageFlags, size_t bufferSize, void const* data);
        void BindVertexBuffer(Buffer& buffer, int index);
        void BindIndexBuffer(Buffer& buffer, int index);
        void Draw(uint32_t vertexCount, uint32_t offset);
        void Draw(uint32_t vertexCount, uint32_t offset, uint32_t instanceCount, uint32_t firstInstance);
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t instanceOffset);

        VkCommandBuffer GetVkCommandBuffer() { return m_vkCommandBuffer; }

    private:
        Context& m_context;
        VkCommandBuffer m_vkCommandBuffer;
        bool m_recording = false;
    };
}
