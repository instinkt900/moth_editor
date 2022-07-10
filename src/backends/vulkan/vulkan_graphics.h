#pragma once

#include "../igraphics.h"

namespace backend::vulkan {
    class VulkanGraphics : public IGraphics {
    public:
        VulkanGraphics();

        void SetBlendMode(EBlendMode mode) override;
        void SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) override;
        void SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) override;
        void SetColor(moth_ui::Color const& color) override;
        void Clear() override;
        void DrawImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) override;
        void DrawToPNG(std::filesystem::path const& path) override;
        void DrawRectF(moth_ui::FloatRect const& rect) override;
        void DrawFillRectF(moth_ui::FloatRect const& rect) override;
        void DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) override;

        std::unique_ptr<moth_ui::IImage> CreateTarget(int width, int height) override;
        std::shared_ptr<moth_ui::IImage> GetTarget() override;
        void SetTarget(std::shared_ptr<moth_ui::IImage> target) override;

        void SetLogicalSize(moth_ui::IntVec2 const& logicalSize) override;

    private:
    };
}