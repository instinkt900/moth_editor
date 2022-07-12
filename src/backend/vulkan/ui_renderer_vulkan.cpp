#include "common.h"
#include "ui_renderer_vulkan.h"

namespace backend::vulkan {
    UIRenderer::UIRenderer() {
    }

    void UIRenderer::PushBlendMode(moth_ui::BlendMode mode) {
    }

    void UIRenderer::PopBlendMode() {
    }

    void UIRenderer::PushColor(moth_ui::Color const& color) {
    }

    void UIRenderer::PopColor() {
    }

    void UIRenderer::PushClip(moth_ui::IntRect const& rect) {
    }

    void UIRenderer::PopClip() {
    }

    void UIRenderer::RenderRect(moth_ui::IntRect const& rect) {
    }

    void UIRenderer::RenderFilledRect(moth_ui::IntRect const& rect) {
    }

    void UIRenderer::RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& sourceRect, moth_ui::IntRect const& destRect, moth_ui::ImageScaleType scaleType, float scale) {
    }

    void UIRenderer::RenderText(std::string const& text, moth_ui::IFont& font, moth_ui::TextHorizAlignment horizontalAlignment, moth_ui::TextVertAlignment verticalAlignment, moth_ui::IntRect const& destRect) {
    }
}
