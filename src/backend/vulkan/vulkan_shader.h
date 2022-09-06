#pragma once

#include "vulkan_image.h"

namespace backend::vulkan {
    struct Shader {
        Shader(uint32_t hash);
        ~Shader();

        struct Stage {
            VkShaderStageFlagBits m_stageFlags;
            VkShaderModule m_module;
            std::string m_entryPoint;
        };

        uint32_t m_hash;
        VkDevice m_device;
        VkDescriptorPool m_descriptorPool;
        std::vector<Shader::Stage> m_stages;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkPipelineLayout m_pipelineLayout;

        VkDescriptorSet GetDescriptorSet(Image& image);
        VkDescriptorSet CreateDescriptorSet(Image& image);

        std::map<uint32_t, VkDescriptorSet> m_descriptorSets;
    };

    class ShaderBuilder {
    public:
        ShaderBuilder(VkDevice device, VkDescriptorPool descriptorPool);
        ShaderBuilder& AddPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
        ShaderBuilder& AddBinding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags flags);
        ShaderBuilder& AddStage(VkShaderStageFlagBits stageFlags, std::string const& entryPoint, char const* byteCode, size_t codeSize);
        std::unique_ptr<Shader> Build();

    private:
        struct Stage {
            VkShaderStageFlagBits m_stageFlags;
            std::string m_entryPoint;
            std::vector<char> m_byteCode;
        };

        VkDevice m_device;
        VkDescriptorPool m_descriptorPool;
        std::vector<Stage> m_stages;
        std::vector<VkPushConstantRange> m_pushConstants;
        std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;

        uint32_t CalculateHash() const;
    };
}
