#pragma once

#include "../igraphics_context.h"
#include "vulkan_image.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"
#include "vulkan_shader.h"
#include "vulkan_pipeline.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_swapchain.h"

namespace backend::vulkan {
    class Graphics : public IGraphicsContext {
    public:
        Graphics(Context& context, VkSurfaceKHR surface, uint32_t surfaceWidth, uint32_t surfaceHeight);
        ~Graphics();

        struct Vertex {
            moth_ui::FloatVec2 xy;
            moth_ui::FloatVec2 uv;
            moth_ui::Color color;
        };

        void Begin();
        void End();

        void SetBlendMode(moth_ui::BlendMode mode) override;
        //void SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) override;
        //void SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) override;
        void SetColor(moth_ui::Color const& color) override;
        void Clear() override;
        void DrawImage(moth_ui::IImage& image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) override;
        void DrawToPNG(std::filesystem::path const& path) override;
        void DrawRectF(moth_ui::FloatRect const& rect) override;
        void DrawFillRectF(moth_ui::FloatRect const& rect) override;
        void DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) override;

        std::unique_ptr<moth_ui::ITarget> CreateTarget(int width, int height) override;
        moth_ui::ITarget* GetTarget() override;
        void SetTarget(moth_ui::ITarget* target) override;

        void SetLogicalSize(moth_ui::IntVec2 const& logicalSize) override;

        Swapchain& GetSwapchain() const { return *m_swapchain; }
        RenderPass& GetRenderPass() const { return *m_renderPass; }
        CommandBuffer* GetCurrentCommandBuffer() {
            if (!m_drawStack.empty()) {
                auto context = m_drawStack.top();
                return &context.m_target->GetCommandBuffer();
            }
            return nullptr;
        }
        VkDescriptorSet GetDescriptorSet(Image& image);

    private:
        Context& m_context;

        struct PushConstants {
            moth_ui::FloatVec2 xyScale;
            moth_ui::FloatVec2 xyOffset;
        };

        enum class ETopologyType {
            Invalid,
            Lines,
            Triangles
        };

        struct DrawCmd {
            uint32_t pipeline;
            uint32_t vertCount;
            VkDescriptorSet descriptorSet;
        };

        struct DrawContext {
            Framebuffer* m_target = nullptr;
            VkExtent2D m_targetExtent;

            moth_ui::BlendMode m_currentBlendMode = moth_ui::BlendMode::Replace;
            moth_ui::Color m_currentColor;
            std::vector<Vertex> m_vertexList;
            std::vector<DrawCmd> m_draws;

            bool m_swapchain = false;
        };

        VkPipelineCache m_vkPipelineCache;
        std::map<uint32_t, std::shared_ptr<Pipeline>> m_pipelines;
        std::unique_ptr<RenderPass> m_renderPass;
        std::unique_ptr<Swapchain> m_swapchain;
        std::unique_ptr<Buffer> m_stagingBuffer;
        std::unique_ptr<Buffer> m_vertexBuffer;
        std::shared_ptr<Shader> m_drawingShader;
        std::unique_ptr<Image> m_defaultImage;
        std::stack<DrawContext> m_drawStack;
        bool m_passBegun = false;

        VkPrimitiveTopology ToVulkan(ETopologyType type) const;
        VkPipelineColorBlendAttachmentState ToVulkan(moth_ui::BlendMode mode) const;

        void CreateRenderPass();
        void CreatePipeline();
        void CreateDefaultImage();
        Pipeline& GetCurrentPipeline(ETopologyType topology);
        void SubmitVertices(Vertex* vertices, uint32_t vertCount, ETopologyType topology, VkDescriptorSet descriptorSet = VK_NULL_HANDLE);
        void BeginContext(Framebuffer* target = nullptr);
        Framebuffer* EndContext();
    };
}