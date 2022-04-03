#pragma once

#include "moth_ui/ifont.h"

class Font : public moth_ui::IFont {
public:
    explicit Font(CachedFontRef fontObj);
    virtual ~Font() = default;

    CachedFontRef GetFontObj() const {
        return m_fontObj;
    }

private:
    CachedFontRef m_fontObj;
};
