#pragma once

#include "moth_ui/irenderer.h"

class UIRenderer : public moth_ui::IRenderer {
public:
    UIRenderer(SDL_Renderer& renderer);
    virtual ~UIRenderer() = default;

    void PushBlendMode(moth_ui::BlendMode mode) override;
    void PopBlendMode() override;
    void PushColor(moth_ui::Color const& color) override;
    void PopColor() override;

    void PushClip(moth_ui::IntRect const& rect) override;
    void PopClip() override;

    void RenderRect(moth_ui::IntRect const& rect) override;
    void RenderFilledRect(moth_ui::IntRect const& rect) override;
    void RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& destRect, moth_ui::ImageScaleType scaleType, float scale) override;
    void RenderText(std::string const& text, moth_ui::IFont& font, moth_ui::TextHorizAlignment horizontalAlignment, moth_ui::TextVertAlignment verticalAlignment, moth_ui::IntRect const& destRect) override;

private:
    SDL_Renderer& m_renderer;
    std::stack<moth_ui::Color> m_drawColor;
    std::stack<moth_ui::BlendMode> m_blendMode;
    std::stack<moth_ui::IntRect> m_clip;
};
