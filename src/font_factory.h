#pragma once

#include "moth_ui/ifont_factory.h"
#include "moth_ui/ifont.h"

class FontFactory : public moth_ui::IFontFactory {
public:
    FontFactory(SDL_Renderer& renderer);
    virtual ~FontFactory() = default;

    std::unique_ptr<moth_ui::IFont> GetDefaultFont(int size) override;
    std::unique_ptr<moth_ui::IFont> GetFont(char const* path, int size) override;

private:
    SDL_Renderer& m_renderer;
};
