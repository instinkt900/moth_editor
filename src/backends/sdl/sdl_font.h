#pragma once

#include "moth_ui/ifont.h"
#include "smart_sdl.h"

class SDLFont : public moth_ui::IFont {
public:
    explicit SDLFont(CachedFontRef fontObj);
    virtual ~SDLFont() = default;

    CachedFontRef GetFontObj() const {
        return m_fontObj;
    }

private:
    CachedFontRef m_fontObj;
};
