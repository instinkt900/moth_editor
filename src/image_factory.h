#pragma once

#include "moth_ui/iimage_factory.h"
#include "moth_ui/iimage.h"

class ImageFactory : public moth_ui::IImageFactory {
public:
    ImageFactory(SDL_Renderer& renderer);
    virtual ~ImageFactory() = default;

    std::unique_ptr<moth_ui::IImage> GetImage(std::filesystem::path const& path);

private:
    SDL_Renderer& m_renderer;
};
