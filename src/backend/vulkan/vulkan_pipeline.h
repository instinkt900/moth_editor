#pragma once

#include "vulkan_utils.h"
#include "vulkan_shader.h"
#include "vulkan_renderpass.h"

namespace backend::vulkan {
    class Pipeline {
    public:
        Pipeline(uint32_t hash, VkDevice device, VkPipeline pipeline, std::shared_ptr<Shader> shader);
        ~Pipeline();

        uint32_t m_hash;
        VkDevice m_device;
        VkPipeline m_pipeline;
        std::shared_ptr<Shader> m_shader; // need to keep this around as long as the pipeline uses them

    private:
        Pipeline(Pipeline const&) = delete;
        Pipeline& operator=(Pipeline const&) = delete;
    };

    class PipelineBuilder {
    public:
        PipelineBuilder(VkDevice device);
        PipelineBuilder& SetPipelineCache(VkPipelineCache cache);
        PipelineBuilder& SetShader(std::shared_ptr<Shader> shader);
        PipelineBuilder& SetRenderPass(RenderPass& renderPass);
        PipelineBuilder& SetSubpass(uint32_t subpass);
        PipelineBuilder& AddVertexInputBinding(VkVertexInputBindingDescription const& bindingDescription);
        PipelineBuilder& AddVertexAttribute(VkVertexInputAttributeDescription const& attributeDescription);
        PipelineBuilder& SetTopology(VkPrimitiveTopology topology);
        PipelineBuilder& SetPolygonMode(VkPolygonMode mode);
        PipelineBuilder& SetCullMode(VkCullModeFlagBits flags);
        PipelineBuilder& SetFrontFace(VkFrontFace frontFace);
        PipelineBuilder& SetLineWidth(float lineWidth);
        PipelineBuilder& AddColorBlendAttachment(VkPipelineColorBlendAttachmentState const& attachment);
        PipelineBuilder& AddDynamicState(VkDynamicState const& state);
        std::unique_ptr<Pipeline> Build() const;

        uint32_t CalculateHash() const;

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
        RenderPass* m_renderPass = nullptr;
        uint32_t m_subpass = 0;
        std::shared_ptr<Shader> m_shader;
        std::vector<VkVertexInputBindingDescription> m_vertexInputBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributeDescriptions;
        VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;
        VkPipelineViewportStateCreateInfo m_viewportInfo;
        VkPipelineRasterizationStateCreateInfo m_rasterizationInfo;
        VkPipelineMultisampleStateCreateInfo m_multisamplingInfo;
        VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo;
        std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendStates;
        std::vector<VkDynamicState> m_dynamicStates;
    };
}
