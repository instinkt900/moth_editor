#include "common.h"
#include "vulkan_fence.h"

namespace backend::vulkan {
    Fence::Fence(Context& context)
        : m_context(context) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_context.m_vkDevice, &fenceInfo, nullptr, &m_vkFence);
    }

    Fence::~Fence() {
        vkDestroyFence(m_context.m_vkDevice, m_vkFence, nullptr);
    }
}
