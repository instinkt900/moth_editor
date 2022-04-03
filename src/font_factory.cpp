#include "common.h"
#include "font_factory.h"
#include "font.h"

FontFactory::FontFactory(SDL_Renderer& renderer)
    : m_renderer(renderer) {
}

std::unique_ptr<moth_ui::IFont> FontFactory::GetDefaultFont(int size) {
    return GetFont("pilotcommand.ttf", size);
}

std::unique_ptr<moth_ui::IFont> FontFactory::GetFont(char const* path, int size) {
    SDL_Color defaultColor{ 0xFF, 0xFF, 0xFF, 0xFF };
    return std::make_unique<Font>(CreateCachedFontRef(&m_renderer, path, size, defaultColor, TTF_STYLE_NORMAL));
}
