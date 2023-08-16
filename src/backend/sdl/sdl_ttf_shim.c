#include "sdl_ttf_shim.h"
#include <windows.h>

PFNTTFINITPROC TTF_Init_Shim = NULL;
PFNTTFOPENFONTRWPROC TTF_OpenFontRW_Shim = NULL;
PFNTTFOPENFONTPROC TTF_OpenFont_Shim = NULL;
PFNTTFCLOSEFONTPROC TTF_CloseFont_Shim = NULL;
PFNTTFSETFONTSTYLEPROC TTF_SetFontStyle_Shim = NULL;
PFNTTFSETFONTOUTLINEPROC TTF_SetFontOutline_Shim = NULL;
PFNTTFFONTHEIGHTPROC TTF_FontHeight_Shim = NULL;
PFNTTFFONTASCENTPROC TTF_FontAscent_Shim = NULL;
PFNTTFFONTDESCENTPROC TTF_FontDescent_Shim = NULL;
PFNTTFRENDERUTF8BLENDEDPROC TTF_RenderUTF8_Blended_Shim = NULL;
PFNTTFWASINITPROC TTF_WasInit_Shim = NULL;

static HMODULE libSDLTTF;

void load_sdl_ttf_shim() {
    libSDLTTF = LoadLibraryW(L"SDL2_ttf.dll");
    if (libSDLTTF != NULL) {
        TTF_Init_Shim = (PFNTTFINITPROC)GetProcAddress(libSDLTTF, "TTF_Init");
        TTF_OpenFontRW_Shim = (PFNTTFOPENFONTRWPROC)GetProcAddress(libSDLTTF, "TTF_OpenFontRW");
        TTF_OpenFont_Shim = (PFNTTFOPENFONTPROC)GetProcAddress(libSDLTTF, "TTF_OpenFont");
        TTF_CloseFont_Shim = (PFNTTFCLOSEFONTPROC)GetProcAddress(libSDLTTF, "TTF_CloseFont");
        TTF_SetFontStyle_Shim = (PFNTTFSETFONTSTYLEPROC)GetProcAddress(libSDLTTF, "TTF_SetFontStyle");
        TTF_SetFontOutline_Shim = (PFNTTFSETFONTOUTLINEPROC)GetProcAddress(libSDLTTF, "TTF_SetFontOutline");
        TTF_FontHeight_Shim = (PFNTTFFONTHEIGHTPROC)GetProcAddress(libSDLTTF, "TTF_FontHeight");
        TTF_FontAscent_Shim = (PFNTTFFONTASCENTPROC)GetProcAddress(libSDLTTF, "TTF_FontAscent");
        TTF_FontDescent_Shim = (PFNTTFFONTDESCENTPROC)GetProcAddress(libSDLTTF, "TTF_FontDescent");
        TTF_RenderUTF8_Blended_Shim = (PFNTTFRENDERUTF8BLENDEDPROC)GetProcAddress(libSDLTTF, "TTF_RenderUTF8_Blended");
        TTF_WasInit_Shim = (PFNTTFWASINITPROC)GetProcAddress(libSDLTTF, "TTF_WasInit");
    }
}
