#pragma once

#include "../igraphics_context.h"
#include <SDL.h>

namespace backend::sdl {
    class SDLGraphics : public IGraphicsContext {
    public:
        SDLGraphics(SDL_Renderer* renderer);

        void SetBlendMode(moth_ui::BlendMode mode) override;
        //void SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) override;
        //void SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) override;
        void SetColor(moth_ui::Color const& color) override;
        void Clear() override;
        void DrawImage(moth_ui::IImage& image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) override;
        void DrawToPNG(std::filesystem::path const& path) override;
        void DrawRectF(moth_ui::FloatRect const& rect) override;
        void DrawFillRectF(moth_ui::FloatRect const& rect) override;
        void DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) override;
        void DrawText(std::string const& text, moth_ui::IFont& font, moth_ui::IntRect const& pos) override {}

        std::unique_ptr<moth_ui::ITarget> CreateTarget(int width, int height) override;
        moth_ui::ITarget* GetTarget() override;
        void SetTarget(moth_ui::ITarget* target) override;

        void SetLogicalSize(moth_ui::IntVec2 const& logicalSize) override;

    private:
        SDL_Renderer* m_renderer = nullptr;
    };
}