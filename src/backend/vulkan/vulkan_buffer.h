#pragma once

#include "vulkan_context.h"

namespace backend::vulkan {
    class Buffer {
    public:
        Buffer(Context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~Buffer();

        size_t GetSize() const { return m_size; }

        void* Map();
        void Unmap();

        void Copy(Buffer& sourceBuffer);

        VkBuffer GetVKBuffer() const { return m_vkBuffer; }

    private:
        Context& m_context;
        VkBuffer m_vkBuffer;
        VmaAllocation m_vmaAllocation;
        size_t m_size;
    };
}
