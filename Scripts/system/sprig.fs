
autoforget sprig
autoload sdl sdl.txt

DLLVocabulary sprig sprig.dll
also sdl
also sprig definitions

// entry points can be defined with:
// ARGUMENT_COUNT addSdlEntry ENTRY_POINT_NAME
// or use the convenience words sprig0 ... sprig5

//    SPriG - SDL Primitive Generator
//    by Jonathan Dearborn
//    
//    Based on SGE: SDL Graphics Extension r030809
//    by Anders Lindström


enum:	eSPGAlphaFlags
	// default = 0
	0		SPG_DEST_ALPHA
	1		SPG_SRC_ALPHA
	2		SPG_COMBINE_ALPHA
	3		SPG_COPY_NO_ALPHA
	4		SPG_COPY_SRC_ALPHA
	5		SPG_COPY_DEST_ALPHA
	6		SPG_COPY_COMBINE_ALPHA
	7		SPG_COPY_ALPHA_ONLY 
	8		SPG_COMBINE_ALPHA_ONLY
	9		SPG_REPLACE_COLORKEY
	// Alternate names:
	4		SPG_SRC_MASK
	5		SPG_DEST_MASK
;enum
	
enum:	eSPGXformFlags
	// Transformation flags
	0		SPG_NONE
	1		SPG_TAA
	2		SPG_TSAFE
	4		SPG_TTMAP
	8		SPG_TSLOW
	16		SPG_TCOLORKEY
	32		SPG_TBLEND
	64		SPG_TSURFACE_ALPHA
	
;enum



//*  Older versions of SDL don't have SDL_CreateRGBSurface
//	 SDL_CreateRGBSurface  SDL_AllocSurface

// Macro to get clipping

//	#define SPG_CLIP_XMIN(pnt) pnt->clip_rect.x
//	#define SPG_CLIP_XMAX(pnt) pnt->clip_rect.x + pnt->clip_rect.w-1
//	#define SPG_CLIP_YMIN(pnt) pnt->clip_rect.y
//	#define SPG_CLIP_YMAX(pnt) pnt->clip_rect.y + pnt->clip_rect.h-1


//  We need to use alpha sometimes but older versions of SDL don't have
//  alpha support.
//	#define SPG_MapRGBA SDL_MapRGBA
//	#define SPG_GetRGBA SDL_GetRGBA


struct: SPG_Point
    float x
    float y
;struct

// A table of dirtyrects for one display page
struct: SPG_DirtyTable
	short				size	//* Table size */
	ptrTo SDL_Rect		rects	//* Table of rects */
	short				count	//* # of rects currently used */
	short				best	//* Merge testing starts here! */
;struct


// #define SPG_bool Uint8

// MISC
// SDL_Surface* SPG_InitSDL(Uint16 w, Uint16 h, Uint8 bitsperpixel, Uint32 systemFlags, Uint32 screenFlags);
dll_5	SPG_InitSDL

// void SPG_EnableAutolock(SPG_bool enable);
// SPG_bool SPG_GetAutolock();
DLLVoid dll_1	SPG_EnableAutolock
dll_0	SPG_GetAutolock

// void SPG_EnableRadians(SPG_bool enable);
// SPG_bool SPG_GetRadians();
DLLVoid dll_1	SPG_EnableRadians
dll_0	SPG_GetRadians

// void SPG_Error(const char* err);
// void SPG_EnableErrors(SPG_bool enable);
// char* SPG_GetError();
// Uint16 SPG_NumErrors();
DLLVoid dll_1	SPG_Error
DLLVoid dll_1	SPG_EnableErrors
dll_0	SPG_GetError
dll_0	SPG_NumErrors

// void SPG_PushThickness(Uint16 state);
// Uint16 SPG_PopThickness();
// Uint16 SPG_GetThickness();
// void SPG_PushBlend(Uint8 state);
// Uint8 SPG_PopBlend();
// Uint8 SPG_GetBlend();
// void SPG_PushAA(SPG_bool state);
// SPG_bool SPG_PopAA();
// SPG_bool SPG_GetAA();
// void SPG_PushSurfaceAlpha(SPG_bool state);
// SPG_bool SPG_PopSurfaceAlpha();
// SPG_bool SPG_GetSurfaceAlpha();
DLLVoid dll_1	SPG_PushThickness
dll_0	SPG_PopThickness
dll_0	SPG_GetThickness
DLLVoid dll_1	SPG_PushBlend
dll_0	SPG_PopBlend
dll_0	SPG_GetBlend
DLLVoid dll_1	SPG_PushAA
dll_0	SPG_PopAA
dll_0	SPG_GetAA
DLLVoid dll_1	SPG_PushSurfaceAlpha
dll_1	SPG_PopSurfaceAlpha
dll_1	SPG_GetSurfaceAlpha


// void SPG_RectOR(const SDL_Rect rect1, const SDL_Rect rect2, SDL_Rect* dst_rect);
// SPG_bool SPG_RectAND(const SDL_Rect A, const SDL_Rect B, SDL_Rect* intersection);
DLLVoid dll_3	SPG_RectOR
dll_3	SPG_RectAND

// DIRTY RECT
//  Important stuff
// void SPG_EnableDirty(SPG_bool enable);
// void SPG_DirtyInit(Uint16 maxsize);
// void SPG_DirtyAdd(SDL_Rect* rect);
// SPG_DirtyTable* SPG_DirtyUpdate(SDL_Surface* screen);
// void SPG_DirtySwap();
DLLVoid dll_1	SPG_EnableDirty
DLLVoid dll_1	SPG_DirtyInit
DLLVoid dll_1	SPG_DirtyAdd
dll_1	SPG_DirtyUpdate
DLLVoid dll_0	SPG_DirtySwap

//  Other stuff
// SPG_bool SPG_DirtyEnabled();
// SPG_DirtyTable* SPG_DirtyMake(Uint16 maxsize);
// void SPG_DirtyAddTo(SPG_DirtyTable* table, SDL_Rect* rect);
// void SPG_DirtyFree(SPG_DirtyTable* table);
// SPG_DirtyTable* SPG_DirtyGet();
// void SPG_DirtyClear(SPG_DirtyTable* table);
// void SPG_DirtyLevel(Uint16 optimizationLevel);
// void SPG_DirtyClip(SDL_Surface* screen, SDL_Rect* rect);
dll_0	SPG_DirtyEnabled
dll_1	SPG_DirtyMake
DLLVoid dll_2	SPG_DirtyAddTo
DLLVoid dll_1	SPG_DirtyFree
dll_0	SPG_DirtyGet
DLLVoid dll_1	SPG_DirtyClear
DLLVoid dll_1	SPG_DirtyLevel
DLLVoid dll_2	SPG_DirtyClip

// PALETTE
// SDL_Color* SPG_ColorPalette();
// SDL_Color* SPG_GrayPalette();
// Uint32 SPG_FindPaletteColor(SDL_Palette* palette, Uint8 r, Uint8 g, Uint8 b);
// SDL_Surface* SPG_PalettizeSurface(SDL_Surface* surface, SDL_Palette* palette);
dll_0	SPG_ColorPalette
dll_0	SPG_GrayPalette
dll_4	SPG_FindPaletteColor
dll_2	SPG_PalettizeSurface

// void SPG_FadedPalette32(SDL_PixelFormat* format, Uint32 color1, Uint32 color2, Uint32* colorArray, Uint16 startIndex, Uint16 stopIndex);
// void SPG_FadedPalette32Alpha(SDL_PixelFormat* format, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2, Uint32* colorArray, Uint16 start, Uint16 stop);
// void SPG_RainbowPalette32(SDL_PixelFormat* format, Uint32 *colorArray, Uint8 intensity, Uint16 startIndex, Uint16 stopIndex);
// void SPG_GrayPalette32(SDL_PixelFormat* format, Uint32 *colorArray, Uint16 startIndex, Uint16 stopIndex);
DLLVoid dll_6	SPG_FadedPalette32
DLLVoid dll_8	SPG_FadedPalette32Alpha
DLLVoid dll_5	SPG_RainbowPalette32
DLLVoid dll_4	SPG_GrayPalette32


// SURFACE

// SDL_Surface* SPG_CreateSurface8(Uint32 flags, Uint16 width, Uint16 height);
// Uint32 SPG_GetPixel(SDL_Surface *surface, Sint16 x, Sint16 y);
// void SPG_SetClip(SDL_Surface *surface, const SDL_Rect rect);
dll_3	SPG_CreateSurface8
dll_3	SPG_GetPixel
DLLVoid dll_2	SPG_SetClip

// SDL_Rect SPG_TransformX(SDL_Surface *src, SDL_Surface *dst, float angle, float xscale, float yscale, Uint16 pivotX, Uint16 pivotY, Uint16 destX, Uint16 destY, Uint8 flags);
// SDL_Surface* SPG_Transform(SDL_Surface *src, Uint32 bgColor, float angle, float xscale, float yscale, Uint8 flags);
// SDL_Surface* SPG_Rotate(SDL_Surface *src, float angle, Uint32 bgColor);
// SDL_Surface* SPG_RotateAA(SDL_Surface *src, float angle, Uint32 bgColor);
dll_10	SPG_TransformX
dll_6	SPG_Transform
dll_3	SPG_Rotate
dll_3	SPG_RotateAA

// SDL_Surface* SPG_ReplaceColor(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dest, SDL_Rect* destrect, Uint32 color);
dll_5	SPG_ReplaceColor


// DRAWING

// int SPG_Blit(SDL_Surface *Src, SDL_Rect* srcRect, SDL_Surface *Dest, SDL_Rect* destRect);
// void SPG_SetBlit(void (*blitfn)(SDL_Surface*, SDL_Rect*, SDL_Surface*, SDL_Rect*));
// void (*SPG_GetBlit())(SDL_Surface*, SDL_Rect*, SDL_Surface*, SDL_Rect*);
dll_4	SPG_Blit
DLLVoid dll_1	SPG_SetBlit
DLLVoid dll_0	SPG_GetBlit

// void SPG_FloodFill(SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 color);
//dll_4	SPG_FloodFill
dll_3	SPG_FloodFill


// PRIMITIVES

// void SPG_Pixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color);
// void SPG_PixelBlend(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha);
// void SPG_PixelPattern(SDL_Surface *surface, SDL_Rect target, SPG_bool* pattern, Uint32* colors);
// void SPG_PixelPatternBlend(SDL_Surface *surface, SDL_Rect target, SPG_bool* pattern, Uint32* colors, Uint8* pixelAlpha);
DLLVoid dll_4	SPG_Pixel
DLLVoid dll_5	SPG_PixelBlend
DLLVoid dll_4	SPG_PixelPattern
DLLVoid dll_5	SPG_PixelPatternBlend


// void SPG_LineH(SDL_Surface *surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 Color);
// void SPG_LineHBlend(SDL_Surface *surface, Sint16 x1, Sint16 y, Sint16 x2, Uint32 color, Uint8 alpha);
DLLVoid dll_5	SPG_LineH
DLLVoid dll_6	SPG_LineHBlend

// void SPG_LineHFade(SDL_Surface *dest,Sint16 x1,Sint16 y,Sint16 x2,Uint32 color1, Uint32 color2);
// void SPG_LineHTex(SDL_Surface *dest,Sint16 x1,Sint16 y,Sint16 x2,SDL_Surface *source,Sint16 sx1,Sint16 sy1,Sint16 sx2,Sint16 sy2);
DLLVoid dll_6	SPG_LineHFade
DLLVoid dll_9	SPG_LineHTex



// void SPG_LineV(SDL_Surface *surface, Sint16 x, Sint16 y1, Sint16 y2, Uint32 Color);
// void SPG_LineVBlend(SDL_Surface *surface, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color, Uint8 alpha);
DLLVoid dll_5	SPG_LineV
DLLVoid dll_6	SPG_LineVBlend

// void SPG_LineFn(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 Color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color));
// void SPG_Line(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 Color);
// void SPG_LineBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_LineFn
DLLVoid dll_6	SPG_Line
DLLVoid dll_7	SPG_LineBlend

// void SPG_LineFadeFn(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint32 color2, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color));
// void SPG_LineFade(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint32 color2);
// void SPG_LineFadeBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color1, Uint8 alpha1, Uint32 color2, Uint8 alpha2);
DLLVoid dll_8	SPG_LineFadeFn
DLLVoid dll_7	SPG_LineFade
DLLVoid dll_9	SPG_LineFadeBlend


// void SPG_Rect(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
// void SPG_RectBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha);
DLLVoid dll_6	SPG_Rect
DLLVoid dll_7	SPG_RectBlend

// void SPG_RectFilled(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
// void SPG_RectFilledBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha);
DLLVoid dll_6	SPG_RectFilled
DLLVoid dll_7	SPG_RectFilledBlend


// void SPG_RectRound(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color);
// void SPG_RectRoundBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_RectRound
DLLVoid dll_8	SPG_RectRoundBlend

// void SPG_RectRoundFilled(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color);
// void SPG_RectRoundFilledBlend(SDL_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, float r, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_RectRoundFilled
DLLVoid dll_8	SPG_RectRoundFilledBlend


// void SPG_EllipseFn(SDL_Surface *surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color));
// void SPG_Ellipse(SDL_Surface *surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color);
// void SPG_EllipseBlend(SDL_Surface *surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_EllipseFn
DLLVoid dll_6	SPG_Ellipse
DLLVoid dll_7	SPG_EllipseBlend

// void SPG_EllipseFilled(SDL_Surface *surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color);
// void SPG_EllipseFilledBlend(SDL_Surface *surface, Sint16 x, Sint16 y, float rx, float ry, Uint32 color, Uint8 alpha);
DLLVoid dll_6	SPG_EllipseFilled
DLLVoid dll_7	SPG_EllipseFilledBlend


// void SPG_EllipseArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color);
// void SPG_EllipseBlendArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_EllipseArb
DLLVoid dll_8	SPG_EllipseBlendArb

// void SPG_EllipseFilledArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color);
// void SPG_EllipseFilledBlendArb(SDL_Surface *Surface, Sint16 x, Sint16 y, float rx, float ry, float angle, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_EllipseFilledArb
DLLVoid dll_8	SPG_EllipseFilledBlendArb


// void SPG_CircleFn(SDL_Surface *surface, Sint16 x, Sint16 y, float r, Uint32 color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color));
// void SPG_Circle(SDL_Surface *surface, Sint16 x, Sint16 y, float r, Uint32 color);
// void SPG_CircleBlend(SDL_Surface *surface, Sint16 x, Sint16 y, float r, Uint32 color, Uint8 alpha);
DLLVoid dll_6	SPG_CircleFn
DLLVoid dll_5	SPG_Circle
DLLVoid dll_6	SPG_CircleBlend

// void SPG_CircleFilled(SDL_Surface *surface, Sint16 x, Sint16 y, float r, Uint32 color);
// void SPG_CircleFilledBlend(SDL_Surface *surface, Sint16 x, Sint16 y, float r, Uint32 color, Uint8 alpha);
DLLVoid dll_5	SPG_CircleFilled
DLLVoid dll_6	SPG_CircleFilledBlend


// void SPG_ArcFn(SDL_Surface* surface, Sint16 cx, Sint16 cy, float radius, float startAngle, float endAngle, Uint32 color, void Callback(SDL_Surface *Surf, Sint16 X, Sint16 Y, Uint32 Color));
// void SPG_Arc(SDL_Surface* surface, Sint16 x, Sint16 y, float radius, float startAngle, float endAngle, Uint32 color);
// void SPG_ArcBlend(SDL_Surface* surface, Sint16 x, Sint16 y, float radius, float startAngle, float endAngle, Uint32 color, Uint8 alpha);
DLLVoid dll_8	SPG_ArcFn
DLLVoid dll_7	SPG_Arc
DLLVoid dll_8	SPG_ArcBlend

// void SPG_ArcFilled(SDL_Surface* surface, Sint16 cx, Sint16 cy, float radius, float startAngle, float endAngle, Uint32 color);
// void SPG_ArcFilledBlend(SDL_Surface* surface, Sint16 cx, Sint16 cy, float radius, float startAngle, float endAngle, Uint32 color, Uint8 alpha);
DLLVoid dll_7	SPG_ArcFilled
DLLVoid dll_8	SPG_ArcFilledBlend


// void SPG_Bezier(SDL_Surface *surface, Sint16 startX, Sint16 startY, Sint16 cx1, Sint16 cy1, Sint16 cx2, Sint16 cy2, Sint16 endX, Sint16 endY, Uint8 quality, Uint32 color);
// void SPG_BezierBlend(SDL_Surface *surface, Sint16 startX, Sint16 startY, Sint16 cx1, Sint16 cy1, Sint16 cx2, Sint16 cy2, Sint16 endX, Sint16 endY, Uint8 quality, Uint32 color, Uint8 alpha);
DLLVoid dll_11 SPG_Bezier
DLLVoid dll_12 SPG_BezierBlend


// POLYGONS

// void SPG_Trigon(SDL_Surface *surface,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color);
// void SPG_TrigonBlend(SDL_Surface *surface,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color, Uint8 alpha);
DLLVoid dll_8	SPG_Trigon
DLLVoid dll_9	SPG_TrigonBlend

// void SPG_TrigonFilled(SDL_Surface *surface,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color);
// void SPG_TrigonFilledBlend(SDL_Surface *surface,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color, Uint8 alpha);
DLLVoid dll_8	SPG_TrigonFilled
DLLVoid dll_9	SPG_TrigonFilledBlend

// void SPG_TrigonFade(SDL_Surface *surface,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,Uint32 color1,Uint32 color2,Uint32 color3);
// void SPG_TrigonTex(SDL_Surface *dest,Sint16 x1,Sint16 y1,Sint16 x2,Sint16 y2,Sint16 x3,Sint16 y3,
//                             SDL_Surface *source,Sint16 sx1,Sint16 sy1,Sint16 sx2,Sint16 sy2,Sint16 sx3,Sint16 sy3);
DLLVoid dll_10	SPG_TrigonFade
DLLVoid dll_14	SPG_TrigonTex


// void SPG_QuadTex(SDL_Surface* dest, Sint16 destULx, Sint16 destULy, Sint16 destDLx, Sint16 destDLy, Sint16 destDRx, Sint16 destDRy, Sint16 destURx, Sint16 destURy,
//                           SDL_Surface* source, Sint16 srcULx, Sint16 srcULy, Sint16 srcDLx, Sint16 srcDLy, Sint16 srcDRx, Sint16 srcDRy, Sint16 srcURx, Sint16 srcURy);
DLLVoid 18 _addDLLEntry	SPG_QuadTex

// void SPG_Polygon(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color);
// void SPG_PolygonBlend(SDL_Surface *dest, Uint16 n, SPG_Point* points, Uint32 color, Uint8 alpha);
DLLVoid dll_4	SPG_Polygon
DLLVoid dll_5	SPG_PolygonBlend

// void SPG_PolygonFilled(SDL_Surface *surface, Uint16 n, SPG_Point* points, Uint32 color);
// void SPG_PolygonFilledBlend(SDL_Surface *surface, Uint16 n, SPG_Point* points, Uint32 color, Uint8 alpha);
DLLVoid dll_4	SPG_PolygonFilled
DLLVoid dll_5	SPG_PolygonFilledBlend

// void SPG_PolygonFade(SDL_Surface *surface, Uint16 n, SPG_Point* points, Uint32* colors);
// void SPG_PolygonFadeBlend(SDL_Surface *surface, Uint16 n, SPG_Point* points, Uint32* colors, Uint8 alpha);
DLLVoid dll_4	SPG_PolygonFade
DLLVoid dll_5	SPG_PolygonFadeBlend

// void SPG_CopyPoints(Uint16 n, SPG_Point* points, SPG_Point* buffer);
// void SPG_RotatePointsXY(Uint16 n, SPG_Point* points, float cx, float cy, float angle);
// void SPG_ScalePointsXY(Uint16 n, SPG_Point* points, float cx, float cy, float xscale, float yscale);
// void SPG_SkewPointsXY(Uint16 n, SPG_Point* points, float cx, float cy, float xskew, float yskew);
// void SPG_TranslatePoints(Uint16 n, SPG_Point* points, float x, float y);
DLLVoid dll_3	SPG_CopyPoints
DLLVoid dll_5	SPG_RotatePointsXY
DLLVoid dll_6	SPG_ScalePointsXY
DLLVoid dll_6	SPG_SkewPointsXY
DLLVoid dll_4	SPG_TranslatePoints


// void SPG_FloodFill(SDL_Surface* dest, Sint16 x, Sint16 y, Uint32 newColor);
DLLVoid dll_4	SPG_FloodFill


previous definitions
