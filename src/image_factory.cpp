#include "common.h"
#include "image_factory.h"
#include "image.h"

ImageFactory::ImageFactory(SDL_Renderer& renderer)
    : m_renderer(renderer) {
}

std::unique_ptr<moth_ui::IImage> ImageFactory::GetImage(char const* path) {
    auto texture = CreateTextureRef(&m_renderer, path);

    moth_ui::IntRect sourceRect{};
    SDL_QueryTexture(texture.get(), NULL, NULL, &sourceRect.bottomRight.x, &sourceRect.bottomRight.y);
    return std::make_unique<Image>(texture, sourceRect);
}
