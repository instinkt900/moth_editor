#include "common.h"
#include "vulkan_renderpass.h"

#include "vulkan_utils.h"

namespace backend::vulkan {
    RenderPass::RenderPass(uint32_t hash, VkDevice device, VkRenderPass renderPass)
        : m_hash(hash)
        , m_device(device)
        , m_renderPass(renderPass) {
    }

    RenderPass ::~RenderPass() {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    }

    RenderPassBuilder::RenderPassBuilder(VkDevice device)
        : m_device(device) {
    }

    RenderPassBuilder& RenderPassBuilder::AddAttachment(VkAttachmentDescription const& attachment) {
        m_attachments.push_back(attachment);
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddSubpass(VkSubpassDescription const& subpass) {
        m_subpasses.push_back(subpass);
        return *this;
    }

    RenderPassBuilder& RenderPassBuilder::AddDependency(VkSubpassDependency const& dependency) {
        m_dependencies.push_back(dependency);
        return *this;
    }

    std::unique_ptr<RenderPass> RenderPassBuilder::Build() {
        uint32_t hash = CalculateHash();

        VkRenderPass renderPass;
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
        renderPassInfo.pAttachments = m_attachments.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(m_subpasses.size());
        renderPassInfo.pSubpasses = m_subpasses.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(m_dependencies.size());
        renderPassInfo.pDependencies = m_dependencies.data();
        CHECK_VK_RESULT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass));

        return std::make_unique<RenderPass>(hash, m_device, renderPass);
    }

    uint32_t RenderPassBuilder::CalculateHash() const {
        std::vector<uint32_t> hashes;
        for (auto& attachment : m_attachments) {
            hashes.push_back(CalcHash(attachment));
        }
        for (auto& subPass : m_subpasses) {
            hashes.push_back(CalcHash(subPass.flags));
            hashes.push_back(CalcHash(subPass.pipelineBindPoint));
            for (uint32_t i = 0; i < subPass.inputAttachmentCount; ++i) {
                hashes.push_back(CalcHash(subPass.pInputAttachments[i]));
            }
            for (uint32_t i = 0; i < subPass.colorAttachmentCount; ++i) {
                hashes.push_back(CalcHash(subPass.pColorAttachments[i]));
                if (subPass.pResolveAttachments) {
                    hashes.push_back(CalcHash(subPass.pResolveAttachments[i]));
                }
            }
            if (subPass.pDepthStencilAttachment) {
                hashes.push_back(CalcHash(*subPass.pDepthStencilAttachment));
            }
            for (uint32_t i = 0; i < subPass.preserveAttachmentCount; ++i) {
                hashes.push_back(CalcHash(subPass.pPreserveAttachments[i]));
            }
        }
        return CalcHash(reinterpret_cast<void const*>(hashes.data()), sizeof(uint32_t) * hashes.size());
    }
}
