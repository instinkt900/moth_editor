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

        struct FontRect {
            float min_x;
            float min_y;
            float max_x;
            float max_y;
        };

        struct FontGlyphInstance {
            moth_ui::FloatVec2 pos;
            uint32_t glyphIndex;
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
        void DrawText(std::string const& text, moth_ui::IFont& font, moth_ui::TextHorizAlignment horizontalAlignment, moth_ui::IntVec2 const& pos, uint32_t width) override;

        std::unique_ptr<moth_ui::ITarget> CreateTarget(int width, int height) override;
        moth_ui::ITarget* GetTarget() override;
        void SetTarget(moth_ui::ITarget* target) override;

        void SetLogicalSize(moth_ui::IntVec2 const& logicalSize) override;

        Swapchain& GetSwapchain() const { return *m_swapchain; }
        RenderPass& GetRenderPass() const { return *m_renderPass; }
        CommandBuffer* GetCurrentCommandBuffer() {
            auto context = m_contextStack.top();
            if (context) {
                return &context->m_target->GetCommandBuffer();
            }
            return nullptr;
        }
        VkDescriptorSet GetDescriptorSet(Image& image);

        Shader& GetFontShader() { return *m_fontShader; }

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

        struct DrawContext {
            Framebuffer* m_target = nullptr;
            VkExtent2D m_logicalExtent;

            moth_ui::BlendMode m_currentBlendMode = moth_ui::BlendMode::Replace;
            moth_ui::Color m_currentColor = moth_ui::BasicColors::White;

            std::unique_ptr<Buffer> m_vertexBuffer;
            Vertex* m_vertexBufferData = nullptr;

            std::unique_ptr<Buffer> m_fontInstanceBuffer;
            std::unique_ptr<Buffer> m_fontInstanceStagingBuffer;
            uint32_t m_glyphCount = 0;

            uint32_t m_vertexCount = 0;
            uint32_t m_maxVertexCount = 0;
            uint32_t m_currentPipelineId = 0;
        };

        VkPipelineCache m_vkPipelineCache;
        std::map<uint32_t, std::shared_ptr<Pipeline>> m_pipelines;
        std::map<uint32_t, std::shared_ptr<Pipeline>> m_fontPipelines;
        std::unique_ptr<RenderPass> m_renderPass;
        std::unique_ptr<RenderPass> m_rtRenderPass;
        std::unique_ptr<Swapchain> m_swapchain;
        std::shared_ptr<Shader> m_drawingShader;
        std::shared_ptr<Shader> m_fontShader;
        std::unique_ptr<Image> m_defaultImage;

        DrawContext m_defaultContext;
        DrawContext m_overrideContext;
        std::stack<DrawContext*> m_contextStack;

        VkPrimitiveTopology ToVulkan(ETopologyType type) const;
        VkPipelineColorBlendAttachmentState ToVulkan(moth_ui::BlendMode mode) const;

        void CreateRenderPass();
        void CreateShaders();
        void CreateDefaultImage();
        RenderPass& GetCurrentRenderPass();
        Pipeline& GetCurrentPipeline(ETopologyType topology);
        Pipeline& GetCurrentFontPipeline();

        void BeginContext(DrawContext* target);
        void EndContext();
        void StartCommands();
        void FlushCommands();
        void SubmitVertices(Vertex* vertices, uint32_t vertCount, ETopologyType topology, VkDescriptorSet descriptorSet = VK_NULL_HANDLE);

        bool IsRenderTarget() const;
    };
}
