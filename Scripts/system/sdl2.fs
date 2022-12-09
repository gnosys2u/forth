
autoforget sdl2

#if WINDOWS
DLLVocabulary sdl2 SDL2.dll
#else
DLLVocabulary sdl2 libSDL2.so
#endif

also sdl2 definitions


//****************** SDL.h

// flags for SDL_Init, SDL_InitSubSystem & SDL_QuitSubSystem
enum: eSDLInit
	0x00000001      SDL_INIT_TIMER
	0x00000010      SDL_INIT_AUDIO
	0x00000020      SDL_INIT_VIDEO          // SDL_INIT_VIDEO implies SDL_INIT_EVENTS
	0x00000200      SDL_INIT_JOYSTICK       // SDL_INIT_JOYSTICK implies SDL_INIT_EVENTS
	0x00001000      SDL_INIT_HAPTIC
	0x00002000      SDL_INIT_GAMECONTROLLER // SDL_INIT_GAMECONTROLLER implies SDL_INIT_JOYSTICK
	0x00004000      SDL_INIT_EVENTS
	0x00100000      SDL_INIT_NOPARACHUTE    // Don't catch fatal signals
  
  SDL_INIT_TIMER SDL_INIT_AUDIO or SDL_INIT_VIDEO or SDL_INIT_EVENTS or
  SDL_INIT_JOYSTICK or SDL_INIT_HAPTIC or SDL_INIT_GAMECONTROLLER or
                  SDL_INIT_EVERYTHING
;enum


// This function initializes  the subsystems specified by flags
dll_1           SDL_Init    // Uint32 flags

// This function initializes specific SDL subsystems
dll_1           SDL_InitSubSystem    // Uint32 flags

// This function cleans up specific SDL subsystems
DLLVoid dll_1   SDL_QuitSubSystem    // Uint32 flags

// This function returns a mask of the specified subsystems which have previously been initialized.
dll_1           SDL_WasInit    // Uint32 flags

// This function cleans up all initialized subsystems. You should call it upon all exit conditions.
DLLVoid dll_0   SDL_Quit


//****************** SDL_error.h
// SDL_SetError() unconditionally returns -1.
//int SDLCALL SDL_SetError(const char *fmt, ...);

dll_0           SDL_GetError
DLLVoid dll_0   SDL_ClearError


//****************** SDL_pixels.h

enum: SDL_MiscPixelEnums
  0       SDL_FALSE
  1       SDL_TRUE
  0xff    SDL_ALPHA_OPAQUE
  0       SDL_ALPHA_TRANSPARENT

  // Pixel type.
  0 SDL_PIXELTYPE_UNKNOWN
  SDL_PIXELTYPE_INDEX1
  SDL_PIXELTYPE_INDEX4
  SDL_PIXELTYPE_INDEX8
  SDL_PIXELTYPE_PACKED8
  SDL_PIXELTYPE_PACKED16
  SDL_PIXELTYPE_PACKED32
  SDL_PIXELTYPE_ARRAYU8
  SDL_PIXELTYPE_ARRAYU16
  SDL_PIXELTYPE_ARRAYU32
  SDL_PIXELTYPE_ARRAYF16
  SDL_PIXELTYPE_ARRAYF32

  // Bitmap pixel order, high bit -> low bit. 
  0 SDL_BITMAPORDER_NONE
  SDL_BITMAPORDER_4321
  SDL_BITMAPORDER_1234

  // Packed component order, high bit -> low bit.
  0 SDL_PACKEDORDER_NONE
  SDL_PACKEDORDER_XRGB
  SDL_PACKEDORDER_RGBX
  SDL_PACKEDORDER_ARGB
  SDL_PACKEDORDER_RGBA
  SDL_PACKEDORDER_XBGR
  SDL_PACKEDORDER_BGRX
  SDL_PACKEDORDER_ABGR
  SDL_PACKEDORDER_BGRA

  // Array component order, low byte -> high byte.
  0 SDL_ARRAYORDER_NONE
  SDL_ARRAYORDER_RGB
  SDL_ARRAYORDER_RGBA
  SDL_ARRAYORDER_ARGB
  SDL_ARRAYORDER_BGR
  SDL_ARRAYORDER_BGRA
  SDL_ARRAYORDER_ABGR

  // Packed component layout.
  0 SDL_PACKEDLAYOUT_NONE
  SDL_PACKEDLAYOUT_332
  SDL_PACKEDLAYOUT_4444
  SDL_PACKEDLAYOUT_1555
  SDL_PACKEDLAYOUT_5551
  SDL_PACKEDLAYOUT_565
  SDL_PACKEDLAYOUT_8888
  SDL_PACKEDLAYOUT_2101010
  SDL_PACKEDLAYOUT_1010102
;enum

: SDL_DEFINE_PIXELFORMAT    // type, order, layout, bits, bytes
  swap 8 lshift or                        // bytes | (bits <<8)
  swap 16 lshift or swap 20 lshift or     // | (layout << 16) | (order << 20)
  swap 24 lshift or 0x10000000 or         // | (type << 24) | (1 << 28)
;

enum: SDL_PixelFormats
  0
    SDL_PIXELFORMAT_UNKNOWN
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_INDEX1 SDL_BITMAPORDER_4321 0 1 0)
    SDL_PIXELFORMAT_INDEX1LSB
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_INDEX1 SDL_BITMAPORDER_1234 0 1 0)
    SDL_PIXELFORMAT_INDEX1MSB
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_INDEX4 SDL_BITMAPORDER_4321 0 4 0)
    SDL_PIXELFORMAT_INDEX4LSB
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_INDEX4 SDL_BITMAPORDER_1234 0 4 0)
    SDL_PIXELFORMAT_INDEX4MSB
    
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_INDEX8 0 0 8 1)
    SDL_PIXELFORMAT_INDEX8
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED8 SDL_PACKEDORDER_XRGB SDL_PACKEDLAYOUT_332 8 1)
    SDL_PIXELFORMAT_RGB332
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_XRGB SDL_PACKEDLAYOUT_4444 12 2)
    SDL_PIXELFORMAT_RGB444
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_XRGB SDL_PACKEDLAYOUT_1555 15 2)
    SDL_PIXELFORMAT_RGB555
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_XBGR SDL_PACKEDLAYOUT_1555 15 2)
    SDL_PIXELFORMAT_BGR555
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_ARGB SDL_PACKEDLAYOUT_4444 16 2)
    SDL_PIXELFORMAT_ARGB4444
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_RGBA SDL_PACKEDLAYOUT_4444 16 2)
    SDL_PIXELFORMAT_RGBA4444
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_ABGR SDL_PACKEDLAYOUT_4444 16 2)
    SDL_PIXELFORMAT_ABGR4444
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_BGRA SDL_PACKEDLAYOUT_4444 16 2)
    SDL_PIXELFORMAT_BGRA4444
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_ARGB SDL_PACKEDLAYOUT_1555 16 2)
    SDL_PIXELFORMAT_ARGB1555
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_RGBA SDL_PACKEDLAYOUT_5551 16 2)
    SDL_PIXELFORMAT_RGBA5551
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_ABGR SDL_PACKEDLAYOUT_1555 16 2)
    SDL_PIXELFORMAT_ABGR1555
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_BGRA SDL_PACKEDLAYOUT_5551 16 2)
    SDL_PIXELFORMAT_BGRA5551
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_XRGB SDL_PACKEDLAYOUT_565 16 2)
    SDL_PIXELFORMAT_RGB565
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED16 SDL_PACKEDORDER_XBGR SDL_PACKEDLAYOUT_565 16 2)
    SDL_PIXELFORMAT_BGR565
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_ARRAYU8 SDL_ARRAYORDER_RGB 0 24 3)
    SDL_PIXELFORMAT_RGB24
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_ARRAYU8 SDL_ARRAYORDER_BGR 0 24 3)
    SDL_PIXELFORMAT_BGR24
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_XRGB SDL_PACKEDLAYOUT_8888 24 4)
    SDL_PIXELFORMAT_RGB888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_RGBX SDL_PACKEDLAYOUT_8888 24 4)
    SDL_PIXELFORMAT_RGBX8888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_XBGR SDL_PACKEDLAYOUT_8888 24 4)
    SDL_PIXELFORMAT_BGR888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_BGRX SDL_PACKEDLAYOUT_8888 24 4)
    SDL_PIXELFORMAT_BGRX8888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_ARGB SDL_PACKEDLAYOUT_8888 32 4)
    SDL_PIXELFORMAT_ARGB8888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_RGBA SDL_PACKEDLAYOUT_8888 32 4)
    SDL_PIXELFORMAT_RGBA8888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_ABGR SDL_PACKEDLAYOUT_8888 32 4)
    SDL_PIXELFORMAT_ABGR8888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_BGRA SDL_PACKEDLAYOUT_8888 32 4)
    SDL_PIXELFORMAT_BGRA8888
  SDL_DEFINE_PIXELFORMAT(SDL_PIXELTYPE_PACKED32 SDL_PACKEDORDER_ARGB SDL_PACKEDLAYOUT_2101010 32 4)
    SDL_PIXELFORMAT_ARGB2101010

  `YV12` SDL_PIXELFORMAT_YV12
  `IYUV` SDL_PIXELFORMAT_IYUV
  `YUY2` SDL_PIXELFORMAT_YUY2
  `UYVY` SDL_PIXELFORMAT_UYVY
  `YVYU` SDL_PIXELFORMAT_YVYU
;enum

struct: SDL_Color
  ubyte r
  ubyte g
  ubyte b
  ubyte a
;struct

struct: SDL_Palette
  int               ncolors
  ptrTo SDL_Color   colors
  uint              version
  int               refcount
;struct

// Everything in the pixel format structure is read-only
struct: SDL_PixelFormat recursive
  int format
  ptrTo SDL_Palette palette
  ubyte   BitsPerPixel
  ubyte   BytesPerPixel
  2 arrayOf ubyte padding
  uint    Rmask
  uint    Gmask
  uint    Bmask
  uint    Amask
  ubyte   Rloss
  ubyte   Gloss
  ubyte   Bloss
  ubyte   Aloss
  ubyte   Rshift
  ubyte   Gshift
  ubyte   Bshift
  ubyte   Ashift
  int     refcount
  ptrTo SDL_PixelFormat next
;struct

// Get the human readable name of a pixel format (returns const char*)
dll_1           SDL_GetPixelFormatName    // Uint32 format

// Convert one of the enumerated pixel formats to a bpp and RGBA masks.
// return SDL_TRUE, or SDL_FALSE if the conversion wasn't possible.
dll_6           SDL_PixelFormatEnumToMasks    // Uint32 format, int *bpp, Uint32 *Rmask, Uint32 *Gmask, Uint32 *Bmask, Uint32 *Amask

// Convert a bpp and RGBA masks to an enumerated pixel format.
// returns The pixel format, or ::SDL_PIXELFORMAT_UNKNOWN if the conversion wasn't possible.
dll_5           SDL_MasksToPixelFormatEnum    // int bpp, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask

// Create an SDL_PixelFormat structure from a pixel format enum. (returns SDL_PixelFormat *)
dll_1           SDL_AllocFormat   // Uint32 pixel_format

// Free an SDL_PixelFormat structure.
DLLVoid dll_1   SDL_FreeFormat    // SDL_PixelFormat *format

// Create a palette structure with the specified number of color entries.
// return a new palette, or NULL if there wasn't enough memory. The palette entries are initialized to white.
dll_1           SDL_AllocPalette    // int ncolors

// Set the palette for a pixel format structure.
dll_2           SDL_SetPixelFormatPalette   // SDL_PixelFormat *format, SDL_Palette *palette

// Set a range of colors in a palette.  Returns 0 for success.
dll_4           SDL_SetPaletteColors        // SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors

// Free a palette created with SDL_AllocPalette().
DLLVoid dll_1   SDL_FreePalette             // SDL_Palette *palette

// Maps an RGB triple to an opaque pixel value for a given pixel format.
dll_4           SDL_MapRGB                  // const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b

// Maps an RGBA quadruple to a pixel value for a given pixel format.
dll_5           SDL_MapRGBA                 // const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b, Uint8 a

// Get the RGB components from a pixel of the specified format.
DLLVoid dll_5   SDL_GetRGB                  // Uint32 pixel, const SDL_PixelFormat *format, Uint8 *r, Uint8 *g, Uint8 *b

// Get the RGBA components from a pixel of the specified format.
DLLVoid dll_6   SDL_GetRGBA                 // Uint32 pixel, const SDL_PixelFormat *format, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a

// Calculate a 256 entry gamma ramp for a gamma value.
DLLVoid dll_2   SDL_CalculateGammaRamp        // float gamma, Uint16 *ramp


//****************** SDL_rect.h

struct: SDL_Rect
	int x		int y
	int w		int h
;struct

struct: SDL_Point
	int x		int y
;struct

// Returns true if the rectangle has no area.
: SDL_RectEmpty   // const SDL_Rect *r
  -> ptrTo SDL_Rect r
  
  if( r 0<> r.w 0> and r.h 0> and )
    SDL_TRUE
  else
    SDL_FALSE
  endif
;

// Returns true if the two rectangles are equal.
: SDL_RectEquals    // const SDL_Rect *a  const SDL_Rect *b
  -> ptrTo SDL_Rect b
  -> ptrTo SDL_Rect a
  
  SDL_FALSE
  if( a 0<> b 0<> and)
    if( a.x b.x = a.y b.y = and a.w b.w = and a.h b.h = and)
      drop SDL_TRUE
    endif
  endif
;

// Determine whether two rectangles intersect.
dll_2           SDL_HasIntersection         // const SDL_Rect * A, const SDL_Rect * B

// Calculate the intersection of two rectangles.
dll_3           SDL_IntersectRect           // const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result

// Calculate the union of two rectangles.
DLLVoid dll_3   SDL_UnionRect               // const SDL_Rect * A, const SDL_Rect * B, SDL_Rect * result

// Calculate a minimal rectangle enclosing a set of points
//  return SDL_TRUE if any points were within the clipping rect
dll_4           SDL_EnclosePoints           // const SDL_Point * points, int count, const SDL_Rect * clip, SDL_Rect * result

// Calculate the intersection of a rectangle and line segment.
//  return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
dll_5           SDL_IntersectRectAndLine    // const SDL_Rect *rect, int *X1, int *Y1, int *X2, int *Y2


//****************** SDL_surface.h

enum: SDL_SurfaceMiscEnum
  0   SDL_SWSURFACE     // Just here for compatibility 
  1   SDL_PREALLOC      // Surface uses preallocated memory
  2   SDL_RLEACCEL      // Surface is RLE encoded
  4   SDL_DONTFREE      // Surface is referenced internally
;enum


// A collection of pixels used in software blitting.
// This structure should be treated as read-only, except for 'pixels',
//   which, if not NULL, contains the raw pixel data for the surface.

struct: SDL_Surface
  uint flags
  ptrTo SDL_PixelFormat format
  int w
  int h
  int pitch
  ptrTo byte pixels		// read-write
  ptrTo byte userdata
  int locked
  ptrTo byte lock_data
  SDL_Rect clip_rect
  ptrTo byte map        // SDL_BlitMap*, private
  int refcount
;struct

// Evaluates to true if the surface needs to be locked before access.
: SDL_MUSTLOCK
  -> ptrTo SDL_Surface s
  s.flags SDL_RLEACCEL and
;

// The type of function used for surface blitting functions.
// typedef int (*SDL_blit) (struct SDL_Surface * src, SDL_Rect * srcrect,
//                         struct SDL_Surface * dst, SDL_Rect * dstrect);

// Allocate and free an RGB surface. (returns SDL_Surface*)

// If the depth is 4 or 8 bits, an empty palette is allocated for the surface.
// If the depth is greater than 8 bits, the pixel format is set using the
// flags '[RGB]mask'.
// If the function runs out of memory, it will return NULL.
dll_8           SDL_CreateRGBSurface      // Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask
dll_8           SDL_CreateRGBSurfaceFrom  // void *pixels, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask

DLLVoid dll_1   SDL_FreeSurface           // SDL_Surface * surface

// Set the palette used by a surface.
// note: A single palette can be shared with many surfaces.
dll_2           SDL_SetSurfacePalette     // SDL_Surface * surface, SDL_Palette * palette

// Sets up a surface for directly accessing the pixels.
dll_1           SDL_LockSurface           // SDL_Surface * surface
DLLVoid dll_1   SDL_UnlockSurface         // SDL_Surface * surface

// Load a surface from a seekable SDL data stream (memory or file). (returns SDL_Surface * or NULL)
// If freesrc is non-zero, the stream will be closed after being read.
dll_2           SDL_LoadBMP_RW            // SDL_RWops * src, int freesrc

//  Load a surface from a file.
//#define SDL_LoadBMP(file)   SDL_LoadBMP_RW(SDL_RWFromFile(file, "rb"), 1)

// Save a surface to a seekable SDL data stream (memory or file).
dll_3           SDL_SaveBMP_RW            // SDL_Surface * surface, SDL_RWops * dst, int freedst

//  Save a surface to a file.
//#define SDL_SaveBMP(surface, file) SDL_SaveBMP_RW(surface, SDL_RWFromFile(file, "wb"), 1)

// Sets the RLE acceleration hint for a surface.
dll_2           SDL_SetSurfaceRLE     //SDL_Surface * surface, int flag

// Sets the color key (transparent pixel) in a blittable surface.
// key The transparent pixel in the native surface format
// You can pass SDL_RLEACCEL to enable RLE accelerated blits.
dll_3           SDL_SetColorKey     // SDL_Surface * surface, int enableColorKey, Uint32 key

// Gets the color key (transparent pixel) in a blittable surface.
dll_2           SDL_GetColorKey     // SDL_Surface * surface, Uint32 * key

// Set an additional color value used in blit operations.
dll_4           SDL_SetSurfaceColorMod    // SDL_Surface * surface, Uint8 r, Uint8 g, Uint8 b


// Get the additional color value used in blit operations.
dll_4           SDL_GetSurfaceColorMod    // SDL_Surface * surface, Uint8* r, Uint8* g, Uint8* b

// Set an additional alpha value used in blit operations.
dll_2           SDL_SetSurfaceAlphaMod    // SDL_Surface * surface, Uint8 alpha

// Get the additional alpha value used in blit operations.
dll_2           SDL_GetSurfaceAlphaMod    // SDL_Surface * surface, Uint8 * alpha

// Set the blend mode used for blit operations.
// blendMode ::SDL_BlendMode to use for blit blending.
dll_2           SDL_SetSurfaceBlendMode     // SDL_Surface * surface, SDL_BlendMode blendMode

// Get the blend mode used for blit operations.
dll_2           SDL_GetSurfaceBlendMode     // SDL_Surface * surface, SDL_BlendMode *blendMode

// Sets the clipping rectangle for the destination surface in a blit.
// If the clip rectangle is NULL, clipping will be disabled.
// Returns SDL_TRUE If the clip rectangle intersects the surface at all.
dll_2           SDL_SetClipRect     // SDL_Surface * surface, const SDL_Rect * rect

// Gets the clipping rectangle for the destination surface in a blit.
DLLVoid dll_2   SDL_GetClipRect // SDL_Surface * surface, SDL_Rect * rect

// Creates a new surface of the specified format, and then copies and maps
// the given surface to it so the blit of the converted surface will be as
// fast as possible.  If this function fails, it returns NULL. (returns SDL_Surface *)
dll_3           SDL_ConvertSurface        // SDL_Surface * src, const SDL_PixelFormat* fmt, Uint32 flags
dll_3           SDL_ConvertSurfaceFormat  // SDL_Surface * src, Uint32 pixel_format, Uint32 flags

// Copy a block of pixels of one format to another format
dll_8           SDL_ConvertPixels       // int width, int height, Uint32 src_format, const void * src, int src_pitch,
                                        // Uint32 dst_format, void * dst, int dst_pitch

// Performs a fast fill of the given rectangle with color.
// If rect is NULL, the whole surface will be filled with color.
dll_3           SDL_FillRect    // SDL_Surface * dst, const SDL_Rect * rect, Uint32 color
dll_4           SDL_FillRects   // SDL_Surface * dst, const SDL_Rect * rects, int count, Uint32 color

// Performs a fast blit from the source surface to the destination surface.
// This assumes that the source and destination rectangles are
// the same size.  If either srcrect or dstrect are NULL, the entire
// surface (src or dst) is copied.  The final blit rectangles are saved
// in srcrect and dstrect after all clipping is performed.
// The blit function should not be called on a locked surface.
dll_4           SDL_UpperBlit     // SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect
: SDL_BlitSurface SDL_UpperBlit ;

dll_4           SDL_LowerBlit     // SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect

// Perform a fast, low quality, stretch blit between two surfaces of the same pixel format.
// This function uses a static buffer, and is not thread-safe.
dll_4           SDL_SoftStretch   // SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, const SDL_Rect * dstrect

// This is the public scaled blit function, SDL_BlitScaled(), and it performs rectangle validation and clipping before passing it to SDL_LowerBlitScaled()
dll_4           SDL_UpperBlitScaled   // SDL_Surface * src, const SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect
: SDL_BlitScaled SDL_UpperBlitScaled ;

// This is a semi-private blit function and it performs low-level surface scaled blitting only.
dll_4           SDL_LowerBlitScaled     // SDL_Surface * src, SDL_Rect * srcrect, SDL_Surface * dst, SDL_Rect * dstrect


//****************** SDL_video.h

struct: SDL_DisplayMode
  uint format         // pixel format
  int w               // width
  int h               // height
  int refresh_rate    // refresh rate (or zero for unspecified)
  int driverdata      // driver-specific data, initialize to 0 (actually a void*)
;struct

enum: SDL_WindowFlags
  0x00000001    SDL_WINDOW_FULLSCREEN         // fullscreen window
  0x00000002    SDL_WINDOW_OPENGL             // window usable with OpenGL context
  0x00000004    SDL_WINDOW_SHOWN              // window is visible
  0x00000008    SDL_WINDOW_HIDDEN             // window is not visible
  0x00000010    SDL_WINDOW_BORDERLESS         // no window decoration
  0x00000020    SDL_WINDOW_RESIZABLE          // window can be resized
  0x00000040    SDL_WINDOW_MINIMIZED          // window is minimized
  0x00000080    SDL_WINDOW_MAXIMIZED          // window is maximized
  0x00000100    SDL_WINDOW_INPUT_GRABBED      // window has grabbed input focus
  0x00000200    SDL_WINDOW_INPUT_FOCUS        // window has input focus
  0x00000400    SDL_WINDOW_MOUSE_FOCUS        // window has mouse focus
  SDL_WINDOW_FULLSCREEN 0x00001000 or
                SDL_WINDOW_FULLSCREEN_DESKTOP
  0x00000800    SDL_WINDOW_FOREIGN            // window not created by SDL
  0x00002000    SDL_WINDOW_ALLOW_HIGHDPI        // window should be created in high-DPI mode if supported
;enum

// Used to indicate that you don't care what the window position is.
0x1FFF0000 constant SDL_WINDOWPOS_UNDEFINED_MASK    

// Used to indicate that the window position should be centered.
0x2FFF0000 constant SDL_WINDOWPOS_CENTERED_MASK

// Event subtype for window events
enum: SDL_WindowEventID
    SDL_WINDOWEVENT_NONE
    SDL_WINDOWEVENT_SHOWN
    SDL_WINDOWEVENT_HIDDEN
    SDL_WINDOWEVENT_EXPOSED
    SDL_WINDOWEVENT_MOVED
    SDL_WINDOWEVENT_RESIZED
    SDL_WINDOWEVENT_SIZE_CHANGED
    SDL_WINDOWEVENT_MINIMIZED
    SDL_WINDOWEVENT_MAXIMIZED
    SDL_WINDOWEVENT_RESTORED
    SDL_WINDOWEVENT_ENTER
    SDL_WINDOWEVENT_LEAVE
    SDL_WINDOWEVENT_FOCUS_GAINED
    SDL_WINDOWEVENT_FOCUS_LOST
    SDL_WINDOWEVENT_CLOSE
;enum

// OpenGL configuration attributes
enum: SDL_GLattr
    SDL_GL_RED_SIZE
    SDL_GL_GREEN_SIZE
    SDL_GL_BLUE_SIZE
    SDL_GL_ALPHA_SIZE
    SDL_GL_BUFFER_SIZE
    SDL_GL_DOUBLEBUFFER
    SDL_GL_DEPTH_SIZE
    SDL_GL_STENCIL_SIZE
    SDL_GL_ACCUM_RED_SIZE
    SDL_GL_ACCUM_GREEN_SIZE
    SDL_GL_ACCUM_BLUE_SIZE
    SDL_GL_ACCUM_ALPHA_SIZE
    SDL_GL_STEREO
    SDL_GL_MULTISAMPLEBUFFERS
    SDL_GL_MULTISAMPLESAMPLES
    SDL_GL_ACCELERATED_VISUAL
    SDL_GL_RETAINED_BACKING
    SDL_GL_CONTEXT_MAJOR_VERSION
    SDL_GL_CONTEXT_MINOR_VERSION
    SDL_GL_CONTEXT_EGL
    SDL_GL_CONTEXT_FLAGS
    SDL_GL_CONTEXT_PROFILE_MASK
    SDL_GL_SHARE_WITH_CURRENT_CONTEXT
    SDL_GL_FRAMEBUFFER_SRGB_CAPABLE
;enum

enum: SDL_GLprofile
  0x0001  SDL_GL_CONTEXT_PROFILE_CORE
  0x0002  SDL_GL_CONTEXT_PROFILE_COMPATIBILITY
  0x0004  SDL_GL_CONTEXT_PROFILE_ES
;enum

enum: SDL_GLcontextFlag
  0x0001  SDL_GL_CONTEXT_DEBUG_FLAG
  0x0002  SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
  0x0004  SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG
  0x0008  SDL_GL_CONTEXT_RESET_ISOLATION_FLAG
;enum


// Get the number of video drivers compiled into SDL
dll_0 SDL_GetNumVideoDrivers

// Get the name of a built in video driver.
dll_1 SDL_GetVideoDriver

// Initialize the video subsystem, optionally specifying a video driver.
dll_1 SDL_VideoInit     // const char *driver_name

// Shuts down the video subsystem.
DLLVoid dll_0 SDL_VideoQuit

// Returns the name of the currently initialized video driver.
dll_0 SDL_GetCurrentVideoDriver

// Returns the number of available video displays.
dll_0  SDL_GetNumVideoDisplays

// Get the name of a display in UTF-8 encoding
dll_1  SDL_GetDisplayName   // int displayIndex

// Get the desktop area represented by a display, with the primary
dll_2 SDL_GetDisplayBounds    // int displayIndex, SDL_Rect * rect

// Returns the number of available display modes.
dll_1 SDL_GetNumDisplayModes    // int displayIndex

// Fill in information about a specific display mode.
dll_3 SDL_GetDisplayMode      // int displayIndex, int modeIndex, SDL_DisplayMode * mode

// Fill in information about the desktop display mode.
dll_2 SDL_GetDesktopDisplayMode   // int displayIndex, SDL_DisplayMode * mode

// Fill in information about the current display mode.
dll_2 SDL_GetCurrentDisplayMode   // int displayIndex, SDL_DisplayMode * mode


// Get the closest match to the requested display mode. (returns SDL_DisplayMode *)
dll_3 SDL_GetClosestDisplayMode   // int displayIndex, const SDL_DisplayMode * mode, SDL_DisplayMode * closest

// Get the display index associated with a window.
dll_1 SDL_GetWindowDisplayIndex   // SDL_Window * window

// Set the display mode used when a fullscreen window is visible.
dll_2 SDL_SetWindowDisplayMode    // SDL_Window * window, const SDL_DisplayMode * mode

// Fill in information about the display mode used when a fullscreen
dll_2 SDL_GetWindowDisplayMode    // SDL_Window * window, SDL_DisplayMode * mode

// Get the pixel format associated with the window.
dll_1 SDL_GetWindowPixelFormat  // SDL_Window * window

// Create a window with the specified position, dimensions, and flags. (returns SDL_Window*)
dll_6 SDL_CreateWindow    // const char *title, int x, int y, int w, int h, Uint32 flags

// Create an SDL window from an existing native window. (returns SDL_Window*)
dll_1 SDL_CreateWindowFrom    // const void *data

// Get the numeric ID of a window, for logging purposes.
dll_1 SDL_GetWindowID     // SDL_Window * window

// Get a window from a stored ID, or NULL if it doesn't exist. (returns SDL_Window*)
dll_1 SDL_GetWindowFromID   // Uint32 id

// Get the window flags.
dll_1 SDL_GetWindowFlags    // SDL_Window * window

// Set the title of a window, in UTF-8 format.
DLLVoid dll_2 SDL_SetWindowTitle    // SDL_Window * window, const char *title

// Get the title of a window, in UTF-8 format. (returns const char *)
dll_1 SDL_GetWindowTitle    // SDL_Window * window

// Set the icon for a window.
DLLVoid dll_2 SDL_SetWindowIcon   // SDL_Window * window, SDL_Surface * icon

// Associate an arbitrary named pointer with a window. (returns void*)
dll_3 SDL_SetWindowData   // SDL_Window * window, const char *name, void *userdata

// Retrieve the data pointer associated with a window. (returns void*)
dll_2 SDL_GetWindowData   // SDL_Window * window, const char *name

// Set the position of a window.
DLLVoid dll_3 SDL_SetWindowPosition   // SDL_Window * window, int x, int y

// Get the position of a window.
DLLVoid dll_3 SDL_GetWindowPosition   // SDL_Window * window, int *x, int *y

// Set the size of a window's client area.
DLLVoid dll_3 SDL_SetWindowSize       // SDL_Window * window, int w, int h

// Get the size of a window's client area.
DLLVoid dll_3 SDL_GetWindowSize       // SDL_Window * window, int *w, int *h

// Set the minimum size of a window's client area.
DLLVoid dll_3 SDL_SetWindowMinimumSize  // SDL_Window * window, int min_w, int min_h

// Get the minimum size of a window's client area.
DLLVoid dll_3 SDL_GetWindowMinimumSize  // SDL_Window * window, int *w, int *h

// Set the maximum size of a window's client area.
DLLVoid dll_3 SDL_SetWindowMaximumSize  // SDL_Window * window, int max_w, int max_h

// Get the maximum size of a window's client area.
DLLVoid dll_3 SDL_GetWindowMaximumSize  // SDL_Window * window, int *w, int *h

// Set the border state of a window.
DLLVoid dll_2 SDL_SetWindowBordered     // SDL_Window * window, SDL_bool bordered

// Show a window.
DLLVoid dll_1 SDL_ShowWindow    // SDL_Window * window

// Hide a window.
DLLVoid dll_1 SDL_HideWindow    // SDL_Window * window

// Raise a window above other windows and set the input focus.
DLLVoid dll_1 SDL_RaiseWindow    // SDL_Window * window

// Make a window as large as possible.
DLLVoid dll_1 SDL_MaximizeWindow    // SDL_Window * window

// Minimize a window to an iconic representation.
DLLVoid dll_1 SDL_MinimizeWindow    // SDL_Window * window

// Restore the size and position of a minimized or maximized window.
DLLVoid dll_1 SDL_RestoreWindow    // SDL_Window * window

// Set a window's fullscreen state.  return 0 on success, or -1 if setting the display mode failed.
dll_2 SDL_SetWindowFullscreen       // SDL_Window * window, Uint32 flags

// Get the SDL surface associated with the window.  (returns SDL_Surface *)
dll_1  SDL_GetWindowSurface     // SDL_Window * window

// Copy the window surface to the screen.
dll_1  SDL_UpdateWindowSurface     // SDL_Window * window

// Copy a number of rectangles on the window surface to the screen.
dll_3 SDL_UpdateWindowSurfaceRects    // SDL_Window * window, const SDL_Rect * rects, int numrects

// Set a window's input grab mode.
DLLVoid dll_2 SDL_SetWindowGrab     // SDL_Window * window, SDL_bool grabbed

// Get a window's input grab mode. (return SDL_bool)
dll_1  SDL_GetWindowSurface     // SDL_Window * window

// Set the brightness (gamma correction) for a window.
dll_2 SDL_SetWindowBrightness     // SDL_Window * window, float brightness

// Get the brightness (gamma correction) for a window. (returns float)
dll_1 SDL_GetWindowBrightness     // SDL_Window * window

// Set the gamma ramp for a window.
dll_4 SDL_SetWindowGammaRamp      // SDL_Window * window, const Uint16 * red, const Uint16 * green, const Uint16 * blue

// Get the gamma ramp for a window.
dll_4 SDL_GetWindowGammaRamp      // SDL_Window * window, const Uint16 * red, const Uint16 * green, const Uint16 * blue

// Destroy a window.
dll_1  SDL_DestroyWindow     // SDL_Window * window

// Returns whether the screensaver is currently enabled (default on).
dll_0 SDL_IsScreenSaverEnabled

// Allow the screen to be blanked by a screensaver
DLLVoid dll_0 SDL_EnableScreenSaver

// Prevent the screen from being blanked by a screensaver
DLLVoid dll_0 SDL_DisableScreenSaver


////////////////////////////////////////////////
// OpenGL support functions
////////////////////////////////////////////////

// Dynamically load an OpenGL library.
dll_1 SDL_GL_LoadLibrary    // const char *path

// Get the address of an OpenGL function.
dll_1 SDL_GL_GetProcAddress   // const char *proc

// Unload the OpenGL library previously loaded by SDL_GL_LoadLibrary().
DLLVoid dll_0 SDL_GL_UnloadLibrary

// Return true if an OpenGL extension is supported for the current
dll_1 SDL_GL_ExtensionSupported   // const char *extension

// Reset all previously set OpenGL context attributes to their default values
DLLVoid dll_0 SDL_GL_ResetAttributes

// Set an OpenGL window attribute before window creation.
dll_2 SDL_GL_SetAttribute     // SDL_GLattr attr, int value

// Get the actual value for an attribute from the current context.
dll_2 SDL_GL_GetAttribute     // SDL_GLattr attr, int *value

// Create an OpenGL context for use with an OpenGL window, and make it (returns SDL_GLContext)
dll_1 SDL_GL_CreateContext    // SDL_Window *window

// Set up an OpenGL context for rendering into an OpenGL window.
dll_2 SDL_GL_MakeCurrent      // SDL_Window * window, SDL_GLContext context

// Get the currently active OpenGL window. (returns SDL_Window*)
dll_0 SDL_GL_GetCurrentWindow

// Get the currently active OpenGL context. (returns SDL_GLContext)
dll_0 SDL_GL_GetCurrentContext

// Get the size of a window's underlying drawable (for use with glViewport).
DLLVoid dll_3 SDL_GL_GetDrawableSize      // SDL_Window * window, int *w, int *h

// Set the swap interval for the current OpenGL context.
dll_1 SDL_GL_SetSwapInterval        // int interval

// Get the swap interval for the current OpenGL context.
dll_0 SDL_GL_GetSwapInterval

// Swap the OpenGL buffers for a window, if double-buffering is supported.
DLLVoid dll_1 SDL_GL_SwapWindow     // SDL_Window * window

// Delete an OpenGL context.
DLLVoid dll_1 SDL_GL_DeleteContext  // SDL_GLContext context


//****************** SDL_timer.h

// Get the number of milliseconds since the SDL library initialization.
//   note This value wraps if the program runs for more than ~49 days.
dll_0    SDL_GetTicks

// Get the current value of the high resolution counter
DLLLong dll_0 SDL_GetPerformanceCounter

// Get the count per second of the high resolution counter
DLLLong dll_0   SDL_GetPerformanceFrequency

// Wait a specified number of milliseconds before returning.
DLLVoid dll_1    SDL_Delay    // Uint32 ms



// Function prototype for the timer callback function.
// The callback function is passed the current timer interval and returns
//  the next timer interval.  If the returned value is the same as the one
//  passed in, the periodic alarm continues, otherwise a new alarm is
//  scheduled.  If the callback returns 0, the periodic alarm is cancelled.
// typedef Uint32 (SDLCALL * SDL_TimerCallback) (Uint32 interval, void *param);

// Definition of the timer ID type.
// typedef int SDL_TimerID;

// Add a new timer to the pool of timers already running.
// return A timer ID, or NULL when an error occurs.
dll_3    SDL_AddTimer   // Uint32 interval, SDL_TimerCallback callback, void *param

// Remove a timer knowing its ID.
//  returns A boolean value indicating success or failure.
dll_1    SDL_RemoveTimer  // SDL_TimerID id

//****************** SDL_rwops.h

enum: SDL_RWOPS_MiscEnum
  SDL_RWOPS_UNKNOWN   // Unknown stream type
  SDL_RWOPS_WINFILE   // Win32 file
  SDL_RWOPS_STDFILE   // Stdio file
  SDL_RWOPS_JNIFILE   // Android asset
  SDL_RWOPS_MEMORY    // Memory stream
  SDL_RWOPS_MEMORY_RO // Read-Only memory stream
  
  0 RW_SEEK_SET       // Seek from the beginning of data
  RW_SEEK_CUR       // Seek relative to current read point
  RW_SEEK_END       // Seek relative to the end of data
;enum


// these return SDL_RWops *
dll_2           SDL_RWFromFile    // const char *file, const char *mode

dll_2           SDL_RWFromFP      // FILE * fp, SDL_bool autoclose

dll_2           SDL_RWFromMem       // void *mem, int size
dll_2           SDL_RWFromConstMem  // const void *mem, int size
dll_0           SDL_AllocRW
  
DLLVoid dll_1   SDL_FreeRW          // SDL_RWops * area


// Read an item of the specified endianness and return in native format.
// all take SDL_RWops * src
dll_1           SDL_ReadU8
dll_1           SDL_ReadLE16
dll_1           SDL_ReadBE16
dll_1           SDL_ReadLE32
dll_1           SDL_ReadBE32
DLLLong dll_1   SDL_ReadLE64
DLLLong dll_1   SDL_ReadBE64

// Write an item of native format to the specified endianness.
// all return size_t
dll_2           SDL_WriteU8       // SDL_RWops * dst, Uint8 value
dll_2           SDL_WriteLE16
dll_2           SDL_WriteBE16
dll_2           SDL_WriteLE32
dll_2           SDL_WriteBE32
dll_3           SDL_WriteLE64     // SDL_RWops * dst, Uint64 value
dll_3           SDL_WriteBE64


//****************** SDL_renderer.h

enum: SDL_RENDER_MiscEnum
  1   SDL_RENDERER_SOFTWARE       // The renderer is a software fallback
  2   SDL_RENDERER_ACCELERATED    // The renderer uses hardware acceleration
  4   SDL_RENDERER_PRESENTVSYNC   // Present is synchronized with the refresh rate
  8   SDL_RENDERER_TARGETTEXTURE  // The renderer supports rendering to texture
  
  // The access pattern allowed for a texture.
  0   SDL_TEXTUREACCESS_STATIC      // Changes rarely, not lockable
      SDL_TEXTUREACCESS_STREAMING   // Changes frequently, lockable
      SDL_TEXTUREACCESS_TARGET      // Texture can be used as a render target
      
  // texture channel modulation used in SDL_RenderCopy().
  0   SDL_TEXTUREMODULATE_NONE      // No modulation
      SDL_TEXTUREMODULATE_COLOR     // srcC = srcC * color
      SDL_TEXTUREMODULATE_ALPHA     // srcA = srcA * alpha
    
  // Flip constants for SDL_RenderCopyEx

  0   SDL_FLIP_NONE           // Do not flip
      SDL_FLIP_HORIZONTAL     // flip horizontally
      SDL_FLIP_VERTICAL       // flip vertically
;enum

struct: SDL_RendererInfo
    ptrTo byte  name                  // The name of the renderer
    uint         flags                // Supported ::SDL_RendererFlags
    uint         num_texture_formats  // The number of available texture formats
    16 arrayOf uint texture_formats   // The available texture formats
    int max_texture_width             // The maximimum texture width
    int max_texture_height            // The maximimum texture height
;struct

// A structure representing rendering state
// struct SDL_Renderer;

// An efficient driver-specific representation of pixel data
// struct SDL_Texture;

// Get the number of 2D rendering drivers available for the current display.
dll_0           SDL_GetNumRenderDrivers

// Get information about a specific 2D rendering driver for the current display.
dll_2           SDL_GetRenderDriverInfo   // int index, SDL_RendererInfo * info

// Create a window and default renderer
dll_5           SDL_CreateWindowAndRenderer   // int width, int height, Uint32 window_flags, SDL_Window **window, SDL_Renderer **renderer

// Create a 2D rendering context for a window.  returns SDL_Renderer *
dll_3           SDL_CreateRenderer      // SDL_Window * window, int index, Uint32 flags

// Create a 2D software rendering context for a surface.  returns SDL_Renderer *
dll_1           SDL_CreateSoftwareRenderer    // SDL_Surface * surface

// Get the renderer associated with a window.  returns SDL_Renderer *
dll_1           SDL_GetRenderer         // SDL_Window * window

// Get information about a rendering context.
dll_2           SDL_GetRendererInfo       // SDL_Renderer * renderer, SDL_RendererInfo * info

// Get the output size of a rendering context.
dll_3           SDL_GetRendererOutputSize   // SDL_Renderer * renderer, int *w, int *h

// Create a texture for a rendering context. returns SDL_Texture *
dll_5           SDL_CreateTexture     // SDL_Renderer * renderer, Uint32 format, int access, int w, int h

// Create a texture from an existing surface.  returns SDL_Texture *
dll_2           SDL_CreateTextureFromSurface    // SDL_Renderer * renderer, SDL_Surface * surface

// Query the attributes of a texture
dll_5           SDL_QueryTexture      // SDL_Texture * texture, Uint32 * format, int *access, int *w, int *h

// Set an additional color value used in render copy operations.
dll_4           SDL_SetTextureColorMod    // SDL_Texture * texture, Uint8 r, Uint8 g, Uint8 b

// Get the additional color value used in render copy operations.
dll_4           SDL_GetTextureColorMod    // SDL_Texture * texture, Uint8 * r, Uint8 * g, Uint8 * b

// Set an additional alpha value used in render copy operations.
dll_2           SDL_SetTextureAlphaMod    // SDL_Texture * texture, Uint8 alpha

// Get the additional alpha value used in render copy operations.
dll_2           SDL_GetTextureAlphaMod    // SDL_Texture * texture, Uint8 * alpha

// Set the blend mode used for texture copy operations.
dll_2           SDL_SetTextureBlendMode   // SDL_Texture * texture, SDL_BlendMode blendMode

// Get the blend mode used for texture copy operations.
dll_2           SDL_GetTextureBlendMode   // SDL_Texture * texture, SDL_BlendMode *blendMode

// Update the given texture rectangle with new pixel data.
dll_4           SDL_UpdateTexture         // SDL_Texture * texture, const SDL_Rect * rect, const void *pixels, int pitch

// Update a rectangle within a planar YV12 or IYUV texture with new pixel data.
dll_8           SDL_UpdateYUVTexture  // SDL_Texture * texture, const SDL_Rect * rect, const Uint8 *Yplane, int Ypitch,
                                      //  const Uint8 *Uplane, int Upitch, const Uint8 *Vplane, int Vpitch

// Lock a portion of the texture for write-only pixel access.
dll_4           SDL_LockTexture   // SDL_Texture * texture, const SDL_Rect * rect, void **pixels, int *pitch

// Unlock a texture, uploading the changes to video memory, if needed.
DLLVoid dll_1   SDL_UnlockTexture     // SDL_Texture * texture

// Determines whether a window supports the use of render targets
dll_1           SDL_RenderTargetSupported   // SDL_Renderer *renderer

// Set a texture as the current rendering target.
dll_2           SDL_SetRenderTarget   // SDL_Renderer *renderer, SDL_Texture *texture

// Get the current render target or NULL for the default render target.  returns SDL_Texture * 
dll_1           SDL_GetRenderTarget   // SDL_Renderer *renderer

// Set device independent resolution for rendering
dll_3           SDL_RenderSetLogicalSize    // SDL_Renderer * renderer, int w, int h

// Get device independent resolution for rendering
DLLVoid dll_3   SDL_RenderGetLogicalSize    //SDL_Renderer * renderer, int *w, int *h

// Set the drawing area for rendering on the current target.
dll_2           SDL_RenderSetViewport       // SDL_Renderer * renderer, const SDL_Rect * rect

// Get the drawing area for the current target.
DLLVoid dll_2   SDL_RenderGetViewport       // SDL_Renderer * renderer, SDL_Rect * rect

// Set the clip rectangle for the current target.
dll_2           SDL_RenderSetClipRect       // SDL_Renderer * renderer, const SDL_Rect * rect

// Get the clip rectangle for the current target.
DLLVoid dll_2   SDL_RenderGetClipRect       // SDL_Renderer * renderer, SDL_Rect * rect

// Set the drawing scale for rendering on the current target.
dll_3           SDL_RenderSetScale          // SDL_Renderer * renderer, float scaleX, float scaleY

// Get the drawing scale for the current target.
DLLVoid dll_3   SDL_RenderGetScale          // SDL_Renderer * renderer, float *scaleX, float *scaleY

// Set the color used for drawing operations (Rect, Line and Clear).
dll_5           SDL_SetRenderDrawColor      // SDL_Renderer * renderer,  Uint8 r, Uint8 g, Uint8 b, Uint8 a

// Get the color used for drawing operations (Rect, Line and Clear).
dll_5           SDL_GetRenderDrawColor      // SDL_Renderer * renderer, Uint8 * r, Uint8 * g, Uint8 * b, Uint8 * a

// Set the blend mode used for drawing operations (Fill and Line).
dll_2           SDL_SetRenderDrawBlendMode  // SDL_Renderer * renderer, SDL_BlendMode blendMode

// Get the blend mode used for drawing operations.
dll_2           SDL_GetRenderDrawBlendMode  // SDL_Renderer * renderer, SDL_BlendMode *blendMode

// Clear the current rendering target with the drawing color
dll_1           SDL_RenderClear   // SDL_Renderer * renderer

// Draw a point on the current rendering target.
dll_3           SDL_RenderDrawPoint   // SDL_Renderer * renderer, int x, int y

// Draw multiple points on the current rendering target.
dll_3           SDL_RenderDrawPoints    // SDL_Renderer * renderer, const SDL_Point * points, int count

// Draw a line on the current rendering target.
dll_5           SDL_RenderDrawLine      // SDL_Renderer * renderer, int x1, int y1, int x2, int y2

// Draw a series of connected lines on the current rendering target.
dll_3           SDL_RenderDrawLines   // SDL_Renderer * renderer, const SDL_Point * points, int count

// Draw a rectangle on the current rendering target.
dll_2           SDL_RenderDrawRect    // SDL_Renderer * renderer, const SDL_Rect * rect

// Draw some number of rectangles on the current rendering target.
dll_3           SDL_RenderDrawRects   // SDL_Renderer * renderer, const SDL_Rect * rects, int count

// Fill a rectangle on the current rendering target with the drawing color.
dll_2           SDL_RenderFillRect    // SDL_Renderer * renderer, const SDL_Rect * rect

// Fill some number of rectangles on the current rendering target with the drawing color.
dll_3           SDL_RenderFillRects     // SDL_Renderer * renderer, const SDL_Rect * rects, int count

// Copy a portion of the texture to the current rendering target.
dll_4           SDL_RenderCopy          // SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect

// Copy a portion of the source texture to the current rendering target, rotating it by angle around the given center
dll_7           SDL_RenderCopyEx        // SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect,
                                      //    const double angle, const SDL_Point *center, const SDL_RendererFlip flip

// Read pixels from the current rendering target.
dll_5           SDL_RenderReadPixels    // SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 format, void *pixels, int pitch

// Update the screen with rendering performed.
DLLVoid dll_1   SDL_RenderPresent       // SDL_Renderer * renderer

// Destroy the specified texture.
DLLVoid dll_1   SDL_DestroyTexture      // SDL_Texture * texture

// Destroy the rendering context for a window and free associated
DLLVoid dll_1  SDL_DestroyRenderer      // SDL_Renderer * renderer

// Bind the texture to the current OpenGL/ES/ES2 context for use with
dll_3           SDL_GL_BindTexture    // SDL_Texture *texture, float *texw, float *texh

// Unbind a texture from the current OpenGL/ES/ES2 context.
dll_1           SDL_GL_UnbindTexture    // SDL_Texture *texture

//****************** SDL_keyboard.h

struct: SDL_Keysym
  uint scancode    // SDL physical key code - see ::SDL_Scancode for details
  uint sym         // SDL virtual key code - see ::SDL_Keycode for details
  ushort mod       // current key modifiers
  uint unused
;struct

// Get the window which currently has keyboard focus. (returns SDL_Window *)
dll_0           SDL_GetKeyboardFocus 

// Get a snapshot of the current state of the keyboard.(returns const Uint8 *)
dll_1           SDL_GetKeyboardState    // int *numkeys

// Get the current key modifier state for the keyboard.(returns SDL_Keymod)
dll_0           SDL_GetModState

// Set the current key modifier state for the keyboard.
DLLVoid dll_1   SDL_SetModState     // SDL_Keymod modstate

// Get the key code corresponding to the given scancode according to the current keyboard layout. (returns SDL_Keycode)
dll_1           SDL_GetKeyFromScancode    // SDL_Scancode scancode

// Get the scancode corresponding to the given key code according to the current keyboard layout. (returns SDL_Scancode)
dll_1           SDL_GetScancodeFromKey      // SDL_Keycode key

// Get a human-readable name for a scancode. (returns const char *)
dll_1           SDL_GetScancodeName         // SDL_Scancode scancode

// Get a scancode from a human-readable name. (returns SDL_Scancode)
dll_1           SDL_GetScancodeFromName     // const char *name

// Get a human-readable name for a key. (returns const char *)
dll_1           SDL_GetKeyName              // SDL_Keycode key

// Get a key code from a human-readable name. (returns SDL_Keycode)
dll_1           SDL_GetKeyFromName          // const char *name

// Start accepting Unicode text input events.
DLLVoid dll_0   SDL_StartTextInput

// Return whether or not Unicode text input events are enabled. (returns SDL_bool)
dll_0           SDL_IsTextInputActive

// Stop receiving any text input events.
DLLVoid dll_0   SDL_StopTextInput

// Set the rectangle used to type Unicode text inputs.
DLLVoid dll_1   SDL_SetTextInputRect      // SDL_Rect *rect

// Returns whether the platform has some screen keyboard support. (returns SDL_bool)
dll_0           SDL_HasScreenKeyboardSupport

// Returns whether the screen keyboard is shown for given window. (returns SDL_bool)
dll_1           SDL_IsScreenKeyboardShown     // SDL_Window *window

//****************** SDL_events.h
0 constant SDL_RELEASED
1 constant SDL_PRESSED

// The types of events that can be delivered.

enum: SDL_EventType
  SDL_FIRSTEVENT
  
  0x100   // Application events
  SDL_QUIT                    // User-requested quit
  SDL_APP_TERMINATING         // The application is being terminated by the OS
  SDL_APP_LOWMEMORY           // The application is low on memory, free memory if possible.
  SDL_APP_WILLENTERBACKGROUND // The application is about to enter the background
  SDL_APP_DIDENTERBACKGROUND  // The application did enter the background and may not get CPU for some time
  SDL_APP_WILLENTERFOREGROUND // The application is about to enter the foreground
  SDL_APP_DIDENTERFOREGROUND, // The application is now interactive
                               
  0x200 // Window events
  SDL_WINDOWEVENT             // Window state change
  SDL_SYSWMEVENT              // System specific event

  0x300 // Keyboard events
  SDL_KEYDOWN                 // Key pressed
  SDL_KEYUP                   // Key released
  SDL_TEXTEDITING             // Keyboard text editing (composition)
  SDL_TEXTINPUT               // Keyboard text input

  0x400   // Mouse events
  SDL_MOUSEMOTION           // Mouse moved
  SDL_MOUSEBUTTONDOWN       // Mouse button pressed
  SDL_MOUSEBUTTONUP         // Mouse button released
  SDL_MOUSEWHEEL            // Mouse wheel motion

  0x600   // Joystick events
  SDL_JOYAXISMOTION           // Joystick axis motion
  SDL_JOYBALLMOTION           // Joystick trackball motion
  SDL_JOYHATMOTION            // Joystick hat position change
  SDL_JOYBUTTONDOWN           // Joystick button pressed
  SDL_JOYBUTTONUP             // Joystick button released
  SDL_JOYDEVICEADDED          // A new joystick has been inserted into the system
  SDL_JOYDEVICEREMOVED        // An opened joystick has been removed

  0x650   // Game controller events
  SDL_CONTROLLERAXISMOTION           // Game controller axis motion
  SDL_CONTROLLERBUTTONDOWN           // Game controller button pressed
  SDL_CONTROLLERBUTTONUP             // Game controller button released
  SDL_CONTROLLERDEVICEADDED          // A new Game controller has been inserted into the system
  SDL_CONTROLLERDEVICEREMOVED        // An opened Game controller has been removed
  SDL_CONTROLLERDEVICEREMAPPED       // The controller mapping was updated

  0x700   // Touch events
  SDL_FINGERDOWN             
  SDL_FINGERUP
  SDL_FINGERMOTION

  0x800   // Gesture events
  SDL_DOLLARGESTURE
  SDL_DOLLARRECORD
  SDL_MULTIGESTURE

  0x900   // Clipboard events
  SDL_CLIPBOARDUPDATE         // The clipboard changed

  0x1000  // Drag and drop events
  SDL_DROPFILE                 // The system requests a file open

  0x2000  // Render events
  SDL_RENDER_TARGETS_RESET     // The render targets have been reset

  // Events SDL_USEREVENT through SDL_LASTEVENT are for your use and should be allocated with SDL_RegisterEvents()
  
  0x8000    SDL_USEREVENT
  0xFFFF    SDL_LASTEVENT
;enum

// Fields shared by every event
struct: SDL_CommonEvent
  int type
  int timestamp
;struct


// Window state change event data (event.window.*)
struct: SDL_WindowEvent
  int type          int timestamp
  uint windowID    // The associated window
  ubyte event        // ::SDL_WindowEventID
  ubyte padding1
  ubyte padding2
  ubyte padding3
  int data1       // event dependent data
  int data2       // event dependent data
;struct

// Keyboard button event structure (event.key.*)
struct: SDL_KeyboardEvent
  int type          int timestamp
  uint windowID    // The window with keyboard focus, if any
  ubyte state        // ::SDL_PRESSED or ::SDL_RELEASED
  ubyte repeat       // Non-zero if this is a key repeat
  ubyte padding2
  ubyte padding3
  SDL_Keysym keysym  // The key that was pressed or released
;struct

32 constant SDL_TEXTEDITINGEVENT_TEXT_SIZE
// Keyboard text editing event structure (event.edit.*)
struct: SDL_TextEditingEvent
  int type          int timestamp
  uint windowID                            // The window with keyboard focus, if any
  SDL_TEXTEDITINGEVENT_TEXT_SIZE arrayOf byte text  // The editing text
  int start                               // The start cursor of selected editing text
  int length                              // The length of selected editing text
;struct


32 constant SDL_TEXTINPUTEVENT_TEXT_SIZE

// Keyboard text input event structure (event.text.*)
struct: SDL_TextInputEvent
  int type          int timestamp
  uint windowID                          // The window with keyboard focus, if any
  SDL_TEXTEDITINGEVENT_TEXT_SIZE arrayOf byte text  // The input text
;struct

// Mouse motion event structure (event.motion.*)

struct: SDL_MouseMotionEvent
  int type          int timestamp
  uint windowID    // The window with mouse focus, if any
  uint which       // The mouse instance id, or SDL_TOUCH_MOUSEID
  uint state       // The current button state
  int x           // X coordinate, relative to window
  int y           // Y coordinate, relative to window
  int xrel        // The relative motion in the X direction
  int yrel        // The relative motion in the Y direction
;struct

// Mouse button event structure (event.button.*)

struct: SDL_MouseButtonEvent
  int type          int timestamp
  uint windowID    // The window with mouse focus, if any
  uint which       // The mouse instance id, or SDL_TOUCH_MOUSEID
  ubyte button       // The mouse button index
  ubyte state        // ::SDL_PRESSED or ::SDL_RELEASED
  ubyte clicks       // 1 for single-click, 2 for double-click, etc.
  ubyte padding1
  int x           // X coordinate, relative to window
  int y           // Y coordinate, relative to window
;struct

// Mouse wheel event structure (event.wheel.*)

struct: SDL_MouseWheelEvent
  int type          int timestamp
  uint windowID    // The window with mouse focus, if any
  uint which       // The mouse instance id, or SDL_TOUCH_MOUSEID
  int x           // The amount scrolled horizontally, positive to the right and negative to the left
  int y           // The amount scrolled vertically, positive away from the user and negative toward the user
;struct

// Joystick axis motion event structure (event.jaxis.*)

struct: SDL_JoyAxisEvent
  int type          int timestamp
  int which // The joystick instance id
  ubyte axis         // The joystick axis index
  ubyte padding1
  ubyte padding2
  ubyte padding3
  short value       // The axis value (range: -32768 to 32767)
  ushort padding4
;struct

// Joystick trackball motion event structure (event.jball.*)
struct: SDL_JoyBallEvent
  int type          int timestamp
  int which // The joystick instance id
  ubyte ball         // The joystick trackball index
  ubyte padding1
  ubyte padding2
  ubyte padding3
  short xrel        // The relative motion in the X direction
  short yrel        // The relative motion in the Y direction
;struct

// Joystick hat position change event structure (event.jhat.*)
struct: SDL_JoyHatEvent
  int type          int timestamp
  int which // The joystick instance id
  ubyte hat          // The joystick hat index
  ubyte value        // The hat position value.
                      //   \sa ::SDL_HAT_LEFTUP ::SDL_HAT_UP ::SDL_HAT_RIGHTUP
                      //   \sa ::SDL_HAT_LEFT ::SDL_HAT_CENTERED ::SDL_HAT_RIGHT
                      //   \sa ::SDL_HAT_LEFTDOWN ::SDL_HAT_DOWN ::SDL_HAT_RIGHTDOWN
                      //   Note that zero means the POV is centered.
                      
  ubyte padding1
  ubyte padding2
;struct

// Joystick button event structure (event.jbutton.*)
struct: SDL_JoyButtonEvent
  int type          int timestamp
  int which // The joystick instance id
  ubyte button       // The joystick button index
  ubyte state        // ::SDL_PRESSED or ::SDL_RELEASED
  ubyte padding1
  ubyte padding2
;struct

// Joystick device event structure (event.jdevice.*)
struct: SDL_JoyDeviceEvent
  int type          int timestamp
  int which       // The joystick device index for the ADDED event, instance id for the REMOVED event
;struct


// Game controller axis motion event structure (event.caxis.*)

struct: SDL_ControllerAxisEvent
  int type          int timestamp
  int which // The joystick instance id
  ubyte axis         // The controller axis (SDL_GameControllerAxis)
  ubyte padding1
  ubyte padding2
  ubyte padding3
  short value       // The axis value (range: -32768 to 32767)
  ushort padding4
;struct

// Game controller button event structure (event.cbutton.*)
struct: SDL_ControllerButtonEvent
  int type          int timestamp
  int which // The joystick instance id
  ubyte button       // The controller button (SDL_GameControllerButton)
  ubyte state        // ::SDL_PRESSED or ::SDL_RELEASED
  ubyte padding1
  ubyte padding2
;struct


// Controller device event structure (event.cdevice.*)
struct: SDL_ControllerDeviceEvent
  int type          int timestamp
  int which       // The joystick device index for the ADDED event, instance id for the REMOVED or REMAPPED event
;struct

// Touch finger event structure (event.tfinger.*)
struct: SDL_TouchFingerEvent
  int type          int timestamp
  long touchId // The touch device id
  long fingerId
  float x            // Normalized in the range 0...1
  float y            // Normalized in the range 0...1
  float dx           // Normalized in the range 0...1
  float dy           // Normalized in the range 0...1
  float pressure     // Normalized in the range 0...1
;struct

// Multiple Finger Gesture Event (event.mgesture.*)
struct: SDL_MultiGestureEvent
  int type          int timestamp
  long touchId // The touch device index
  float dTheta
  float dDist
  float x
  float y
  ushort numFingers
  ushort padding
;struct

// Dollar Gesture Event (event.dgesture.*)
struct: SDL_DollarGestureEvent
  int type          int timestamp
  long touchId // The touch device id
  long gestureId
  uint numFingers
  float error
  float x            // Normalized center of gesture
  float y            // Normalized center of gesture
;struct

// An event used to request a file open by the system (event.drop.*)
struct: SDL_DropEvent
  int type          int timestamp
  ptrTo byte file         // The file name, which should be freed with SDL_free()
;struct


// The "quit requested" event SDL_QUIT is same as SDL_CommonEvent

// A user-defined event type (event.user.*)

struct: SDL_UserEvent
  int type          int timestamp
  uint windowID    // The associated window if any
  int code        // User defined event code
  ptrTo byte data1        // User defined data pointer
  ptrTo byte data2        // User defined data pointer
;struct



// General event structure
struct: SDL_Event
  56 arrayOf ubyte padding
  union SDL_CommonEvent common
  union SDL_WindowEvent window
  union SDL_KeyboardEvent key
  union SDL_TextEditingEvent edit
  union SDL_TextInputEvent text
  union SDL_MouseMotionEvent motion
  union SDL_MouseButtonEvent button
  union SDL_MouseWheelEvent wheel
  union SDL_JoyAxisEvent jaxiz
  union SDL_JoyBallEvent jball
  union SDL_JoyHatEvent jhat
  union SDL_JoyButtonEvent jbutton
  union SDL_JoyDeviceEvent jdevice
  union SDL_ControllerAxisEvent caxis
  union SDL_ControllerButtonEvent cbutton
  union SDL_ControllerDeviceEvent cdevice
  union SDL_UserEvent user
  union SDL_TouchFingerEvent tfinger
  union SDL_MultiGestureEvent mgesture
  union SDL_DollarGestureEvent dgesture
  union SDL_DropEvent drop
 
  //    SDL_QuitEvent quit;             /**< Quit request event data */
  //  SDL_SysWMEvent syswm;           /**< System dependent window event data */
;struct


//  Pumps the event loop, gathering events from the input devices.
DLLVoid dll_0   SDL_PumpEvents

enum: SDL_EventMiscEnums
  // SDL_eventaction
  0   SDL_ADDEVENT
  1   SDL_PEEKEVENT
  2   SDL_GETEVENT
  // SDL_EventStateEnum
  -1  SDL_QUERY
  0   SDL_IGNORE
  0   SDL_DISABLE
  1   SDL_ENABLE
;enum


// Checks the event queue for messages and optionally returns them.
//  If action is ::SDL_ADDEVENT, up to numevents events will be added to
//  the back of the event queue.

//  If action is ::SDL_PEEKEVENT, up to numevents events at the front
//  of the event queue, within the specified minimum and maximum type,
//  will be returned and will not be removed from the queue.

//  If action is ::SDL_GETEVENT, up to numevents events at the front
//  of the event queue, within the specified minimum and maximum type,
//  will be returned and will be removed from the queue.

//  returns The number of events actually stored, or -1 if there was an error.
//  This function is thread-safe.

dll_5           SDL_PeepEvents  // SDL_Event * events, int numevents, SDL_eventaction action, uint minType, uint maxType

// Checks to see if certain event types are in the event queue.
dll_1           SDL_HasEvent    // uint type
dll_1           SDL_HasEvents   // uint minType, uint maxType

// This function clears events from the event queue
DLLVoid dll_1   SDL_FlushEvent  // uint type
DLLVoid dll_1   SDL_FlushEvents // uint minType, uint maxType

// Polls for currently pending events.
// return 1 if there are any pending events, or 0 if there are none available.
//   event: If not NULL, the next event is removed from the queue and stored in that area.
dll_1           SDL_PollEvent   // SDL_Event * event

// Waits indefinitely for the next available event.
// return 1, or 0 if there was an error while waiting for events.
//   event: If not NULL, the next event is removed from the queue and stored in that area.
dll_1           SDL_WaitEvent   // SDL_Event * event

// Waits until the specified timeout (in milliseconds) for the next available event.
//   return 1, or 0 if there was an error while waiting for events.
//   event: If not NULL, the next event is removed from the queue and stored in that area.
//   timeout: The timeout (in milliseconds) to wait for next event.
dll_2           SDL_WaitEventTimeout  // SDL_Event * event, int timeout

// Add an event to the event queue.
//   return 1 on success, 0 if the event was filtered, or -1 if the event queue was full or there was some other error.
dll_1           SDL_PushEvent     // SDL_Event * event


// Sets up a filter to process all events before they change internal state and are posted to the internal event queue.
//   filter prototype is: typedef int (* SDL_EventFilter) (DLLVoid *userdata, SDL_Event * event);
//  If the filter returns 1, then the event will be added to the internal queue.
//  If it returns 0, then the event will be dropped from the queue, but the
//  internal state will still be updated.  This allows selective filtering of
//  dynamically arriving events.
//  WARNING  Be very careful of what you do in the event filter function, as it may run in a different thread!
//
//  There is one caveat when dealing with the ::SDL_QuitEvent event type.  The
//  event filter is only called when the window manager desires to close the
//  application window.  If the event filter returns 1, then the window will
//  be closed, otherwise the window will remain open if possible.
//
//  If the quit event is generated by an interrupt signal, it will bypass the
//  internal queue and be delivered to the application at the next event poll.
DLLVoid dll_2   SDL_SetEventFilter    // SDL_EventFilter filter, DLLVoid *userdata

// Return the current event filter - can be used to "chain" filters.
//  If there is no event filter set, this function returns SDL_FALSE.
dll_2           SDL_GetEventFilter    // SDL_EventFilter* filter, DLLVoid **userdata

// Add a function which is called when an event is added to the queue.

DLLVoid dll_2   SDL_AddEventWatch    // SDL_EventFilter filter, DLLVoid *userdata

// Remove an event watch function added with SDL_AddEventWatch()
DLLVoid dll_2   SDL_DelEventWatch    // SDL_EventFilter filter, DLLVoid *userdata

// Run the filter function on the current event queue, removing any events for which the filter returns 0.
DLLVoid dll_2   SDL_FilterEvents    // SDL_EventFilter filter, DLLVoid *userdata

// This function allows you to set the state of processing certain events.
// state SDL_IGNORE: that event will be automatically dropped from the event queue and will not event be filtered.
// state SDL_ENABLE: that event will be processed normally.
// state SDL_QUERY: SDL_EventState() will return the current processing state of the specified event.
dll_2           SDL_EventState      // uint type, int state

: SDL_GetEventState     // EVENT_TYPE ... STATE
  SDL_QUERY SDL_EventState
;

// This function allocates a set of user-defined events, and returns the beginning event number for that set of events.
dll_1           SDL_RegisterEvents    // int numevents

: SDL_LoadBMP   // returns SDL_Surface*
  // TOS is filename
  SDL_LoadBMP_RW( "rb" SDL_RWFromFile 1 )
;

//****************** SDL_audio.h

// Audio format flags (defaults to LSB byte order)
enum: eSDLAudioFormatFlags
  0x008         AUDIO_U8      // Unsigned 8-bit samples
  0x8008        AUDIO_S8      // Signed 8-bit samples
  0x0010        AUDIO_U16LSB  // Unsigned 16-bit samples
  0x8010        AUDIO_S16LSB  // Signed 16-bit samples
  0x1010        AUDIO_U16MSB  // As above, but big-endian byte order
  0x9010        AUDIO_S16MSB  // As above, but big-endian byte order
  AUDIO_U16LSB  AUDIO_U16
  AUDIO_S16LSB  AUDIO_S16
  // Native audio byte ordering
  AUDIO_U16LSB  AUDIO_U16SYS  
  AUDIO_S16LSB  AUDIO_S16SYS

  0x8020        AUDIO_S32LSB     // 32-bit integer samples
  0x9020        AUDIO_S32MSB     // As above, but big-endian byte order 
  AUDIO_S32LSB  AUDIO_S32
  
  0x8120        AUDIO_F32LSB     // 32-bit floating point samples
  0x9120        AUDIO_F32MSB     // As above, but big-endian byte order 
  AUDIO_F32LSB  AUDIO_F32
;enum

struct: SDL_AudioSpec
  int      freq        // DSP frequency -- samples per second
  short      format      // Audio data format
  byte      channels     // Number of channels: 1 mono, 2 stereo
  byte      silence      // Audio buffer silence value (calculated)
  short      samples      // Audio buffer size in samples (power of 2)
  short      padding      // Necessary for some compile environments
  int      size        // Audio buffer size in bytes (calculated)
  // This function is called when the audio device needs more data.
  //   'stream' is a pointer to the audio data buffer
  //   'len' is the length of that buffer in bytes.
  //   Once the callback returns, the buffer will no longer be valid.
  //   Stereo samples are stored in a LRLRLR ordering.
  // void (SDLCALL *callback)(void *userdata, Uint8 *stream, int len);
  ptrTo int  callback
  ptrTo int  userdata
;struct

// A structure to hold a set of audio conversion filters and buffers
struct: SDL_AudioCVT
  int        needed      // Set to 1 if conversion possible
  short      src_format    // Source audio format
  short      dst_format    // Target audio format
  double      rate_incr    // Rate conversion increment
  ptrTo byte  buf        // Buffer to hold entire audio data
  int        len        // Length of original audio buffer
  int        len_cvt      // Length of converted audio buffer
  int        len_mult      // buffer must be len*len_mult big
  double      len_ratio    // Given len, final size is len*len_ratio
  // void (SDLCALL *filters[10])(struct: SDL_AudioCVT *cvt, Uint16 format);
  10 arrayOf ptrTo int    filters
  int        filter_index  // Current audio conversion function
;struct

// These functions return the list of built in audio drivers, in the order that they are normally initialized by default.
dll_0   SDL_GetNumAudioDrivers
dll_1   SDL_GetAudioDriver    // int index - returns const char ptr to driver name

// These functions are used internally, and should not be used unless you
// have a specific need to specify the audio driver you want to use.
// You should normally use SDL_Init() or SDL_InitSubSystem().
dll_1 SDL_AudioInit      // const char *driver_name
DLLVoid dll_0 SDL_AudioQuit

// This function returns the name of the current audio driver, or NULL if no driver has been initialized.
dll_0 SDL_GetCurrentAudioDriver    // returns const char*

// This function opens the audio device with the desired parameters, and
// returns 0 if successful, placing the actual hardware parameters in the
// structure pointed to by 'obtained'.  If 'obtained' is NULL, the audio
// data passed to the callback function will be guaranteed to be in the
// requested format, and will be automatically converted to the hardware
// audio format if necessary.  This function returns -1 if it failed 
// to open the audio device, or couldn't set up the audio thread.
//
// When filling in the desired audio spec structure,
//  'desired->freq' should be the desired audio frequency in samples-per-second.
//  'desired->format' should be the desired audio format.
//  'desired->samples' is the desired size of the audio buffer, in samples.
//     This number should be a power of two, and may be adjusted by the audio
//     driver to a value more suitable for the hardware.  Good values seem to
//     range between 512 and 8096 inclusive, depending on the application and
//     CPU speed.  Smaller values yield faster response time, but can lead
//     to underflow if the application is doing heavy processing and cannot
//     fill the audio buffer in time.  A stereo sample consists of both right
//     and left channels in LR ordering.
//     Note that the number of samples is directly related to time by the
//     following formula:  ms = (samples*1000)/freq
//  'desired->size' is the size in bytes of the audio buffer, and is
//     calculated by SDL_OpenAudio().
//  'desired->silence' is the value used to set the buffer to silence,
//     and is calculated by SDL_OpenAudio().
//  'desired->callback' should be set to a function that will be called
//     when the audio device is ready for more data.  It is passed a pointer
//     to the audio buffer, and the length in bytes of the audio buffer.
//     This function usually runs in a separate thread, and so you should
//     protect data structures that it accesses by calling SDL_LockAudio()
//     and SDL_UnlockAudio() in your code.
//  'desired->userdata' is passed as the first parameter to your callback
//     function.
//
// The audio device starts out playing silence when it's opened, and should
// be enabled for playing by calling SDL_PauseAudio(0) when you are ready
// for your audio callback function to be called.  Since the audio driver
// may modify the requested size of the audio buffer, you should allocate
// any local mixing buffers after you open the audio device.
dll_2 SDL_OpenAudio    // SDL_AudioSpec *desired, SDL_AudioSpec *obtained

//  SDL Audio Device IDs.
//
//  A successful call to SDL_OpenAudio() is always device id 1, and legacy
//  SDL audio APIs assume you want this device ID. SDL_OpenAudioDevice() calls
//  always returns devices >= 2 on success. The legacy calls are good both
//  for backwards compatibility and when you don't care about multiple,
//  specific, or capture devices.
///
// typedef Uint32 SDL_AudioDeviceID;

//  Get the number of available devices exposed by the current driver.
//  Only valid after a successfully initializing the audio subsystem.
//  Returns -1 if an explicit list of devices can't be determined; this is
//  not an error. For example, if SDL is set up to talk to a remote audio
//  server, it can't list every one available on the Internet, but it will
//  still allow a specific host to be specified to SDL_OpenAudioDevice().
//
//  In many common cases, when this function returns a value <= 0, it can still
//  successfully open the default device (NULL for first argument of SDL_OpenAudioDevice()).
dll_1 SDL_GetNumAudioDevices    // int iscapture

//  Get the human-readable name of a specific audio device.
//  Must be a value between 0 and (number of audio devices-1).
//  Only valid after a successfully initializing the audio subsystem.
//  The values returned by this function reflect the latest call to
//  SDL_GetNumAudioDevices(); recall that function to redetect available
//  hardware.
//
//  The string returned by this function is UTF-8 encoded, read-only, and
//  managed internally. You are not to free it. If you need to keep the
//  string for any length of time, you should make your own copy of it, as it
//  will be invalid next time any of several other SDL functions is called.
dll_2   SDL_GetAudioDeviceName    // int index, int iscapture

//  Open a specific audio device. Passing in a device name of NULL requests
//  the most reasonable default (and is equivalent to calling SDL_OpenAudio()).
//
//  The device name is a UTF-8 string reported by SDL_GetAudioDeviceName(), but
//  some drivers allow arbitrary and driver-specific strings, such as a
//  hostname/IP address for a remote audio server, or a filename in the
//  diskaudio driver.
//
//  return 0 on error, a valid device ID that is >= 2 on success.
//
//  SDL_OpenAudio(), unlike this function, always acts on device ID 1.
dll_5     SDL_OpenAudioDevice   // const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes



// Get the current audio state:
 enum: eSDLAudioStatus
   0 SDL_AUDIO_STOPPED
  SDL_AUDIO_PLAYING
  SDL_AUDIO_PAUSED
;enum
dll_0   SDL_GetAudioStatus
dll_1   SDL_GetAudioDeviceStatus    // SDL_AudioDeviceID dev

//  These functions pause and unpause the audio callback processing.
//  They should be called with a parameter of 0 after opening the audio
//  device to start playing sound.  This is so you can safely initialize
//  data for your callback function after opening the audio device.
//  Silence will be written to the audio device during the pause.
DLLVoid dll_1 SDL_PauseAudio      // int pause_on
DLLVoid dll_2 SDL_PauseAudioDevice      // SDL_AudioDeviceID dev, int pause_on

// This function loads a WAVE from the data source, automatically freeing
// that source if 'freesrc' is non-zero.  For example, to load a WAVE file,
// you could do:
//  SDL_LoadWAV_RW(SDL_RWFromFile("sample.wav", "rb"), 1, ...);
//
// If this function succeeds, it returns the given SDL_AudioSpec,
// filled with the audio data format of the wave data, and sets
// 'audio_buf' to a malloc()'d buffer containing the audio data,
// and sets 'audio_len' to the length of that audio buffer, in bytes.
// You need to free the audio buffer with SDL_FreeWAV() when you are 
// done with it.
//
// This function returns NULL and sets the SDL error message if the 
// wave file cannot be opened, uses an unknown data format, or is 
// corrupt.  Currently raw and MS-ADPCM WAVE files are supported.
dll_5 SDL_LoadWAV_RW    // SDL_RWops *src, int freesrc, SDL_AudioSpec *spec, Uint8 **audio_buf, Uint32 *audio_len

// Compatibility convenience function -- loads a WAV from a file
: SDL_LoadWAV    // file, spec, audio_buf, audio_len
  >r >r >r
  "rb" SDL_RWFromFile 1
  r> r> r>
  SDL_LoadWAV_RW
;

// This function frees data previously allocated with SDL_LoadWAV_RW()
DLLVoid dll_1 SDL_FreeWAV    // Uint8 *audio_buf

//  This function takes a source format and rate and a destination format
//  and rate, and initializes the \c cvt structure with information needed
//  by SDL_ConvertAudio() to convert a buffer of audio data from one format
//  to the other.
//  returns -1 if the format conversion is not supported, 0 if there's
//  no conversion needed, or 1 if the audio filter is set up.
dll_7   SDL_BuildAudioCVT   // SDL_AudioCVT * cvt, SDL_AudioFormat src_format, Uint8 src_channels,
                            //   int src_rate, SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate

// Once you have initialized the 'cvt' structure using SDL_BuildAudioCVT(),
// created an audio buffer cvt->buf, and filled it with cvt->len bytes of
// audio data in the source format, this function will convert it in-place
// to the desired format.
// The data conversion may expand the size of the audio data, so the buffer
// cvt->buf should be allocated after the cvt structure is initialized by
// SDL_BuildAudioCVT(), and should be cvt->len*cvt->len_mult bytes long.
dll_1 SDL_ConvertAudio    // SDL_AudioCVT *cvt

128 constant SDL_MIX_MAXVOLUME

// This takes two audio buffers of the playing audio format and mixes
// them, performing addition, volume adjustment, and overflow clipping.
// The volume ranges from 0 - 128, and should be set to SDL_MIX_MAXVOLUME
// for full audio volume.  Note this does not change hardware volume.
// This is provided for convenience -- you can mix your own audio data.
// #define SDL_MIX_MAXVOLUME 128
DLLVoid dll_4 SDL_MixAudio      // Uint8 *dst, const Uint8 *src, Uint32 len, int volume

//  This works like SDL_MixAudio(), but you specify the audio format instead of
//  using the format of audio device 1. Thus it can be used when no audio
//  device is open at all.
DLLVoid dll_5 SDL_MixAudioFormat  // Uint8 * dst, const Uint8 * src, SDL_AudioFormat format, Uint32 len, int volume

// The lock manipulated by these functions protects the callback function.
// During a LockAudio/UnlockAudio pair, you can be guaranteed that the
// callback function is not running.  Do not call these from the callback
// function or you will cause deadlock.
DLLVoid dll_0 SDL_LockAudio
DLLVoid dll_1 SDL_LockAudioDevice     // SDL_AudioDeviceID dev
DLLVoid dll_0 SDL_UnlockAudio
DLLVoid dll_0 SDL_UnlockAudioDevice     // SDL_AudioDeviceID dev

// This function shuts down audio processing and closes the audio device.
DLLVoid dll_0 SDL_CloseAudio
DLLVoid dll_1 SDL_CloseAudioDevice     // SDL_AudioDeviceID dev


previous definitions
loaddone


