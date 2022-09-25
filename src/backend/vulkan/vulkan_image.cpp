#include "common.h"
#include "vulkan_image.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
    static uint32_t NextTextureId = 1;
}

namespace backend::vulkan {
    std::unique_ptr<Image> Image::FromFile(Context& context, std::filesystem::path const& path) {
        if (std::filesystem::exists(path)) {
            int texWidth;
            int texHeight;
            int texChannels;
            stbi_uc* pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            VkDeviceSize imageSize = texWidth * texHeight * 4;
            if (pixels != nullptr) {
                auto stagingBuffer = std::make_unique<Buffer>(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

                void* data = stagingBuffer->Map();
                memcpy(data, pixels, static_cast<size_t>(imageSize));
                stagingBuffer->Unmap();

                stbi_image_free(pixels);

                VkFormat const format = VK_FORMAT_R8G8B8A8_SRGB;
                auto newImage = std::make_unique<Image>(context, texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

                auto commandBuffer = std::make_unique<CommandBuffer>(context);
                commandBuffer->BeginRecord();
                commandBuffer->TransitionImageLayout(*newImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                commandBuffer->CopyBufferToImage(*newImage, *stagingBuffer);
                commandBuffer->TransitionImageLayout(*newImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                commandBuffer->SubmitAndWait();

                return newImage;
            }
        }
        return nullptr;
    }

    Image::Image(Context& context)
        : m_id(NextTextureId++)
        , m_context(context) {
    }

    Image::Image(Context& context, VkImage image, VkImageView view, VkExtent2D extent, VkFormat format, bool owning)
        : m_id(NextTextureId++)
        , m_context(context)
        , m_vkExtent(extent)
        , m_vkFormat(format)
        , m_vkImage(image)
        , m_vkView(view)
        , m_owningImage(owning) {
    }

    Image::Image(Context& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, bool owning)
        : m_id(NextTextureId++)
        , m_context(context)
        , m_vkExtent{ width, height }
        , m_vkFormat(format)
        , m_owningImage(owning) {
        CreateResource(tiling, usage);
        CreateView();
        CreateDefaultSampler();
    }

    Image::~Image() {
        if (m_vkSampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_context.m_vkDevice, m_vkSampler, nullptr);
        }
        if (m_vkView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_context.m_vkDevice, m_vkView, nullptr);
        }
        if (m_owningImage && m_vkImage != VK_NULL_HANDLE) {
            vmaFreeMemory(m_context.m_vmaAllocator, m_vmaAllocation);
            vkDestroyImage(m_context.m_vkDevice, m_vkImage, nullptr);
        }
    }

    void Image::CreateResource(VkImageTiling tiling, VkImageUsageFlags usage) {
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent = { m_vkExtent.width, m_vkExtent.height, 1 };
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.format = m_vkFormat;
        info.tiling = tiling;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.flags = 0;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        CHECK_VK_RESULT(vmaCreateImage(m_context.m_vmaAllocator, &info, &allocInfo, &m_vkImage, &m_vmaAllocation, nullptr));
    }

    void Image::CreateView() {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = m_vkFormat;
        info.subresourceRange = {};
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.image = m_vkImage;
        CHECK_VK_RESULT(vkCreateImageView(m_context.m_vkDevice, &info, nullptr, &m_vkView));
    }

    void Image::CreateDefaultSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;

        // todo can probably cache this
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_context.m_vkPhysicalDevice, &properties);

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        CHECK_VK_RESULT(vkCreateSampler(m_context.m_vkDevice, &samplerInfo, nullptr, &m_vkSampler));
    }
}
