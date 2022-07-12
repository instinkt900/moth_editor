#pragma once

#include "moth_ui/iimage.h"

namespace backend {
    enum class EBlendMode {
        Invalid,
        None,
        Blend,
        Add,
        Mod,
        Mul,
    };

    class IGraphicsContext {
    public:
        virtual void SetBlendMode(EBlendMode mode) = 0;
        virtual void SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) = 0;
        virtual void SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) = 0;
        virtual void SetColor(moth_ui::Color const& color) = 0;
        virtual void Clear() = 0;
        virtual void DrawImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) = 0;
        virtual void DrawToPNG(std::filesystem::path const& path) = 0;
        virtual void DrawRectF(moth_ui::FloatRect const& rect) = 0;
        virtual void DrawFillRectF(moth_ui::FloatRect const& rect) = 0;
        virtual void DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) = 0;

        virtual std::unique_ptr<moth_ui::IImage> CreateTarget(int width, int height) = 0;
        virtual std::shared_ptr<moth_ui::IImage> GetTarget() = 0;
        virtual void SetTarget(std::shared_ptr<moth_ui::IImage> target) = 0;

        virtual void SetLogicalSize(moth_ui::IntVec2 const& logicalSize) = 0;
    };
}