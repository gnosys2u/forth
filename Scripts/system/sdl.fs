
"SDL is deprecated, use SDL2\n" %s
lf sdl2.txt
loaddone

autoforget sdl

#if WINDOWS
DLLVocabulary sdl SDL.dll
#else
DLLVocabulary sdl libSDL.so
#endif

also sdl definitions

//****************** SDL.h
dll_1    SDL_Init
DLLVoid dll_0    SDL_Quit
dll_1    SDL_InitSubSystem
DLLVoid dll_1    SDL_QuitSubSystem
dll_0	SDL_GetError

// arg is what systems you want to check for initialization
dll_1    SDL_WasInit

// flags for SDL_Init, SDL_InitSubSystem & SDL_QuitSubSystem
enum: eSDLInit
	0x00000001      SDL_INIT_TIMER
	0x00000010      SDL_INIT_AUDIO
	0x00000020      SDL_INIT_VIDEO
	0x00000100      SDL_INIT_CDROM
	0x00000200      SDL_INIT_JOYSTICK
	0x00100000      SDL_INIT_NOPARACHUTE    // Don't catch fatal signals
	0x01000000      SDL_INIT_EVENTTHREAD    // Not supported on all OS's
	0x0000FFFF      SDL_INIT_EVERYTHING
;enum

//****************** SDL_active.h

// This function returns the current state of the application, which is a
// bitwise combination of SDL_APPMOUSEFOCUS, SDL_APPINPUTFOCUS, and
// SDL_APPACTIVE.  If SDL_APPACTIVE is set, then the user is able to
// see your application, otherwise it has been iconified or disabled.

dll_0    SDL_GetAppState

enum: eSDLActive
	0x01        SDL_APPMOUSEFOCUS   // The app has mouse coverage
	0x02        SDL_APPINPUTFOCUS	// The app has input focus
	0x04        SDL_APPACTIVE       // The application is active
;enum

//****************** SDL_video.h

enum: eSDLVideo
	255		SDL_ALPHA_OPAQUE
	0		SDL_ALPHA_TRANSPARENT
;enum

struct: SDL_Rect
	short x		short y
	short w		short h
;struct

struct: SDL_Color
	byte r	byte g	byte b	byte unused
;struct

struct: SDL_Palette
	int					ncolors
	ptrTo SDL_Color		colors
;struct

// Everything in the pixel format structure is read-only
struct: SDL_PixelFormat
	ptrTo SDL_Palette	palette
	byte	BitsPerPixel
	byte	BytesPerPixel
	byte	Rloss
	byte	Gloss
	byte	Bloss
	byte	Aloss
	byte	Rshift
	byte	Gshift
	byte	Bshift
	byte	Ashift
	byte	Rmask
	byte	Gmask
	byte	Bmask
	byte	Amask
	int		colorkey
	byte	alpha
;struct

// This structure should be treated as read-only, except for 'pixels',
//   which, if not NULL, contains the raw pixel data for the surface.

struct: SDL_Surface
	int flags
	ptrTo SDL_PixelFormat format
	int w
	int h
	short pitch
	ptrTo byte pixels		// read-write
	int	offset
	ptrTo byte hwdata
	SDL_Rect clip_rect
	int unused1
	int locked
	ptrTo byte map
	int format_version
	int refcount
;struct

// These are the currently supported flags for the SDL_surface
// Available for SDL_CreateRGBSurface() or SDL_SetVideoMode()
enum: eSDL_Surface
	0			SDL_SWSURFACE
	1			SDL_HWSURFACE
	4			SDL_ASYNCBLIT
	// Available for SDL_SetVideoMode()
	0x10000000	SDL_ANYFORMAT
	0x20000000	SDL_HWPALETTE
	0x40000000	SDL_DOUBLEBUF	
	0x80000000	SDL_FULLSCREEN	
	0x00000002	SDL_OPENGL      
	0x0000000A	SDL_OPENGLBLIT	
	0x00000010	SDL_RESIZABLE	
	0x00000020	SDL_NOFRAME
	// Used internally (read-only)
	0x00000100	SDL_HWACCEL
	0x00001000	SDL_SRCCOLORKEY
	0x00002000	SDL_RLEACCELOK
	0x00004000	SDL_RLEACCEL
	0x00010000	SDL_SRCALPHA
	0x01000000	SDL_PREALLOC
;enum

struct: SDL_VideoInfo
	int						flags			// see eSDL_VideoInfoFlags
	int						video_mem
	ptrTo SDL_PixelFormat	vfmt
	int						current_w
	int						current_h
;struct

enum: eSDL_VideoInfoFlags
	1		hwAvailable
	2		wmAvailable
	0x200	blitHW
	0x400	blitHWColorKey
	0x800	blitHWAlpha
	0x1000	blitSW
	0x2000	blitSWColorKey
	0x4000	blitSWAlpha
	0x8000	blitColorFill
;enum
	
dll_2	SDL_VideoInit
DLLVoid dll_0	SDL_VideoQuit
dll_2	SDL_VideoDriverName
dll_0	SDL_GetVideoSurface
dll_0	SDL_GetVideoInfo
dll_4	SDL_VideoModeOK
dll_2	SDL_ListModes
dll_4	SDL_SetVideoMode

DLLVoid dll_3	SDL_UpdateRects
DLLVoid dll_5	SDL_UpdateRect
dll_1	SDL_Flip

dll_3	SDL_SetGamma
dll_3	SDL_SetGammaRamp
dll_3	SDL_GetGammaRamp

dll_4	SDL_SetColors
dll_5	SDL_SetPalette
dll_4	SDL_MapRGB
dll_5	SDL_MapRGBA
DLLVoid dll_5	SDL_GetRGB
DLLVoid dll_6	SDL_GetRGBA
dll_8	SDL_CreateRGBSurface
dll_9	SDL_CreateRGBSurfaceFrom
DLLVoid dll_1	SDL_FreeSurface

dll_1	SDL_LockSurface
DLLVoid dll_1	SDL_UnlockSurface
dll_2	SDL_LoadBMP_RW
dll_3	SDL_SaveBMP_RW
dll_3	SDL_SetColorKey
dll_3	SDL_SetAlpha
dll_2	SDL_SetClipRect
DLLVoid dll_2	SDL_GetClipRect
dll_3	SDL_ConvertSurface
dll_4	SDL_UpperBlit
dll_4	SDL_LowerBlit
dll_3	SDL_FillRect
dll_1	SDL_DisplayFormat
dll_1	SDL_DisplayFormatAlpha
DLLVoid dll_2	SDL_WM_SetCaption
DLLVoid dll_2	SDL_WM_GetCaption
DLLVoid dll_2	SDL_WM_SetIcon
dll_0	SDL_WM_IconifyWindow
dll_1	SDL_WM_ToggleFullScreen

enum: eSDL_GrabMode
	-1	SDL_GRAB_QUERY
	SDL_GRAB_OFF
	SDL_GRAB_ON
	SDL_GRAB_FULLSCREEN
;enum

dll_1	SDL_WM_GrabInput

//****************** SDL_timer.h

dll_0    SDL_GetTicks
DLLVoid dll_1    SDL_Delay

// extern DECLSPEC int SDLCALL SDL_SetTimer(Uint32 interval, SDL_TimerCallback callback);
dll_2    SDL_SetTimer

// extern DECLSPEC SDL_TimerID SDLCALL SDL_AddTimer(Uint32 interval, SDL_NewTimerCallback callback, void *param);
dll_2    SDL_AddTimer

dll_1    SDL_RemoveTimer



//****************** SDL_rwops.h
dll_2	SDL_RWFromFile
dll_2	SDL_RWFromFP
dll_2	SDL_RWFromMem
dll_2	SDL_RWFromConstMem
dll_0	SDL_AllocRW
DLLVoid dll_1	SDL_FreeRW

enum: eSDL_Seek
	RW_SEEK_SET
	RW_SEEK_CUR
	RW_SEEK_END
;enum

dll_1 SDL_ReadLE16
dll_1 SDL_ReadBE16
dll_1 SDL_ReadLE32
dll_1 SDL_ReadBE32
dll_1 SDL_ReadLE64
dll_1 SDL_ReadBE64

dll_2 SDL_WriteLE16
dll_2 SDL_WriteBE16
dll_2 SDL_WriteLE32
dll_2 SDL_WriteBE32
dll_2 SDL_WriteLE64
dll_2 SDL_WriteBE64

//****************** SDL_keyboard.h

//struct: SDL_keysym
//    byte    scancode
    
//GetError
//	SDL_error.h
//SDL_Event
//PollEvent
//	SDL_events.h
//GetKeyState
//	SDL_keyboard.h


: SDL_LoadBMP
  // TOS is filename
  SDL_LoadBMP_RW( "rb" SDL_RWFromFile 1 )
;

//****************** SDL_audio.h
struct: SDL_AudioSpec
	int			freq				// DSP frequency -- samples per second
	short			format			// Audio data format
	byte			channels 		// Number of channels: 1 mono, 2 stereo
	byte			silence			// Audio buffer silence value (calculated)
	short			samples			// Audio buffer size in samples (power of 2)
	short			padding			// Necessary for some compile environments
	int			size				// Audio buffer size in bytes (calculated)
	// This function is called when the audio device needs more data.
	//   'stream' is a pointer to the audio data buffer
	//   'len' is the length of that buffer in bytes.
	//   Once the callback returns, the buffer will no longer be valid.
	//   Stereo samples are stored in a LRLRLR ordering.
	// void (SDLCALL *callback)(void *userdata, Uint8 *stream, int len);
	ptrTo int	callback
	ptrTo int	userdata
;struct

// Audio format flags (defaults to LSB byte order)
enum: eSDLAudioFormatFlags
  0x008		AUDIO_U8			// Unsigned 8-bit samples
  0x8008		AUDIO_S8			// Signed 8-bit samples
  0x0010		AUDIO_U16LSB	// Unsigned 16-bit samples
  0x8010		AUDIO_S16LSB	// Signed 16-bit samples
  0x1010		AUDIO_U16MSB	// As above, but big-endian byte order
  0x9010		AUDIO_S16MSB	// As above, but big-endian byte order
  AUDIO_U16LSB		AUDIO_U16
  AUDIO_S16LSB		AUDIO_S16
  // Native audio byte ordering
  AUDIO_U16LSB	AUDIO_U16SYS	
  AUDIO_S16LSB	AUDIO_S16SYS
;enum

// A structure to hold a set of audio conversion filters and buffers
struct: SDL_AudioCVT
  int				needed			// Set to 1 if conversion possible
  short			src_format		// Source audio format
  short			dst_format		// Target audio format
  double			rate_incr		// Rate conversion increment
  ptrTo byte	buf				// Buffer to hold entire audio data
  int				len				// Length of original audio buffer
  int				len_cvt			// Length of converted audio buffer
  int				len_mult			// buffer must be len*len_mult big
  double			len_ratio		// Given len, final size is len*len_ratio
  // void (SDLCALL *filters[10])(struct: SDL_AudioCVT *cvt, Uint16 format);
  10 arrayOf ptrTo int		filters
  int				filter_index	// Current audio conversion function
;struct

// These functions are used internally, and should not be used unless you
// have a specific need to specify the audio driver you want to use.
// You should normally use SDL_Init() or SDL_InitSubSystem().
dll_1 SDL_AudioInit			// const char *driver_name
DLLVoid dll_0 SDL_AudioQuit

// This function fills the given character buffer with the name of the
// current audio driver, and returns a pointer to it if the audio driver has
// been initialized.  It returns NULL if no driver has been initialized.
dll_2 SDL_AudioDriverName		// char *namebuf, int maxlen

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
dll_2 SDL_OpenAudio		// SDL_AudioSpec *desired, SDL_AudioSpec *obtained

// Get the current audio state:
 enum: eSDLAudioStatus
   0 SDL_AUDIO_STOPPED
	SDL_AUDIO_PLAYING
	SDL_AUDIO_PAUSED
;enum
dll_0 SDL_GetAudioStatus

// This function pauses and unpauses the audio callback processing.
// It should be called with a parameter of 0 after opening the audio
// device to start playing sound.  This is so you can safely initialize
// data for your callback function after opening the audio device.
// Silence will be written to the audio device during the pause.
DLLVoid dll_1 SDL_PauseAudio			// int pause_on

// This function loads a WAVE from the data source, automatically freeing
// that source if 'freesrc' is non-zero.  For example, to load a WAVE file,
// you could do:
//	SDL_LoadWAV_RW(SDL_RWFromFile("sample.wav", "rb"), 1, ...);
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
dll_5 SDL_LoadWAV_RW		// SDL_RWops *src, int freesrc, SDL_AudioSpec *spec, Uint8 **audio_buf, Uint32 *audio_len

// Compatibility convenience function -- loads a WAV from a file
: SDL_LoadWAV		// file, spec, audio_buf, audio_len
  >r >r >r
  "rb" SDL_RWFromFile 1
  r> r> r>
  SDL_LoadWAV_RW
;

// This function frees data previously allocated with SDL_LoadWAV_RW()
DLLVoid dll_1 SDL_FreeWAV		// Uint8 *audio_buf

// This function takes a source format and rate and a destination format
// and rate, and initializes the 'cvt' structure with information needed
// by SDL_ConvertAudio() to convert a buffer of audio data from one format
// to the other.
// This function returns 0, or -1 if there was an error.
dll_7 SDL_BuildAudioCVT
// SDL_AudioCVT *cvt,
//	Uint16 src_format, Uint8 src_channels, int src_rate,
// Uint16 dst_format, Uint8 dst_channels, int dst_rate

// Once you have initialized the 'cvt' structure using SDL_BuildAudioCVT(),
// created an audio buffer cvt->buf, and filled it with cvt->len bytes of
// audio data in the source format, this function will convert it in-place
// to the desired format.
// The data conversion may expand the size of the audio data, so the buffer
// cvt->buf should be allocated after the cvt structure is initialized by
// SDL_BuildAudioCVT(), and should be cvt->len*cvt->len_mult bytes long.
dll_1 SDL_ConvertAudio		// SDL_AudioCVT *cvt

// This takes two audio buffers of the playing audio format and mixes
// them, performing addition, volume adjustment, and overflow clipping.
// The volume ranges from 0 - 128, and should be set to SDL_MIX_MAXVOLUME
// for full audio volume.  Note this does not change hardware volume.
// This is provided for convenience -- you can mix your own audio data.
// #define SDL_MIX_MAXVOLUME 128
DLLVoid dll_4 SDL_MixAudio			// Uint8 *dst, const Uint8 *src, Uint32 len, int volume

// The lock manipulated by these functions protects the callback function.
// During a LockAudio/UnlockAudio pair, you can be guaranteed that the
// callback function is not running.  Do not call these from the callback
// function or you will cause deadlock.
DLLVoid dll_0 SDL_LockAudio
DLLVoid dll_0 SDL_UnlockAudio

// This function shuts down audio processing and closes the audio device.
DLLVoid dll_0 SDL_CloseAudio


previous definitions
