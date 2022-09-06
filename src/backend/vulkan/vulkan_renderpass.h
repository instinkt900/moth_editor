#pragma once

namespace backend::vulkan {
    class RenderPass {
    public:
        RenderPass(uint32_t hash, VkDevice device, VkRenderPass renderPass);
        ~RenderPass();

        uint32_t GetHash() const { return m_hash; }
        VkDevice GetDevice() const { return m_device; }
        VkRenderPass GetRenderPass() const { return m_renderPass; }

    private:
        uint32_t m_hash;
        VkDevice m_device;
        VkRenderPass m_renderPass;
    };

    class RenderPassBuilder {
    public:
        RenderPassBuilder(VkDevice device);
        RenderPassBuilder& AddAttachment(VkAttachmentDescription const& attachment);
        RenderPassBuilder& AddSubpass(VkSubpassDescription const& subpass);
        RenderPassBuilder& AddDependency(VkSubpassDependency const& dependency);
        std::unique_ptr<RenderPass> Build();

    private:
        VkDevice m_device;
        std::vector<VkAttachmentDescription> m_attachments;
        std::vector<VkSubpassDescription> m_subpasses;
        std::vector<VkSubpassDependency> m_dependencies;

        uint32_t CalculateHash() const;
    };
}
