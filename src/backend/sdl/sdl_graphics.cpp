#include "common.h"
#include "sdl_graphics.h"
#include "sdl_image.h"
#include "sdl_utils.h"

namespace {
    SDL_BlendMode ToSDL(backend::EBlendMode mode) {
        switch (mode) {
        default:
        case backend::EBlendMode::Invalid :
            return SDL_BlendMode::SDL_BLENDMODE_INVALID;
        case backend::EBlendMode::None:
            return SDL_BlendMode::SDL_BLENDMODE_NONE;
        case backend::EBlendMode::Blend:
            return SDL_BlendMode::SDL_BLENDMODE_BLEND;
        case backend::EBlendMode::Add:
            return SDL_BlendMode::SDL_BLENDMODE_ADD;
        case backend::EBlendMode::Mod:
            return SDL_BlendMode::SDL_BLENDMODE_MOD;
        case backend::EBlendMode::Mul:
            return SDL_BlendMode::SDL_BLENDMODE_MUL;
        }
    }
}

namespace backend::sdl {
    SDLGraphics::SDLGraphics(SDL_Renderer* renderer)
    :m_renderer(renderer) {
    }
    void SDLGraphics::SetBlendMode(EBlendMode mode) {
        SDL_SetRenderDrawBlendMode(m_renderer, ToSDL(mode));
    }

    void SDLGraphics::SetBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) {
        auto sdlImage = std::dynamic_pointer_cast<Image>(target);
        auto sdlTexture = sdlImage->GetTexture();
        SDL_SetTextureBlendMode(sdlTexture->GetImpl(), ToSDL(mode));
    }

    void SDLGraphics::SetColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) {
        auto sdlImage = std::dynamic_pointer_cast<Image>(target);
        auto sdlTexture = sdlImage->GetTexture();
        ColorComponents components(color);
        SDL_SetTextureColorMod(sdlTexture->GetImpl(), components.r, components.g, components.b);
        SDL_SetTextureAlphaMod(sdlTexture->GetImpl(), components.a);
    }

    void SDLGraphics::SetColor(moth_ui::Color const& color) {
        ColorComponents components(color);
        SDL_SetRenderDrawColor(m_renderer, components.r, components.g, components.b, components.a);
    }

    void SDLGraphics::Clear() {
        SDL_RenderClear(m_renderer);
    }

    void SDLGraphics::DrawImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) {
        auto sdlImage = std::dynamic_pointer_cast<Image>(image);
        auto sdlTexture = sdlImage->GetTexture();

        if (sourceRect && destRect) {
            auto srcRect = ToSDL(*sourceRect);
            auto dstRect = ToSDL(*destRect);
            SDL_RenderCopy(m_renderer, sdlTexture->GetImpl(), &srcRect, &dstRect);
        } else if (sourceRect) {
            auto srcRect = ToSDL(*sourceRect);
            SDL_RenderCopy(m_renderer, sdlTexture->GetImpl(), &srcRect, nullptr);
        } else if (destRect) {
            auto dstRect = ToSDL(*destRect);
            SDL_RenderCopy(m_renderer, sdlTexture->GetImpl(), nullptr, &dstRect);
        } else {
            SDL_RenderCopy(m_renderer, sdlTexture->GetImpl(), nullptr, nullptr);
        }
    }

    void SDLGraphics::DrawToPNG(std::filesystem::path const& path) {
        Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
#else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
#endif

        int width;
        int height;
        SDL_GetRendererOutputSize(m_renderer, &width, &height);

        SurfaceRef surface = CreateSurfaceRef(SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask));
        SDL_RenderReadPixels(m_renderer, nullptr, surface->format->format, surface->pixels, surface->pitch);
        IMG_SavePNG(surface.get(), path.string().c_str());
    }

    void SDLGraphics::DrawRectF(moth_ui::FloatRect const& rect) {
        auto const sdlRectF = ToSDL(rect);
        SDL_RenderDrawRectF(m_renderer, &sdlRectF);
    }

    void SDLGraphics::DrawFillRectF(moth_ui::FloatRect const& rect) {
        auto const sdlRectF = ToSDL(rect);
        SDL_RenderFillRectF(m_renderer, &sdlRectF);
    }

    void SDLGraphics::DrawLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) {
        SDL_RenderDrawLineF(m_renderer, p0.x, p0.y, p1.x, p1.y);
    }

    std::unique_ptr<moth_ui::IImage> SDLGraphics::CreateTarget(int width, int height) {
        auto sdlTexture = CreateTextureRef(SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height));

        moth_ui::IntVec2 const dimensions{ width, height };
        moth_ui::IntRect const sourceRect{ { 0, 0 }, { width, height } };
        return std::make_unique<Image>(sdlTexture, dimensions, sourceRect);
    }

    std::shared_ptr<moth_ui::IImage> SDLGraphics::GetTarget() {
        std::shared_ptr<SDLTextureWrap> sdlTexture = SDLTextureWrap::CreateNonOwning(SDL_GetRenderTarget(m_renderer));
        return std::make_shared<Image>(sdlTexture);
    }

    void SDLGraphics::SetTarget(std::shared_ptr<moth_ui::IImage> target) {
        if (!target) {
            SDL_SetRenderTarget(m_renderer, nullptr);
        } else {
            auto sdlImage = std::dynamic_pointer_cast<Image>(target);
            auto sdlTexture = sdlImage->GetTexture();
            SDL_SetRenderTarget(m_renderer, sdlTexture->GetImpl());
        }
    }

    void SDLGraphics::SetLogicalSize(moth_ui::IntVec2 const& logicalSize) {
        SDL_RenderSetLogicalSize(m_renderer, logicalSize.x, logicalSize.y);
    }
}
