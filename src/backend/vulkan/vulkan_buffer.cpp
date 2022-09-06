#include "common.h"
#include "vulkan_buffer.h"
#include "vulkan_utils.h"

namespace backend::vulkan {
    Buffer::Buffer(Context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        : m_context(context)
        , m_size(size) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = properties;
        if (properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }

        CHECK_VK_RESULT(vmaCreateBuffer(m_context.m_vmaAllocator, &bufferInfo, &allocInfo, &m_vkBuffer, &m_vmaAllocation, nullptr));
    }

    Buffer::~Buffer() {
        vkDestroyBuffer(m_context.m_vkDevice, m_vkBuffer, nullptr);
        vmaFreeMemory(m_context.m_vmaAllocator, m_vmaAllocation);
    }

    void* Buffer::Map() {
        void* data;
        vmaMapMemory(m_context.m_vmaAllocator, m_vmaAllocation, &data);
        return data;
    }

    void Buffer::Unmap() {
        vmaUnmapMemory(m_context.m_vmaAllocator, m_vmaAllocation);
    }

    void Buffer::Copy(Buffer& sourceBuffer) {
        VkCommandBuffer commandBuffer = m_context.beginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = m_size;
        vkCmdCopyBuffer(commandBuffer, sourceBuffer.m_vkBuffer, m_vkBuffer, 1, &copyRegion);
        m_context.endSingleTimeCommands(commandBuffer);
    }
}
