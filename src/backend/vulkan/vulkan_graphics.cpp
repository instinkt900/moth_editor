#include "common.h"
#include "vulkan_graphics.h"
#include "vulkan_command_buffer.h"
#include "vulkan_subimage.h"

namespace backend::vulkan {
    VkVertexInputBindingDescription getVertexBindingDescription() {
        VkVertexInputBindingDescription vertexBindingDesc{};

        vertexBindingDesc.binding = 0;
        vertexBindingDesc.stride = sizeof(Graphics::Vertex);
        vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertexBindingDesc;
    }

    std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescs(3);

        vertexAttributeDescs[0].binding = 0;
        vertexAttributeDescs[0].location = 0;
        vertexAttributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributeDescs[0].offset = offsetof(Graphics::Vertex, xy);

        vertexAttributeDescs[1].binding = 0;
        vertexAttributeDescs[1].location = 1;
        vertexAttributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributeDescs[1].offset = offsetof(Graphics::Vertex, uv);

        vertexAttributeDescs[2].binding = 0;
        vertexAttributeDescs[2].location = 2;
        vertexAttributeDescs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vertexAttributeDescs[2].offset = offsetof(Graphics::Vertex, color);

        return vertexAttributeDescs;
    }

    Graphics::Graphics(Context& context, VkSurfaceKHR surface, uint32_t surfaceWidth, uint32_t surfaceHeight)
        : m_context(context) {
        createRenderPass();
        createPipeline();
        createDefaultImage();

        VkPipelineCacheCreateInfo cacheInfo{};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        CHECK_VK_RESULT(vkCreatePipelineCache(m_context.m_vkDevice, &cacheInfo, nullptr, &m_vkPipelineCache));

        m_swapchain = std::make_unique<Swapchain>(m_context, *m_renderPass, surface, VkExtent2D{ surfaceWidth, surfaceHeight });
    }

    Graphics::~Graphics() {
        //for (auto& [hash, pipeline] : m_pipelines) {
        //    vkDestroyPipeline(m_context.m_vkDevice, pipeline->m_pipeline, nullptr);
        //}
        vkDestroyPipelineCache(m_context.m_vkDevice, m_vkPipelineCache, nullptr);
    }

    void Graphics::BeginContext(Framebuffer* target) {
        auto& context = m_drawStack.emplace();

        if (target) {
            context.m_target = target;
            context.m_targetExtent = target->GetVkExtent();
        } else {
            context.m_target = m_swapchain->GetNextFramebuffer();
            context.m_targetExtent = m_swapchain->GetExtent();
            context.m_swapchain = true;
            VkFence cmdFence = context.m_target->GetFence().GetVkFence();
            vkWaitForFences(m_context.m_vkDevice, 1, &cmdFence, VK_TRUE, UINT64_MAX);
            vkResetFences(m_context.m_vkDevice, 1, &cmdFence);
        }

        auto& commandBuffer = context.m_target->GetCommandBuffer();
        commandBuffer.Reset();
        commandBuffer.BeginRecord();

        if (!context.m_swapchain) {
            //commandBuffer.TransitionImageLayout(context.m_target->GetVkImage(), context.m_target->GetVkFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            commandBuffer.BeginRenderPass(*m_renderPass2, *context.m_target);
        } else {
            commandBuffer.BeginRenderPass(*m_renderPass, *context.m_target);
        }

        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(context.m_targetExtent.width);
        viewport.height = static_cast<float>(context.m_targetExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.SetViewport(viewport);

        VkRect2D scissor;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = context.m_target->GetDimensions().x;
        scissor.extent.height = context.m_target->GetDimensions().y;
        commandBuffer.SetScissor(scissor);

        context.m_currentColor = moth_ui::BasicColors::Black;
        context.m_currentBlendMode = EBlendMode::None;
    }

    Framebuffer* Graphics::EndContext() {
        auto context = m_drawStack.top();
        m_drawStack.pop();

        VkDeviceSize const vertexBufferSize = context.m_vertexList.size() * sizeof(Vertex);

        if ((m_stagingBuffer == nullptr || vertexBufferSize > m_vertexBuffer->GetSize()) && vertexBufferSize > 0) {
            m_stagingBuffer = std::make_unique<Buffer>(m_context, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            m_vertexBuffer = std::make_unique<Buffer>(m_context, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

        if (m_vertexBuffer) {
            void* data = m_stagingBuffer->Map();
            memcpy(data, context.m_vertexList.data(), static_cast<size_t>(vertexBufferSize));
            m_stagingBuffer->Unmap();
            m_vertexBuffer->Copy(*m_stagingBuffer);
        }

        auto& commandBuffer = context.m_target->GetCommandBuffer();

        if (m_vertexBuffer) {
            commandBuffer.BindVertexBuffer(*m_vertexBuffer);
        }

        PushConstants pushConstants;
        pushConstants.xyScale = { 2.0f / static_cast<float>(context.m_targetExtent.width), 2.0f / static_cast<float>(context.m_targetExtent.height) };
        pushConstants.xyOffset = { -1.0f, -1.0f };
        commandBuffer.PushConstants(*m_drawingShader, VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstants), &pushConstants);

        uint32_t currentPipeline = 0;
        uint32_t vertexOffset = 0;
        for (auto& draw : context.m_draws) {
            if (draw.pipeline != currentPipeline) {
                auto const it = m_pipelines.find(draw.pipeline);
                commandBuffer.BindPipeline(*it->second);
                currentPipeline = draw.pipeline;
            }
            if (draw.descriptorSet != VK_NULL_HANDLE) {
                commandBuffer.BindDescriptorSet(*m_drawingShader, draw.descriptorSet);
            } else {
                VkDescriptorSet descriptorSet = m_drawingShader->GetDescriptorSet(*m_defaultImage);
                commandBuffer.BindDescriptorSet(*m_drawingShader, descriptorSet);
            }
            commandBuffer.Draw(draw.vertCount, vertexOffset);
            vertexOffset += draw.vertCount;
        }

        commandBuffer.EndRenderPass();

        if (context.m_swapchain) {
            commandBuffer.EndRecord();
            commandBuffer.Submit(context.m_target->GetFence().GetVkFence(), context.m_target->GetAvailableSemaphore(), context.m_target->GetRenderFinishedSemaphore());
        } else {
            //commandBuffer.TransitionImageLayout(context.m_target->GetVkImage(), context.m_target->GetVkFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            commandBuffer.EndRecord();
            //vkResetFences(m_context.m_vkDevice, 1, &cmdFence);
            VkFence cmdFence = context.m_target->GetFence().GetVkFence();
            vkResetFences(m_context.m_vkDevice, 1, &cmdFence);
            commandBuffer.Submit(cmdFence);
            vkWaitForFences(m_context.m_vkDevice, 1, &cmdFence, VK_TRUE, UINT64_MAX);
            //vkResetFences(m_context.m_vkDevice, 1, &cmdFence);
        }

        return context.m_target;
    }

    void Graphics::Begin() {
        BeginContext();
    }

    void Graphics::End() {
        Framebuffer* target = EndContext();

        VkSemaphore waitSemaphores[] = { target->GetRenderFinishedSemaphore() };
        VkSwapchainKHR swapChains[] = { m_swapchain->GetVkSwapchain() };
        uint32_t swapchainIndices[] = { target->GetSwapchainIndex() };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = waitSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = swapchainIndices;
        vkQueuePresentKHR(m_context.m_vkQueue, &presentInfo);
    }

    void Graphics::SetBlendMode(EBlendMode mode) {
        auto& context = m_drawStack.top();
        context.m_currentBlendMode = mode;
    }

    void Graphics::SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) {
    }

    void Graphics::SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) {
    }

    void Graphics::SetColor(moth_ui::Color const& color) {
        auto& context = m_drawStack.top();
        context.m_currentColor = color;
    }

    void Graphics::Clear() {
        auto& context = m_drawStack.top();
        DrawFillRectF({ { 0, 0 }, { static_cast<float>(context.m_targetExtent.width), static_cast<float>(context.m_targetExtent.height) } });
    }

    void Graphics::DrawImage(moth_ui::IImage& image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) {
        auto& context = m_drawStack.top();

        auto& vulkanImage = dynamic_cast<SubImage&>(image);
        auto texture = vulkanImage.m_texture;

        moth_ui::FloatRect const targetRect = moth_ui::MakeRect(0.0f, 0.0f, static_cast<float>(context.m_targetExtent.width), static_cast<float>(context.m_targetExtent.height));

        moth_ui::FloatRect fDestRect;
        if (destRect) {
            fDestRect = static_cast<moth_ui::FloatRect>(*destRect);
        } else {
            fDestRect = targetRect;
        }

        moth_ui::FloatRect imageRect;
        if (sourceRect) {
            imageRect = static_cast<moth_ui::FloatRect>(*sourceRect);
        } else {
            imageRect = moth_ui::MakeRect(0.0f, 0.0f, static_cast<float>(image.GetWidth()), static_cast<float>(image.GetHeight()));
        }

        moth_ui::FloatVec2 textureDimensions = moth_ui::FloatVec2{ texture->GetVkExtent().width, texture->GetVkExtent().height };
        imageRect += static_cast<moth_ui::FloatVec2>(vulkanImage.m_sourceRect.topLeft);
        imageRect /= textureDimensions;

        Vertex vertices[6];

        vertices[0].xy = { fDestRect.topLeft.x, fDestRect.topLeft.y };
        vertices[0].uv = { imageRect.topLeft.x, imageRect.topLeft.y };
        vertices[0].color = context.m_currentColor;
        vertices[1].xy = { fDestRect.bottomRight.x, fDestRect.topLeft.y };
        vertices[1].uv = { imageRect.bottomRight.x, imageRect.topLeft.y };
        vertices[1].color = context.m_currentColor;
        vertices[2].xy = { fDestRect.topLeft.x, fDestRect.bottomRight.y };
        vertices[2].uv = { imageRect.topLeft.x, imageRect.bottomRight.y };
        vertices[2].color = context.m_currentColor;

        vertices[3].xy = { fDestRect.topLeft.x, fDestRect.bottomRight.y };
        vertices[3].uv = { imageRect.topLeft.x, imageRect.bottomRight.y };
        vertices[3].color = context.m_currentColor;
        vertices[4].xy = { fDestRect.bottomRight.x, fDestRect.bottomRight.y };
        vertices[4].uv = { imageRect.bottomRight.x, imageRect.bottomRight.y };
        vertices[4].color = context.m_currentColor;
        vertices[5].xy = { fDestRect.bottomRight.x, fDestRect.topLeft.y };
        vertices[5].uv = { imageRect.bottomRight.x, imageRect.topLeft.y };
        vertices[5].color = context.m_currentColor;

        VkDescriptorSet descriptorSet = m_drawingShader->GetDescriptorSet(*texture);
        SubmitVertices(vertices, 6, ETopologyType::Triangles, descriptorSet);
    }

    void Graphics::DrawToPNG(std::filesystem::path const& path) {
    }

    void Graphics::DrawRectF(moth_ui::FloatRect const& rect) {
        auto& context = m_drawStack.top();
        Vertex vertices[8];

        vertices[0].xy = { rect.topLeft.x, rect.topLeft.y };
        vertices[0].uv = { 0, 0 };
        vertices[0].color = context.m_currentColor;
        vertices[1].xy = { rect.bottomRight.x, rect.topLeft.y };
        vertices[1].uv = { 0, 0 };
        vertices[1].color = context.m_currentColor;

        vertices[2].xy = { rect.bottomRight.x, rect.topLeft.y };
        vertices[2].uv = { 0, 0 };
        vertices[2].color = context.m_currentColor;
        vertices[3].xy = { rect.bottomRight.x, rect.bottomRight.y };
        vertices[3].uv = { 0, 0 };
        vertices[3].color = context.m_currentColor;

        vertices[4].xy = { rect.bottomRight.x, rect.bottomRight.y };
        vertices[4].uv = { 0, 0 };
        vertices[4].color = context.m_currentColor;
        vertices[5].xy = { rect.topLeft.x, rect.bottomRight.y };
        vertices[5].uv = { 0, 0 };
        vertices[5].color = context.m_currentColor;

        vertices[6].xy = { rect.topLeft.x, rect.bottomRight.y };
        vertices[6].uv = { 0, 0 };
        vertices[6].color = context.m_currentColor;
        vertices[7].xy = { rect.topLeft.x, rect.topLeft.y };
        vertices[7].uv = { 0, 0 };
        vertices[7].color = context.m_currentColor;

        SubmitVertices(vertices, 8, ETopologyType::Lines);
    }

    void Graphics::DrawFillRectF(moth_ui::FloatRect const& rect) {
        auto& context = m_drawStack.top();
        Vertex vertices[6];

        vertices[0].xy = { rect.topLeft.x, rect.topLeft.y };
        vertices[0].uv = { 0, 0 };
        vertices[0].color = context.m_currentColor;
        vertices[1].xy = { rect.bottomRight.x, rect.topLeft.y };
        vertices[1].uv = { 0, 0 };
        vertices[1].color = context.m_currentColor;
        vertices[2].xy = { rect.topLeft.x, rect.bottomRight.y };
        vertices[2].uv = { 0, 0 };
        vertices[2].color = context.m_currentColor;

        vertices[3].xy = { rect.topLeft.x, rect.bottomRight.y };
        vertices[3].uv = { 0, 0 };
        vertices[3].color = context.m_currentColor;
        vertices[4].xy = { rect.bottomRight.x, rect.bottomRight.y };
        vertices[4].uv = { 0, 0 };
        vertices[4].color = context.m_currentColor;
        vertices[5].xy = { rect.bottomRight.x, rect.topLeft.y };
        vertices[5].uv = { 0, 0 };
        vertices[5].color = context.m_currentColor;

        SubmitVertices(vertices, 6, ETopologyType::Triangles);
    }

    void Graphics::DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) {
        auto& context = m_drawStack.top();
        Vertex vertices[2];

        vertices[0].xy = { p0.x, p0.y };
        vertices[0].uv = { 0, 0 };
        vertices[0].color = context.m_currentColor;
        vertices[1].xy = { p1.x, p1.y };
        vertices[1].uv = { 0, 0 };
        vertices[1].color = context.m_currentColor;

        SubmitVertices(vertices, 2, ETopologyType::Lines);
    }

    std::unique_ptr<moth_ui::ITarget> Graphics::CreateTarget(int width, int height) {
        return std::make_unique<Framebuffer>(m_context, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_renderPass2->GetRenderPass());
    }

    moth_ui::ITarget* Graphics::GetTarget() {
        if (!m_drawStack.empty()) {
            auto& context = m_drawStack.top();
            return context.m_target;
        }
        return nullptr;
    }

    void Graphics::SetTarget(moth_ui::ITarget* target) {
        if (!m_drawStack.empty()) {
            auto& context = m_drawStack.top();
            if (context.m_target && !context.m_swapchain) {
                EndContext();
            }
        }

        if (target) {
            BeginContext(static_cast<Framebuffer*>(target));
        }
    }

    void Graphics::SetLogicalSize(moth_ui::IntVec2 const& logicalSize) {
        auto& context = m_drawStack.top();
        context.m_targetExtent = { static_cast<uint32_t>(logicalSize.x), static_cast<uint32_t>(logicalSize.y) };
    }

    bool readFile(std::string const& filename, std::vector<char>& outBuffer) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            spdlog::error("Failed to readFile {}", filename);
            return false;
        }

        size_t const fileSize = static_cast<size_t>(file.tellg());
        outBuffer.resize(fileSize);

        file.seekg(0);
        file.read(outBuffer.data(), fileSize);
        file.close();

        return true;
    }

    void Graphics::createPipeline() {
        std::vector<char> vertShaderCode;
        std::vector<char> fragShaderCode;
        readFile("resources/drawing_vert.spv", vertShaderCode);
        readFile("resources/drawing_frag.spv", fragShaderCode);

        m_drawingShader = ShaderBuilder(m_context.m_vkDevice, m_context.m_vkDescriptorPool)
                              .AddPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants))
                              .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
                              .AddStage(VK_SHADER_STAGE_VERTEX_BIT, "main", vertShaderCode.data(), vertShaderCode.size())
                              .AddStage(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragShaderCode.data(), fragShaderCode.size())
                              .Build();
    }

    void Graphics::createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        //colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB; // TODO this might have to change?
        colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        m_renderPass = RenderPassBuilder(m_context.m_vkDevice)
                           .AddAttachment(colorAttachment)
                           .AddSubpass(subpass)
                           .AddDependency(dependency)
                           .Build();

        VkAttachmentDescription colorAttachment2{};
        //colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB; // TODO this might have to change?
        colorAttachment2.format = VK_FORMAT_B8G8R8A8_UNORM;
        colorAttachment2.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment2.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment2.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment2.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment2.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment2.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        m_renderPass2 = RenderPassBuilder(m_context.m_vkDevice)
                            .AddAttachment(colorAttachment2)
                            .AddSubpass(subpass)
                            .AddDependency(dependency)
                            .Build();
    }

    void Graphics::createDefaultImage() {
        auto stagingBuffer = std::make_unique<Buffer>(m_context, 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        unsigned char const pixel[] = { 0xFF, 0xFF, 0xFF, 0xFF };
        void* data = stagingBuffer->Map();
        memcpy(data, pixel, 4);
        stagingBuffer->Unmap();

        const VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

        m_defaultImage = std::make_unique<Image>(m_context, 1, 1, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        auto commandBuffer = std::make_unique<CommandBuffer>(m_context);
        commandBuffer->BeginRecord();
        commandBuffer->TransitionImageLayout(*m_defaultImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        commandBuffer->CopyBufferToImage(*m_defaultImage, *stagingBuffer);
        commandBuffer->TransitionImageLayout(*m_defaultImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        commandBuffer->SubmitAndWait();
    }

    VkPrimitiveTopology Graphics::ToVulkan(ETopologyType type) const {
        switch (type) {
        default:
            assert(false);
        case ETopologyType::Lines:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case ETopologyType::Triangles:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkPipelineColorBlendAttachmentState Graphics::ToVulkan(EBlendMode mode) const {
        VkPipelineColorBlendAttachmentState currentBlend{};
        switch (mode) {
        default:
        case EBlendMode::None:
            currentBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            currentBlend.blendEnable = VK_FALSE;
            currentBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            currentBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            currentBlend.colorBlendOp = VK_BLEND_OP_ADD;
            currentBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            currentBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            currentBlend.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case EBlendMode::Add:
            currentBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            currentBlend.blendEnable = VK_TRUE;
            currentBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            currentBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            currentBlend.colorBlendOp = VK_BLEND_OP_ADD;
            currentBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            currentBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            currentBlend.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case EBlendMode::Blend:
            currentBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            currentBlend.blendEnable = VK_TRUE;
            currentBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            currentBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            currentBlend.colorBlendOp = VK_BLEND_OP_ADD;
            currentBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            currentBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            currentBlend.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case EBlendMode::Mod:
            currentBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            currentBlend.blendEnable = VK_TRUE;
            currentBlend.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            currentBlend.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
            currentBlend.colorBlendOp = VK_BLEND_OP_ADD;
            currentBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            currentBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            currentBlend.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case EBlendMode::Mul:
            currentBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            currentBlend.blendEnable = VK_TRUE;
            currentBlend.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            currentBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            currentBlend.colorBlendOp = VK_BLEND_OP_ADD;
            currentBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            currentBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            currentBlend.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        }
        return currentBlend;
    }

    Pipeline& Graphics::GetCurrentPipeline(ETopologyType topology) {
        auto& context = m_drawStack.top();
        auto const vkTopology = ToVulkan(topology);
        auto const blendAttachment = ToVulkan(context.m_currentBlendMode);
        auto const vertexInputBinding = getVertexBindingDescription();
        auto const vertexAttributeBindings = getVertexAttributeDescriptions();

        auto const builder = PipelineBuilder(m_context.m_vkDevice)
                                 .SetPipelineCache(m_vkPipelineCache)
                                 .SetRenderPass(*m_renderPass)
                                 .SetShader(m_drawingShader)
                                 .AddVertexInputBinding(vertexInputBinding)
                                 .AddVertexAttribute(vertexAttributeBindings[0])
                                 .AddVertexAttribute(vertexAttributeBindings[1])
                                 .AddVertexAttribute(vertexAttributeBindings[2])
                                 .AddColorBlendAttachment(blendAttachment)
                                 .AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
                                 .AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
                                 .SetTopology(vkTopology);

        uint32_t const pipelineHash = builder.CalculateHash();
        auto it = m_pipelines.find(pipelineHash);
        if (std::end(m_pipelines) == it) {
            auto const insertResult = m_pipelines.insert(std::make_pair(pipelineHash, builder.Build()));
            it = insertResult.first;
        }

        return *it->second;
    }

    void Graphics::SubmitVertices(Vertex* vertices, uint32_t vertCount, ETopologyType topology, VkDescriptorSet descriptorSet) {
        auto& context = m_drawStack.top();

        for (uint32_t i = 0; i < vertCount; ++i) {
            context.m_vertexList.push_back(vertices[i]);
        }

        auto const& pipeline = GetCurrentPipeline(topology);
        context.m_draws.push_back({ pipeline.m_hash, vertCount, descriptorSet });
    }
}
