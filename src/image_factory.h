#pragma once

#include "moth_ui/iimage_factory.h"
#include "moth_ui/iimage.h"

class ImageFactory : public ui::IImageFactory {
public:
    ImageFactory(SDL_Renderer& renderer);
    virtual ~ImageFactory() = default;

    std::unique_ptr<ui::IImage> GetImage(char const* path);

private:
    SDL_Renderer& m_renderer;
};
