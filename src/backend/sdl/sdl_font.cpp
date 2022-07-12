#include "common.h"
#include "sdl_font.h"

namespace backend::sdl {
    Font::Font(CachedFontRef fontObj)
        : m_fontObj(fontObj) {
    }
}
