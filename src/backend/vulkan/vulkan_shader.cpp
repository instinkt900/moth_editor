#include "common.h"
#include "vulkan_shader.h"
#include "vulkan_utils.h"

namespace backend::vulkan {
    Shader::Shader(uint32_t hash)
        : m_hash(hash) {
    }

    Shader::~Shader() {
        std::vector<VkDescriptorSet> freedSets;
        for (auto& [imageId, descriptorSet] : m_descriptorSets) {
            freedSets.push_back(descriptorSet);
        }
        vkFreeDescriptorSets(m_device, m_descriptorPool, static_cast<uint32_t>(freedSets.size()), freedSets.data());

        for (auto& stage : m_stages) {
            vkDestroyShaderModule(m_device, stage.m_module, nullptr);
        }
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    }

    VkDescriptorSet Shader::GetDescriptorSet(Image& image) {
        auto it = m_descriptorSets.find(image.GetId());
        if (std::end(m_descriptorSets) != it) {
            return it->second;
        }
        return CreateDescriptorSet(image);
    }

    VkDescriptorSet Shader::CreateDescriptorSet(Image& image) {
        VkDescriptorSet descriptorSet;

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_descriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptorSetLayout;
        CHECK_VK_RESULT(vkAllocateDescriptorSets(m_device, &alloc_info, &descriptorSet));

        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = image.GetVkSampler();
        desc_image[0].imageView = image.GetVkView();
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = descriptorSet;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(m_device, 1, write_desc, 0, nullptr);

        m_descriptorSets.insert(std::make_pair(image.GetId(), descriptorSet));
        return descriptorSet;
    }


    ShaderBuilder::ShaderBuilder(VkDevice device, VkDescriptorPool descriptorPool)
        : m_device(device)
        , m_descriptorPool(descriptorPool) {
    }

    ShaderBuilder& ShaderBuilder::AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size) {
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = stageFlags;
        pushConstant.offset = offset;
        pushConstant.size = size;
        m_pushConstants.push_back(pushConstant);
        return *this;
    }

    ShaderBuilder& ShaderBuilder::AddBinding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags flags) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = type;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = flags;
        m_layoutBindings.push_back(layoutBinding);
        return *this;
    }

    ShaderBuilder& ShaderBuilder::AddStage(VkShaderStageFlagBits stageFlags, std::string const& entryPoint, char const* byteCode, size_t codeSize) {
        Stage stage;
        stage.m_stageFlags = stageFlags;
        stage.m_entryPoint = entryPoint;
        stage.m_byteCode.resize(codeSize);
        memcpy(stage.m_byteCode.data(), byteCode, codeSize);
        m_stages.push_back(stage);
        return *this;
    }

    std::unique_ptr<Shader> ShaderBuilder::Build() {
        uint32_t hash = CalculateHash();

        std::unique_ptr<Shader> newShader = std::make_unique<Shader>(hash);
        newShader->m_device = m_device;
        newShader->m_descriptorPool = m_descriptorPool;
        newShader->m_stages.reserve(m_stages.size());
        for (auto& stage : m_stages) {
            Shader::Stage newStage;
            newStage.m_stageFlags = stage.m_stageFlags;
            newStage.m_entryPoint = stage.m_entryPoint;

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = stage.m_byteCode.size();
            createInfo.pCode = reinterpret_cast<uint32_t const*>(stage.m_byteCode.data());
            CHECK_VK_RESULT(vkCreateShaderModule(m_device, &createInfo, nullptr, &newStage.m_module));

            newShader->m_stages.push_back(newStage);
        }

        VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
        setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutInfo.bindingCount = static_cast<uint32_t>(m_layoutBindings.size());
        setLayoutInfo.pBindings = m_layoutBindings.data();
        CHECK_VK_RESULT(vkCreateDescriptorSetLayout(m_device, &setLayoutInfo, nullptr, &newShader->m_descriptorSetLayout));

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &newShader->m_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstants.size());
        pipelineLayoutInfo.pPushConstantRanges = m_pushConstants.data();
        CHECK_VK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &newShader->m_pipelineLayout));

        return newShader;
    }

    uint32_t ShaderBuilder::CalculateHash() const {
        std::vector<uint32_t> hashes;
        for (auto& stage : m_stages) {
            hashes.push_back(CalcHash(stage.m_stageFlags));
            hashes.push_back(CalcHash(stage.m_entryPoint));
            hashes.push_back(CalcHash(stage.m_byteCode.data(), stage.m_byteCode.size()));
        }
        return CalcHash(reinterpret_cast<void const*>(hashes.data()), sizeof(uint32_t) * hashes.size());
    }
}
