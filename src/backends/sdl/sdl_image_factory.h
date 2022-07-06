#pragma once

#include "moth_ui/iimage_factory.h"
#include "moth_ui/iimage.h"
#include "smart_sdl.h"

class SDLImageFactory : public moth_ui::IImageFactory {
public:
    SDLImageFactory(SDL_Renderer& renderer);
    virtual ~SDLImageFactory() = default;

    void FlushCache() override;
    bool LoadTexturePack(std::filesystem::path const& path) override;

    std::unique_ptr<moth_ui::IImage> GetImage(std::filesystem::path const& path) override;

private:
    SDL_Renderer& m_renderer;

    struct ImageDesc {
        TextureRef m_texture;
        moth_ui::IntRect m_sourceRect;
        std::string m_path;
    };

    std::unordered_map<std::string, ImageDesc> m_cachedImages;
};
