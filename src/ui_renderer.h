#pragma once

#include "moth_ui/irenderer.h"

class UIRenderer : public ui::IRenderer {
public:
    UIRenderer(SDL_Renderer& renderer);
    virtual ~UIRenderer() = default;

    void SetRenderColor(uint32_t argb) override;
    void RenderRect(IntRect const& rect) override;
    void RenderFilledRect(IntRect const& rect) override;
    void RenderImage(ui::IImage& image, IntRect const& destRect) override;

private:
    SDL_Renderer& m_renderer;
};
