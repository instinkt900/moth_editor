#include "common.h"
#include "sdl_app.h"
#include "sdl_image.h"
#include "sdl_utils.h"

std::unique_ptr<moth_ui::IImage> CreateRenderTarget(int width, int height) {
    auto renderer = g_sdlApp->GetRenderer();
    auto sdlTexture = CreateTextureRef(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height));

    moth_ui::IntVec2 const dimensions{ width, height };
    moth_ui::IntRect const sourceRect{ { 0, 0 }, { width, height } };
    return std::make_unique<SDLImage>(sdlTexture, dimensions, sourceRect);
}

std::shared_ptr<moth_ui::IImage> GetRenderTarget() {
    auto renderer = g_sdlApp->GetRenderer();
    std::shared_ptr<SDLTextureWrap> sdlTexture = SDLTextureWrap::CreateNonOwning(SDL_GetRenderTarget(renderer));
    return std::make_shared<SDLImage>(sdlTexture);
}

void SetRenderTarget(std::shared_ptr<moth_ui::IImage> target) {
    auto renderer = g_sdlApp->GetRenderer();
    if (!target) {
        SDL_SetRenderTarget(renderer, nullptr);
    } else {
        auto sdlImage = std::dynamic_pointer_cast<SDLImage>(target);
        auto sdlTexture = sdlImage->GetTexture();
        SDL_SetRenderTarget(renderer, sdlTexture->GetImpl());
    }
}

void SetRenderBlendMode(EBlendMode mode) {
    SDL_BlendMode sdlMode;
    switch (mode) {
    default:
    case EBlendMode::Invalid:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_INVALID;
        break;
    case EBlendMode::None:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_NONE;
        break;
    case EBlendMode::Blend:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_BLEND;
        break;
    case EBlendMode::Add:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_ADD;
        break;
    case EBlendMode::Mod:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_MOD;
        break;
    case EBlendMode::Mul:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_MUL;
        break;
    }
    auto renderer = g_sdlApp->GetRenderer();
    SDL_SetRenderDrawBlendMode(renderer, sdlMode);
}

void SetImageBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode) {
    SDL_BlendMode sdlMode;
    switch (mode) {
    default:
    case EBlendMode::Invalid:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_INVALID;
        break;
    case EBlendMode::None:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_NONE;
        break;
    case EBlendMode::Blend:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_BLEND;
        break;
    case EBlendMode::Add:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_ADD;
        break;
    case EBlendMode::Mod:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_MOD;
        break;
    case EBlendMode::Mul:
        sdlMode = SDL_BlendMode::SDL_BLENDMODE_MUL;
        break;
    }

    auto sdlImage = std::dynamic_pointer_cast<SDLImage>(target);
    auto sdlTexture = sdlImage->GetTexture();
    SDL_SetTextureBlendMode(sdlTexture->GetImpl(), sdlMode);
}

void SetImageColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color) {
    auto sdlImage = std::dynamic_pointer_cast<SDLImage>(target);
    auto sdlTexture = sdlImage->GetTexture();
    ColorComponents components(color);
    SDL_SetTextureColorMod(sdlTexture->GetImpl(), components.r, components.g, components.b);
    SDL_SetTextureAlphaMod(sdlTexture->GetImpl(), components.a);
}

void SetDrawColor(moth_ui::Color const& color) {
    auto renderer = g_sdlApp->GetRenderer();
    ColorComponents components(color);
    SDL_SetRenderDrawColor(renderer, components.r, components.g, components.b, components.a);
}

void RenderClear() {
    auto renderer = g_sdlApp->GetRenderer();
    SDL_RenderClear(renderer);
}

void RenderImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect) {
    auto renderer = g_sdlApp->GetRenderer();
    auto sdlImage = std::dynamic_pointer_cast<SDLImage>(image);
    auto sdlTexture = sdlImage->GetTexture();

    if (sourceRect && destRect) {
        auto srcRect = ToSDL(*sourceRect);
        auto dstRect = ToSDL(*destRect);
        SDL_RenderCopy(renderer, sdlTexture->GetImpl(), &srcRect, &dstRect);
    } else if (sourceRect) {
        auto srcRect = ToSDL(*sourceRect);
        SDL_RenderCopy(renderer, sdlTexture->GetImpl(), &srcRect, nullptr);
    } else if (destRect) {
        auto dstRect = ToSDL(*destRect);
        SDL_RenderCopy(renderer, sdlTexture->GetImpl(), nullptr, &dstRect);
    } else {
        SDL_RenderCopy(renderer, sdlTexture->GetImpl(), nullptr, nullptr);
    }
}

void RenderToPNG(std::filesystem::path const& path) {
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

    auto renderer = g_sdlApp->GetRenderer();

    int width;
    int height;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    SurfaceRef surface = CreateSurfaceRef(SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask));
    SDL_RenderReadPixels(renderer, nullptr, surface->format->format, surface->pixels, surface->pitch);
    IMG_SavePNG(surface.get(), path.string().c_str());
}

void RenderRectF(moth_ui::FloatRect const& rect) {
    auto renderer = g_sdlApp->GetRenderer();
    auto const sdlRectF = ToSDL(rect);
    SDL_RenderDrawRectF(renderer, &sdlRectF);
}

void RenderFillRectF(moth_ui::FloatRect const& rect) {
    auto renderer = g_sdlApp->GetRenderer();
    auto const sdlRectF = ToSDL(rect);
    SDL_RenderFillRectF(renderer, &sdlRectF);
}

void RenderLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1) {
    auto renderer = g_sdlApp->GetRenderer();
    SDL_RenderDrawLineF(renderer, p0.x, p0.y, p1.x, p1.y);
}

void SetRenderLogicalSize(moth_ui::IntVec2 const& logicalSize) {
    auto renderer = g_sdlApp->GetRenderer();
    SDL_RenderSetLogicalSize(renderer, logicalSize.x, logicalSize.y);
}
