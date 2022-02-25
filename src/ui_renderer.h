#pragma once

#include "moth_ui/irenderer.h"

class UIRenderer : public moth_ui::IRenderer {
public:
    UIRenderer(SDL_Renderer& renderer);
    virtual ~UIRenderer() = default;

    void SetRenderColor(uint32_t argb) override;
    void RenderRect(moth_ui::IntRect const& rect) override;
    void RenderFilledRect(moth_ui::IntRect const& rect) override;
    void RenderImage(moth_ui::IImage& image, moth_ui::IntRect const& destRect) override;

private:
    SDL_Renderer& m_renderer;
};
