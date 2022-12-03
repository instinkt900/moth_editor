#pragma once

#include <SDL_ttf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PFNTTFINITPROC)(void);
extern PFNTTFINITPROC TTF_Init_Shim;
#define TTF_Init TTF_Init_Shim

typedef TTF_Font* (*PFNTTFOPENFONTRWPROC)(const char*, int);
extern PFNTTFOPENFONTRWPROC TTF_OpenFontRW_Shim;
#define TTF_OpenFontRW TTF_OpenFontRW_Shim

typedef TTF_Font* (*PFNTTFOPENFONTPROC)(const char*, int);
extern PFNTTFOPENFONTPROC TTF_OpenFont_Shim;
#define TTF_OpenFont TTF_OpenFont_Shim

typedef void (*PFNTTFCLOSEFONTPROC)(TTF_Font*);
extern PFNTTFCLOSEFONTPROC TTF_CloseFont_Shim;
#define TTF_CloseFont TTF_CloseFont_Shim

typedef void (*PFNTTFSETFONTSTYLEPROC)(TTF_Font*, int);
extern PFNTTFSETFONTSTYLEPROC TTF_SetFontStyle_Shim;
#define TTF_SetFontStyle TTF_SetFontStyle_Shim

typedef void (*PFNTTFSETFONTOUTLINEPROC)(TTF_Font*, int);
extern PFNTTFSETFONTOUTLINEPROC TTF_SetFontOutline_Shim;
#define TTF_SetFontOutline TTF_SetFontOutline_Shim

typedef int (*PFNTTFFONTHEIGHTPROC)(TTF_Font*);
extern PFNTTFFONTHEIGHTPROC TTF_FontHeight_Shim;
#define TTF_FontHeight TTF_FontHeight_Shim

typedef int (*PFNTTFFONTASCENTPROC)(TTF_Font*);
extern PFNTTFFONTASCENTPROC TTF_FontAscent_Shim;
#define TTF_FontAscent TTF_FontAscent_Shim

typedef int (*PFNTTFFONTDESCENTPROC)(TTF_Font*);
extern PFNTTFFONTDESCENTPROC TTF_FontDescent_Shim;
#define TTF_FontDescent TTF_FontDescent_Shim

typedef SDL_Surface* (*PFNTTFRENDERUTF8BLENDEDPROC)(TTF_Font*, const char*, SDL_Color);
extern PFNTTFRENDERUTF8BLENDEDPROC TTF_RenderUTF8_Blended_Shim;
#define TTF_RenderUTF8_Blended TTF_RenderUTF8_Blended_Shim

typedef SDL_Surface* (*PFNTTFWASINITPROC)(void);
extern PFNTTFWASINITPROC TTF_WasInit_Shim;
#define TTF_WasInit TTF_WasInit_Shim

extern void __cdecl load_sdl_ttf_shim();

#ifdef __cplusplus
}
#endif
