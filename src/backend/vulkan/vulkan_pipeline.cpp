#include "common.h"
#include "vulkan_pipeline.h"

namespace backend::vulkan {
    Pipeline::Pipeline(uint32_t hash, VkDevice device, VkPipeline pipeline, std::shared_ptr<Shader> shader)
        : m_hash(hash)
        , m_device(device)
        , m_pipeline(pipeline)
        , m_shader(shader) {
    }

    Pipeline::~Pipeline() {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }

    PipelineBuilder::PipelineBuilder(VkDevice device)
        : m_device(device)
        , m_inputAssemblyInfo{}
        , m_viewportInfo{}
        , m_rasterizationInfo{}
        , m_multisamplingInfo{}
        , m_depthStencilInfo{} {

        m_inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        m_inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        m_viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        m_viewportInfo.viewportCount = 1;
        m_viewportInfo.scissorCount = 1;

        m_rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        m_rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        m_rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        m_rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        m_rasterizationInfo.lineWidth = 1.0f;

        m_multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        m_multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        m_depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    }

    PipelineBuilder& PipelineBuilder::SetPipelineCache(VkPipelineCache cache) {
        m_pipelineCache = cache;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetShader(std::shared_ptr<Shader> shader) {
        m_shader = shader;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetRenderPass(RenderPass& renderPass) {
        m_renderPass = &renderPass;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetSubpass(uint32_t subpass) {
        m_subpass = subpass;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::AddVertexInputBinding(VkVertexInputBindingDescription const& bindingDescription) {
        m_vertexInputBindingDescriptions.push_back(bindingDescription);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::AddVertexAttribute(VkVertexInputAttributeDescription const& attributeDescription) {
        m_vertexInputAttributeDescriptions.push_back(attributeDescription);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetTopology(VkPrimitiveTopology topology) {
        m_inputAssemblyInfo.topology = topology;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetPolygonMode(VkPolygonMode mode) {
        m_rasterizationInfo.polygonMode = mode;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetCullMode(VkCullModeFlagBits flags) {
        m_rasterizationInfo.cullMode = flags;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetFrontFace(VkFrontFace frontFace) {
        m_rasterizationInfo.frontFace = frontFace;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetLineWidth(float lineWidth) {
        m_rasterizationInfo.lineWidth = lineWidth;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::AddColorBlendAttachment(VkPipelineColorBlendAttachmentState const& attachment) {
        m_colorBlendStates.push_back(attachment);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::AddDynamicState(VkDynamicState const& state) {
        m_dynamicStates.push_back(state);
        return *this;
    }

    std::unique_ptr<Pipeline> PipelineBuilder::Build() const {
        assert(m_renderPass);

        uint32_t hash = CalculateHash();

        VkPipeline pipeline;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = m_vertexInputBindingDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = m_vertexInputAttributeDescriptions.data();

        VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
        colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingInfo.attachmentCount = static_cast<uint32_t>(m_colorBlendStates.size());
        colorBlendingInfo.pAttachments = m_colorBlendStates.data();

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
        dynamicStateInfo.pDynamicStates = m_dynamicStates.data();

        VkPipelineLayout pipelineLayout{};
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
        if (m_shader) {
            pipelineLayout = m_shader->m_pipelineLayout;
            for (auto& stage : m_shader->m_stages) {
                VkPipelineShaderStageCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                info.stage = stage.m_stageFlags;
                info.module = stage.m_module;
                info.pName = stage.m_entryPoint.c_str();
                shaderStageInfos.push_back(info);
            }
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
        pipelineInfo.pStages = shaderStageInfos.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &m_inputAssemblyInfo;
        pipelineInfo.pViewportState = &m_viewportInfo;
        pipelineInfo.pRasterizationState = &m_rasterizationInfo;
        pipelineInfo.pMultisampleState = &m_multisamplingInfo;
        pipelineInfo.pDepthStencilState = &m_depthStencilInfo;
        pipelineInfo.pColorBlendState = &colorBlendingInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = m_renderPass->GetRenderPass();
        pipelineInfo.subpass = m_subpass;

        CHECK_VK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &pipeline));

        return std::make_unique<Pipeline>(hash, m_device, pipeline, m_shader);
    }

    uint32_t PipelineBuilder::CalculateHash() const {
        std::vector<uint32_t> hashes;
        hashes.push_back(m_renderPass->GetHash());
        hashes.push_back(m_subpass);
        hashes.push_back(CalcHash(m_inputAssemblyInfo.topology));
        hashes.push_back(CalcHash(m_rasterizationInfo.polygonMode));
        hashes.push_back(CalcHash(m_rasterizationInfo.cullMode));
        hashes.push_back(CalcHash(m_rasterizationInfo.frontFace));
        hashes.push_back(CalcHash(m_rasterizationInfo.lineWidth));
        for (auto& colorBlend : m_colorBlendStates) {
            hashes.push_back(CalcHash(colorBlend));
        }
        for (auto& dynamicState : m_dynamicStates) {
            hashes.push_back(CalcHash(dynamicState));
        }
        return CalcHash(reinterpret_cast<void const*>(hashes.data()), sizeof(uint32_t) * hashes.size());
    }
}
