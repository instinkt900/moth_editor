#pragma once

#include "SDL_FontCache.h"

using SurfaceRef = std::shared_ptr<SDL_Surface>;

inline SurfaceRef CreateSurfaceRef(SDL_Surface* surface) {
    return SurfaceRef(surface, SDL_FreeSurface);
}

inline SurfaceRef CreateSurfaceRef(char const* assetPath) {
    return SurfaceRef(IMG_Load(assetPath), SDL_FreeSurface);
}

using TextureRef = std::shared_ptr<SDL_Texture>;

inline TextureRef CreateTextureRef(SDL_Texture* texture) {
    return TextureRef(texture, SDL_DestroyTexture);
}

inline TextureRef CreateTextureRef(SDL_Renderer* renderer, char const* assetPath) {
    return TextureRef(SDL_CreateTextureFromSurface(renderer, CreateSurfaceRef(assetPath).get()), SDL_DestroyTexture);
}

inline TextureRef CreateTextureRef(SDL_Renderer* renderer, SurfaceRef surface) {
    return TextureRef(SDL_CreateTextureFromSurface(renderer, surface.get()), SDL_DestroyTexture);
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

inline FontRef CreateFontRef(char const* assetPath, int size) {
    return FontRef(TTF_OpenFont(assetPath, size), TTF_CloseFont);
}

using CachedFontRef = std::shared_ptr<FC_Font>;

inline CachedFontRef CreateCachedFontRef(SDL_Renderer* renderer, char const* assetPath, int size, SDL_Color const& color, int style) {
    auto font = FC_CreateFont();
    FC_LoadFont(font, renderer, assetPath, size, color, style);
    return CachedFontRef(font, FC_FreeFont);
}
