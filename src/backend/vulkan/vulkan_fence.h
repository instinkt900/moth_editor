#pragma once

#include "vulkan_context.h"

namespace backend::vulkan {
    class Fence {
    public:
        Fence(Context& context);
        ~Fence();

        VkFence GetVkFence() const { return m_vkFence; }

    private:
        Context& m_context;
        VkFence m_vkFence;
    };
}
