
//  SDL_ttf:  A companion library to SDL for working with TrueType (tm) fonts
//  Copyright (C) 2001-2012 Sam Lantinga <slouken@libsdl.org>

autoforget sdl2_ttf
requires sdl2

#if WINDOWS
DLLVocabulary sdl2_ttf SDL2_ttf.dll
#else
DLLVocabulary sdl2_ttf libSDL2_ttf.so
#endif

also sdl2_ttf definitions

// extern DECLSPEC const SDL_version * SDLCALL TTF_Linked_Version(void);
dll_0 TTF_Linked_Version

// ZERO WIDTH NO-BREAKSPACE (Unicode byte order mark)
enum:	eTTF_BOM_Characters
	0xFEFF	UNICODE_BOM_NATIVE	
	0xFFFE	UNICODE_BOM_SWAPPED
;enum

// This function tells the library whether UNICODE text is generally byteswapped.
// a UNICODE BOM character in a string will override this setting for the remainder of that string.

// extern DECLSPEC void SDLCALL TTF_ByteSwappedUNICODE(int swapped);
//DLLVoid dll_1 TTF_ByteSwappedUNICODE

// The internal structure containing font information 
// typedef struct _TTF_Font TTF_Font;

// Initialize the TTF engine - returns 0 if successful, -1 on error 
// extern DECLSPEC int SDLCALL TTF_Init(void);
dll_0 TTF_Init

// Open a font file and create a font of the specified point size.
// Some .fon fonts will have several sizes embedded in the file, so the
// point size becomes the index of choosing which size.  If the value
// is too high, the last indexed size will be the default. 

// extern DECLSPEC TTF_Font * SDLCALL TTF_OpenFont(const char *file, int ptsize);
// extern DECLSPEC TTF_Font * SDLCALL TTF_OpenFontIndex(const char *file, int ptsize, long index);
// extern DECLSPEC TTF_Font * SDLCALL TTF_OpenFontRW(SDL_RWops *src, int freesrc, int ptsize);
// extern DECLSPEC TTF_Font * SDLCALL TTF_OpenFontIndexRW(SDL_RWops *src, int freesrc, int ptsize, long index);
dll_2 TTF_OpenFont
dll_3 TTF_OpenFontIndex
dll_3 TTF_OpenFontRW
dll_4 TTF_OpenFontIndexRW

// Set and retrieve the font style 
enum: eTTF_Style
	0	TTF_STYLE_NORMAL
	1	TTF_STYLE_BOLD
	2	TTF_STYLE_ITALIC
	4	TTF_STYLE_UNDERLINE
	8	TTF_STYLE_STRIKETHROUGH
;enum

// extern DECLSPEC int SDLCALL TTF_GetFontStyle(const TTF_Font *font);
// extern DECLSPEC void SDLCALL TTF_SetFontStyle(TTF_Font *font, int style);
// extern DECLSPEC int SDLCALL TTF_GetFontOutline(const TTF_Font *font);
// extern DECLSPEC void SDLCALL TTF_SetFontOutline(TTF_Font *font, int outline);
dll_1 TTF_GetFontStyle
DLLVoid dll_2 TTF_SetFontStyle
dll_1 TTF_GetFontOutline
DLLVoid dll_2 TTF_SetFontOutline

// Set and retrieve FreeType hinter settings 
enum:	eTTF_Hinting
	TTF_HINTING_NORMAL
	TTF_HINTING_LIGHT
	TTF_HINTING_MONO
	TTF_HINTING_NONE
;enum

// extern DECLSPEC int SDLCALL TTF_GetFontHinting(const TTF_Font *font);
// extern DECLSPEC void SDLCALL TTF_SetFontHinting(TTF_Font *font, int hinting);
dll_1 TTF_GetFontHinting
DLLVoid dll_2 TTF_SetFontHinting

// Get the total height of the font - usually equal to point size 
// extern DECLSPEC int SDLCALL TTF_FontHeight(const TTF_Font *font);
dll_1 TTF_FontHeight

// Get the offset from the baseline to the top of the font
//   This is a positive value, relative to the baseline.
// extern DECLSPEC int SDLCALL TTF_FontAscent(const TTF_Font *font);
dll_1 TTF_FontAscent

// Get the offset from the baseline to the bottom of the font
//   This is a negative value, relative to the baseline.
// extern DECLSPEC int SDLCALL TTF_FontDescent(const TTF_Font *font);
dll_1 TTF_FontDescent

// Get the recommended spacing between lines of text for this font 
// extern DECLSPEC int SDLCALL TTF_FontLineSkip(const TTF_Font *font);
dll_1 TTF_FontLineSkip

// Get/Set whether or not kerning is allowed for this font 
// extern DECLSPEC int SDLCALL TTF_GetFontKerning(const TTF_Font *font);
// extern DECLSPEC void SDLCALL TTF_SetFontKerning(TTF_Font *font, int allowed);
dll_1 TTF_GetFontKerning
DLLVoid dll_2 TTF_SetFontKerning

// Get the number of faces of the font 
// extern DECLSPEC long SDLCALL TTF_FontFaces(const TTF_Font *font);
dll_1 TTF_FontFaces

// Get the font face attributes, if any 
// extern DECLSPEC int SDLCALL TTF_FontFaceIsFixedWidth(const TTF_Font *font);
// extern DECLSPEC char * SDLCALL TTF_FontFaceFamilyName(const TTF_Font *font);
// extern DECLSPEC char * SDLCALL TTF_FontFaceStyleName(const TTF_Font *font);
dll_1 TTF_FontFaceIsFixedWidth
dll_1 TTF_FontFaceFamilyName
dll_1 TTF_FontFaceStyleName

// Check wether a glyph is provided by the font or not 
// extern DECLSPEC int SDLCALL TTF_GlyphIsProvided(const TTF_Font *font, Uint16 ch);
dll_2 TTF_GlyphIsProvided

// Get the metrics (dimensions) of a glyph
//   To understand what these metrics mean, here is a useful link:
//    http://freetype.sourceforge.net/freetype2/docs/tutorial/step2.html
 
// extern DECLSPEC int SDLCALL TTF_GlyphMetrics(TTF_Font *font, Uint16 ch, int *minx, int *maxx, int *miny, int *maxy, int *advance);
dll_7 TTF_GlyphMetrics

// Get the dimensions of a rendered string of text 
// extern DECLSPEC int SDLCALL TTF_SizeText(TTF_Font *font, const char *text, int *w, int *h);
// extern DECLSPEC int SDLCALL TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h);
// extern DECLSPEC int SDLCALL TTF_SizeUNICODE(TTF_Font *font, const Uint16 *text, int *w, int *h);
dll_4 TTF_SizeText
dll_4 TTF_SizeUTF8
dll_4 TTF_SizeUNICODE

// Create an 8-bit palettized surface and render the given text at
//   fast quality with the given font and color.  The 0 pixel is the
//   colorkey, giving a transparent background, and the 1 pixel is set
//   to the text color.
//   This function returns the new surface, or NULL if there was an error.

// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderText_Solid(TTF_Font *font, const char *text, SDL_Color fg);
// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUTF8_Solid(TTF_Font *font, const char *text, SDL_Color fg);
// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUNICODE_Solid(TTF_Font *font, const Uint16 *text, SDL_Color fg);
dll_3 TTF_RenderText_Solid
dll_3 TTF_RenderUTF8_Solid
dll_3 TTF_RenderUNICODE_Solid

// Create an 8-bit palettized surface and render the given glyph at
//   fast quality with the given font and color.  The 0 pixel is the
//   colorkey, giving a transparent background, and the 1 pixel is set
//   to the text color.  The glyph is rendered without any padding or
//   centering in the X direction, and aligned normally in the Y direction.
//   This function returns the new surface, or NULL if there was an error.

// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderGlyph_Solid(TTF_Font *font, Uint16 ch, SDL_Color fg);
dll_3 TTF_RenderGlyph_Solid

// Create an 8-bit palettized surface and render the given text at
//   high quality with the given font and colors.  The 0 pixel is background,
//   while other pixels have varying degrees of the foreground color.
//   This function returns the new surface, or NULL if there was an error.

// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderText_Shaded(TTF_Font *font, const char *text, SDL_Color fg, SDL_Color bg);
// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUTF8_Shaded(TTF_Font *font, const char *text, SDL_Color fg, SDL_Color bg);
// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUNICODE_Shaded(TTF_Font *font, const Uint16 *text, SDL_Color fg, SDL_Color bg);
dll_4 TTF_RenderText_Shaded
dll_4 TTF_RenderUTF8_Shaded
dll_4 TTF_RenderUNICODE_Shaded

// Create an 8-bit palettized surface and render the given glyph at
//   high quality with the given font and colors.  The 0 pixel is background,
//   while other pixels have varying degrees of the foreground color.
//   The glyph is rendered without any padding or centering in the X
//   direction, and aligned normally in the Y direction.
//   This function returns the new surface, or NULL if there was an error.

// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderGlyph_Shaded(TTF_Font *font, Uint16 ch, SDL_Color fg, SDL_Color bg);
dll_4 TTF_RenderGlyph_Shaded

// Create a 32-bit ARGB surface and render the given text at high quality,
//   using alpha blending to dither the font with the given color.
//   This function returns the new surface, or NULL if there was an error.

// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderText_Blended(TTF_Font *font, const char *text, SDL_Color fg);
// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg);
// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderUNICODE_Blended(TTF_Font *font, const Uint16 *text, SDL_Color fg);
dll_3 TTF_RenderText_Blended
dll_3 TTF_RenderUTF8_Blended
dll_3 TTF_RenderUNICODE_Blended

// Create a 32-bit ARGB surface and render the given glyph at high quality,
//   using alpha blending to dither the font with the given color.
//   The glyph is rendered without any padding or centering in the X
//   direction, and aligned normally in the Y direction.
//   This function returns the new surface, or NULL if there was an error.

// extern DECLSPEC SDL_Surface * SDLCALL TTF_RenderGlyph_Blended(TTF_Font *font, Uint16 ch, SDL_Color fg);
dll_3 TTF_RenderGlyph_Blended

// For compatibility with previous versions, here are the old functions 
//#define TTF_RenderText(font, text, fg, bg)		TTF_RenderText_Shaded(font, text, fg, bg)
//#define TTF_RenderUTF8(font, text, fg, bg)		TTF_RenderUTF8_Shaded(font, text, fg, bg)
//#define TTF_RenderUNICODE(font, text, fg, bg)	TTF_RenderUNICODE_Shaded(font, text, fg, bg)

// Close an opened font file 
// extern DECLSPEC void SDLCALL TTF_CloseFont(TTF_Font *font);
DLLVoid DLLVoid dll_1 TTF_CloseFont

// De-initialize the TTF engine 
// extern DECLSPEC void SDLCALL TTF_Quit(void);
DLLVoid dll_0 TTF_Quit

// Check if the TTF engine is initialized 
// extern DECLSPEC int SDLCALL TTF_WasInit(void);
dll_0 TTF_WasInit

// Get the kerning size of two glyphs 
// extern DECLSPEC int TTF_GetFontKerningSize(TTF_Font *font, int prev_index, int index);
//dll_3 TTF_GetFontKerningSize

previous definitions
loaddone
