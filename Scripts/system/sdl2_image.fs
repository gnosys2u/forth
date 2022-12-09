//  SDL_image:  An example image loading library for use with SDL
//  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

// A simple library to load images of various formats as SDL surfaces


autoforget sdl2_image
requires sdl2

#if WINDOWS
DLLVocabulary sdl2_image SDL2_image.dll
#else
DLLVocabulary sdl2_image libSDL2_image.so
#endif

also sdl2_image definitions


// A simple library to load images of various formats as SDL surfaces
enum: eSDL_Image_Misc
  2 SDL_IMAGE_MAJOR_VERSION
  0 SDL_IMAGE_MINOR_VERSION
  0 SDL_IMAGE_PATCHLEVEL

  // IMG_InitFlags
  1 IMG_INIT_JPG
  2 IMG_INIT_PNG
  4 IMG_INIT_TIF
  8 IMG_INIT_WEBP
;enum

dll_0   IMG_Linked_Version        // (returns const SDL_version *)

// Loads dynamic libraries and prepares them for use.  Flags should be one or more flags from IMG_InitFlags OR'd together.
//   It returns the flags successfully initialized, or 0 on failure.
dll_1           IMG_Init      // int flags

// Unloads libraries loaded with IMG_Init
DLLVoid dll_0   IMG_Quit

// Load an image from an SDL data source.
//   The 'type' may be one of: "BMP", "GIF", "PNG", etc.

//   If the image format supports a transparent pixel, SDL will set the
//   colorkey for the surface.  You can enable RLE acceleration on the
//   surface afterwards by calling:   SDL_SetColorKey(image, SDL_RLEACCEL, image->format->colorkey);
dll_3           IMG_LoadTyped_RW      // SDL_RWops *src, int freesrc, const char *type (returns SDL_Surface *)

// Convenience functions (return SDL_Surface *)
dll_1           IMG_Load        // const char *file
dll_2           IMG_Load_RW     // SDL_RWops *src, int freesrc

// Load an image directly into a render texture. (return SDL_Texture *)
dll_2           IMG_LoadTexture           // SDL_Renderer *renderer, const char *file
dll_3           IMG_LoadTexture_RW        // SDL_Renderer *renderer, SDL_RWops *src, int freesrc
dll_4           IMG_LoadTextureTyped_RW   // SDL_Renderer *renderer, SDL_RWops *src, int freesrc, const char *type

// Functions to detect a file type, given a seekable source (return int)
dll_1           IMG_isICO       // SDL_RWops *src
dll_1           IMG_isCUR       // SDL_RWops *src
dll_1           IMG_isBMP       // SDL_RWops *src
dll_1           IMG_isGIF       // SDL_RWops *src
dll_1           IMG_isJPG       // SDL_RWops *src
dll_1           IMG_isLBM       // SDL_RWops *src
dll_1           IMG_isPCX       // SDL_RWops *src
dll_1           IMG_isPNG       // SDL_RWops *src
dll_1           IMG_isPNM       // SDL_RWops *src
dll_1           IMG_isTIF       // SDL_RWops *src
dll_1           IMG_isXCF       // SDL_RWops *src
dll_1           IMG_isXPM       // SDL_RWops *src
dll_1           IMG_isXV        // SDL_RWops *src
dll_1           IMG_isWEBP      // SDL_RWops *src

// Individual loading functions (return SDL_Surface *)
dll_1           IMG_LoadICO_RW    // SDL_RWops *src
dll_1           IMG_LoadCUR_RW    // SDL_RWops *src
dll_1           IMG_LoadBMP_RW    // SDL_RWops *src
dll_1           IMG_LoadGIF_RW    // SDL_RWops *src
dll_1           IMG_LoadJPG_RW    // SDL_RWops *src
dll_1           IMG_LoadLBM_RW    // SDL_RWops *src
dll_1           IMG_LoadPCX_RW    // SDL_RWops *src
dll_1           IMG_LoadPNG_RW    // SDL_RWops *src
dll_1           IMG_LoadPNM_RW    // SDL_RWops *src
dll_1           IMG_LoadTGA_RW    // SDL_RWops *src
dll_1           IMG_LoadTIF_RW    // SDL_RWops *src
dll_1           IMG_LoadXCF_RW    // SDL_RWops *src
dll_1           IMG_LoadXPM_RW    // SDL_RWops *src
dll_1           IMG_LoadXV_RW     // SDL_RWops *src
dll_1           IMG_LoadWEBP_RW   // SDL_RWops *src

dll_1           IMG_ReadXPMFromArray    // char **xpm

// Individual saving functions (return int)
dll_2           IMG_SavePNG       // SDL_Surface *surface, const char *file
dll_3           IMG_SavePNG_RW    // SDL_Surface *surface, SDL_RWops *dst, int freedst

previous definitions
loaddone

