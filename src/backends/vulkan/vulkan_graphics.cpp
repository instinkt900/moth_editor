#include "common.h"
#include "vulkan_graphics.h"

namespace backend::vulkan {
    VulkanGraphics::VulkanGraphics() {
    }

    void VulkanGraphics::SetBlendMode(EBlendMode mode) {
    }

    void VulkanGraphics::SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) {
    }

    void VulkanGraphics::SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) {
    }

    void VulkanGraphics::SetColor(moth_ui::Color const& color) {
    }

    void VulkanGraphics::Clear() {
    }

    void VulkanGraphics::DrawImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) {
    }

    void VulkanGraphics::DrawToPNG(std::filesystem::path const& path) {
    }

    void VulkanGraphics::DrawRectF(moth_ui::FloatRect const& rect) {
    }

    void VulkanGraphics::DrawFillRectF(moth_ui::FloatRect const& rect) {
    }

    void VulkanGraphics::DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) {
    }

    std::unique_ptr<moth_ui::IImage> VulkanGraphics::CreateTarget(int width, int height) {
        return nullptr;
    }

    std::shared_ptr<moth_ui::IImage> VulkanGraphics::GetTarget() {
        return nullptr;
    }

    void VulkanGraphics::SetTarget(std::shared_ptr<moth_ui::IImage> target) {
    }

    void VulkanGraphics::SetLogicalSize(moth_ui::IntVec2 const& logicalSize) {
    }
}
