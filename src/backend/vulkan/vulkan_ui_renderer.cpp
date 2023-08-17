#include "common.h"
#include "vulkan_ui_renderer.h"
#include "vulkan_font.h"

namespace backend::vulkan {
    UIRenderer::UIRenderer(Graphics& graphics)
        : m_graphics(graphics) {
    }

    void UIRenderer::PushBlendMode(moth_ui::BlendMode mode) {
        m_blendMode.push(mode);
    }

    void UIRenderer::PopBlendMode() {
        if (!m_blendMode.empty()) {
            m_blendMode.pop();
        }
    }

    void UIRenderer::PushColor(moth_ui::Color const& color) {
        m_drawColor.push(color);
    }

    void UIRenderer::PopColor() {
        if (!m_drawColor.empty()) {
            m_drawColor.pop();
        }
    }

    moth_ui::IntRect ClipRect(moth_ui::IntRect const& parentRect, moth_ui::IntRect const& childRect) {
        moth_ui::IntRect result;
        result.topLeft.x = std::max(parentRect.topLeft.x, childRect.topLeft.x);
        result.topLeft.y = std::max(parentRect.topLeft.y, childRect.topLeft.y);
        result.bottomRight.x = std::min(parentRect.bottomRight.x, childRect.bottomRight.x);
        result.bottomRight.y = std::min(parentRect.bottomRight.y, childRect.bottomRight.y);
        return result;
    }

    void UIRenderer::PushClip(moth_ui::IntRect const& rect) {
        if (m_clip.empty()) {
            m_clip.push(rect);
        } else {
            // want to clip rect within the current clip
            auto const parentRect = m_clip.top();
            auto const newRect = ClipRect(parentRect, rect);
            m_clip.push(newRect);
        }

        auto const currentRect = m_clip.top();
        m_graphics.SetClipRect(&currentRect);
    }

    void UIRenderer::PopClip() {
        if (!m_clip.empty()) {
            m_clip.pop();
        }

        if (m_clip.empty()) {
            m_graphics.SetClipRect(nullptr);
        } else {
            auto const currentRect = m_clip.top();
            m_graphics.SetClipRect(&currentRect);
        }
    }

    void UIRenderer::RenderRect(moth_ui::IntRect const& rect) {
        m_graphics.SetBlendMode(m_blendMode.top());
        m_graphics.SetColor(m_drawColor.top());
        m_graphics.DrawRectF(static_cast<moth_ui::FloatRect>(rect));
    }

    void UIRenderer::RenderFilledRect(moth_ui::IntRect const& rect) {
        m_graphics.SetBlendMode(m_blendMode.top());
        m_graphics.SetColor(m_drawColor.top());
        m_graphics.DrawFillRectF(static_cast<moth_ui::FloatRect>(rect));
    }

    void UIRenderer::RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& sourceRect, moth_ui::IntRect const& destRect, moth_ui::ImageScaleType scaleType, float scale) {
        m_graphics.SetBlendMode(m_blendMode.top());
        m_graphics.SetColor(m_drawColor.top());
        m_graphics.DrawImage(image, &sourceRect, &destRect);
    }

    void UIRenderer::RenderText(std::string const& text, moth_ui::IFont& font, moth_ui::TextHorizAlignment horizontalAlignment, moth_ui::TextVertAlignment verticalAlignment, moth_ui::IntRect const& destRect) {
        auto const& vFont = static_cast<Font&>(font);

        auto const destWidth = destRect.bottomRight.x - destRect.topLeft.x;
        auto const destHeight = destRect.bottomRight.y - destRect.topLeft.y;
        auto const textHeight = vFont.GetColumnHeight(text, destWidth);

        auto x = static_cast<float>(destRect.topLeft.x);
        switch (horizontalAlignment) {
        case moth_ui::TextHorizAlignment::Left:
            break;
        case moth_ui::TextHorizAlignment::Center:
            x = x + destWidth / 2;
            break;
        case moth_ui::TextHorizAlignment::Right:
            x = x + destWidth;
            break;
        }

        auto y = static_cast<float>(destRect.topLeft.y);
        switch (verticalAlignment) {
        case moth_ui::TextVertAlignment::Top:
            break;
        case moth_ui::TextVertAlignment::Middle:
            y = y + (destHeight - textHeight) / 2;
            break;
        case moth_ui::TextVertAlignment::Bottom:
            y = y + destHeight - textHeight;
            break;
        }

        m_graphics.SetBlendMode(moth_ui::BlendMode::Alpha);
        m_graphics.SetColor(m_drawColor.top());
        m_graphics.DrawText(text, font, horizontalAlignment, { x, y }, destWidth);
    }
}
