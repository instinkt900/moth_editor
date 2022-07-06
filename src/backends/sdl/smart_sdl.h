#pragma once

#include "SDL_FontCache.h"

using SurfaceRef = std::shared_ptr<SDL_Surface>;

inline SurfaceRef CreateSurfaceRef(SDL_Surface* surface) {
    return SurfaceRef(surface, SDL_FreeSurface);
}

inline SurfaceRef CreateSurfaceRef(std::filesystem::path const& assetPath) {
    return SurfaceRef(IMG_Load(assetPath.string().c_str()), SDL_FreeSurface);
}

class SDLTextureWrap {
public:
    static std::unique_ptr<SDLTextureWrap> CreateOwning(SDL_Texture* texture) {
        return std::unique_ptr<SDLTextureWrap>(new SDLTextureWrap(texture, true));
    }

    static std::unique_ptr<SDLTextureWrap> CreateNonOwning(SDL_Texture* texture) {
        return std::unique_ptr<SDLTextureWrap>(new SDLTextureWrap(texture, false));
    }

    ~SDLTextureWrap() {
        if (m_owning) {
            SDL_DestroyTexture(m_impl);
        }
    }

    SDL_Texture* GetImpl() {
        return m_impl;
    }

private:
    SDL_Texture* m_impl;
    bool m_owning;

    SDLTextureWrap(SDL_Texture* texture, bool owning)
    : m_impl(texture)
    , m_owning(owning) {
    }
};

using TextureRef = std::shared_ptr<SDLTextureWrap>;

inline TextureRef CreateTextureRef(SDL_Texture* texture) {
    return SDLTextureWrap::CreateOwning(texture);
}

inline TextureRef CreateTextureRef(SDL_Renderer* renderer, std::filesystem::path const& assetPath) {
    return SDLTextureWrap::CreateOwning(SDL_CreateTextureFromSurface(renderer, CreateSurfaceRef(assetPath).get()));
}

inline TextureRef CreateTextureRef(SDL_Renderer* renderer, SurfaceRef surface) {
    return SDLTextureWrap::CreateOwning(SDL_CreateTextureFromSurface(renderer, surface.get()));
}

//using AudioRef = std::shared_ptr<Mix_Chunk>;
//
//inline AudioRef CreateAudioRef(char const* assetPath) {
//    return AudioRef(Mix_LoadWAV(assetPath), Mix_FreeChunk);
//}
//
//using MusicRef = std::shared_ptr<Mix_Music>;
//
//inline MusicRef CreateMusicRef(char const* assetPath) {
//    return MusicRef(Mix_LoadMUS(assetPath), Mix_FreeMusic);
//}
//
using FontRef = std::shared_ptr<TTF_Font>;

inline FontRef CreateFontRef(std::filesystem::path const& assetPath, int size) {
    return FontRef(TTF_OpenFont(assetPath.string().c_str(), size), TTF_CloseFont);
}

using CachedFontRef = std::shared_ptr<FC_Font>;

inline CachedFontRef CreateCachedFontRef(SDL_Renderer* renderer, std::filesystem::path const& assetPath, int size, SDL_Color const& color, int style) {
    auto font = FC_CreateFont();
    FC_LoadFont(font, renderer, assetPath.string().c_str(), size, color, style);
    return CachedFontRef(font, FC_FreeFont);
}
