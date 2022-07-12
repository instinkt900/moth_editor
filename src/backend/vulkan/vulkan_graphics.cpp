#include "common.h"
#include "vulkan_graphics.h"

namespace backend::vulkan {
    Graphics::Graphics() {
    }

    void Graphics::SetBlendMode(EBlendMode mode) {
    }

    void Graphics::SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) {
    }

    void Graphics::SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) {
    }

    void Graphics::SetColor(moth_ui::Color const& color) {
    }

    void Graphics::Clear() {
    }

    void Graphics::DrawImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) {
    }

    void Graphics::DrawToPNG(std::filesystem::path const& path) {
    }

    void Graphics::DrawRectF(moth_ui::FloatRect const& rect) {
    }

    void Graphics::DrawFillRectF(moth_ui::FloatRect const& rect) {
    }

    void Graphics::DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) {
    }

    std::unique_ptr<moth_ui::IImage> Graphics::CreateTarget(int width, int height) {
        return nullptr;
    }

    std::shared_ptr<moth_ui::IImage> Graphics::GetTarget() {
        return nullptr;
    }

    void Graphics::SetTarget(std::shared_ptr<moth_ui::IImage> target) {
    }

    void Graphics::SetLogicalSize(moth_ui::IntVec2 const& logicalSize) {
    }
}
