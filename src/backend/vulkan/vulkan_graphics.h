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

        void Begin();
        void End();

        void SetBlendMode(EBlendMode mode) override;
        void SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) override;
        void SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) override;
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

        //private:
        Context& m_context;

        std::unique_ptr<RenderPass> m_renderPass;
        std::unique_ptr<RenderPass> m_renderPass2;
        std::unique_ptr<Swapchain> m_swapchain;
        std::unique_ptr<Buffer> m_stagingBuffer;
        std::unique_ptr<Buffer> m_vertexBuffer;

        std::shared_ptr<Shader> m_imageShader;
        std::shared_ptr<Shader> m_drawingShader;

        VkPipelineCache m_vkPipelineCache;

        bool m_passBegun = false;

        void createRenderPass();
        void createPipeline();

        struct PushConstants {
            moth_ui::FloatVec2 xyScale;
            moth_ui::FloatVec2 xyOffset;
        };

        //PushConstants m_pushConstants;

        struct Vertex {
            moth_ui::FloatVec2 xy;
            moth_ui::FloatVec2 uv;
            moth_ui::Color color;
        };

        struct DrawCmd {
            uint32_t pipeline;
            uint32_t vertCount;
            VkDescriptorSet descriptorSet;
        };

        enum class ETopologyType {
            Invalid,
            Lines,
            Triangles
        };

        struct DrawContext {
            Framebuffer* m_target = nullptr;
            VkExtent2D m_targetExtent;

            EBlendMode m_currentBlendMode = EBlendMode::None;
            moth_ui::Color m_currentColor;
            std::vector<Vertex> m_vertexList;
            std::vector<DrawCmd> m_draws;
        };

        std::stack<DrawContext> m_drawStack;

        VkPrimitiveTopology ToVulkan(ETopologyType type) const;
        VkPipelineColorBlendAttachmentState ToVulkan(EBlendMode mode) const;
        Pipeline& GetCurrentPipeline(ETopologyType topology);
        void SubmitVertices(Vertex* vertices, uint32_t vertCount, ETopologyType topology, VkDescriptorSet descriptorSet = VK_NULL_HANDLE);

        std::map<uint32_t, std::shared_ptr<Pipeline>> m_pipelines;

        std::unique_ptr<Image> m_defaultImage;
        void createDefaultImage();

        void BeginContext(Framebuffer* target = nullptr);
        Framebuffer* EndContext();
    };
}