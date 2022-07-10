#include "common.h"
#include "ui_renderer_vulkan.h"

UIRendererVulkan::UIRendererVulkan() {
}

void UIRendererVulkan::PushBlendMode(moth_ui::BlendMode mode) {
}

void UIRendererVulkan::PopBlendMode() {
}

void UIRendererVulkan::PushColor(moth_ui::Color const& color) {
}

void UIRendererVulkan::PopColor() {
}

void UIRendererVulkan::PushClip(moth_ui::IntRect const& rect) {
}

void UIRendererVulkan::PopClip() {
}

void UIRendererVulkan::RenderRect(moth_ui::IntRect const& rect) {
}

void UIRendererVulkan::RenderFilledRect(moth_ui::IntRect const& rect) {
}

void UIRendererVulkan::RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& sourceRect, moth_ui::IntRect const& destRect, moth_ui::ImageScaleType scaleType, float scale) {
}

void UIRendererVulkan::RenderText(std::string const& text, moth_ui::IFont& font, moth_ui::TextHorizAlignment horizontalAlignment, moth_ui::TextVertAlignment verticalAlignment, moth_ui::IntRect const& destRect) {
}
