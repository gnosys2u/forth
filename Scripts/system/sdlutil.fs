
autoforget sdlutil
requires sdl
also sdl

: sdlutil ;

null  -> ptrTo SDL_Surface sdlSurface
null  -> ptrTo byte sdlWindow
null  -> ptrTo byte sdlRenderer
null  -> ptrTo byte sdlTexture
 640  -> int screenW
 480  -> int screenH
-320  -> int screenLeftLimit
 320  -> int screenRightLimit
-240  -> int screenTopLimit
 240  -> int screenBottomLimit
32    -> bitsPerPixel
 
: dummyFetchOp drop 0 ;

' dummyFetchOp -> op pixel@
' 2drop -> op pixel!

0x00FFFF00 -> int pixelColor

screenW 2/ -> int screenCenterX 
screenH 2/ -> int screenCenterY

int _screenCenterPixelAddr
0 -> int drawDepth

SDL_Rect srcPos
SDL_Rect dstPos


0 -> srcPos.x 0 -> srcPos.y

0 -> dstPos.x 0 -> dstPos.y

0 -> int _sdlStarted

: endSDL
  if( _sdlStarted )
    SDL_Quit
    false -> _sdlStarted
  endif
;

: startSDL

  if( _sdlStarted )
    endSDL
  endif
  true -> _sdlStarted
  
  SDL_Init( SDL_INIT_VIDEO ) "SDL_Init returns " %s %d %nl
  //SDL_SetVideoMode( screenW screenH 0 SDL_SWSURFACE ) -> sdlSurface
  SDL_CreateWindow( "My Game Window" SDL_WINDOWPOS_UNDEFINED_MASK SDL_WINDOWPOS_UNDEFINED_MASK screenW screenH SDL_WINDOW_SHOWN ) -> sdlWindow
  
  if( sdlWindow 0= )
    "SDL_CreateWindow failed!\n" %s abort
  else
    SDL_GetWindowSurface(sdlWindow) -> sdlSurface
    SDL_CreateRenderer(sdlWindow -1 0) -> sdlRenderer
    SDL_CreateTexture(sdlRenderer SDL_PIXELFORMAT_ARGB8888 SDL_TEXTUREACCESS_STREAMING screenW screenH) -> sdlTexture
    bitsPerPixel
    //"bits per pixel: " %s dup %d %nl
    case
      8 of
        lit c@ -> pixel@
        lit c! -> pixel!
        0x55 -> pixelColor
      endof
      16 of
        lit w@ -> pixel@
        lit w! -> pixel!
        0x5555 -> pixelColor
      endof
      32 of
        lit @ -> pixel@
        lit ! -> pixel!
        0x00FFFF00 -> pixelColor
      endof
      // unhandled pixel size
      lit dummyFetchOp -> pixel@
      lit 2drop -> pixel!
      "startSDL: Unhandled pixel size " %s dup %d %nl
    endcase
    
    // set coordinates so center of screen is 0,0
    screenW 2/ dup -> screenCenterX dup -> screenRightLimit negate -> screenLeftLimit
    screenH 2/ dup -> screenCenterY dup -> screenBottomLimit negate -> screenTopLimit
    screenCenterY sdlSurface.pitch *
    screenCenterX sdlSurface.format.BytesPerPixel *
    + sdlSurface.pixels + -> _screenCenterPixelAddr
    
    // setup draw count and full-screen dirty rect for beginDraw/endDraw
    screenW -> srcPos.w
    screenH -> srcPos.h
    screenW -> dstPos.w
    screenH -> dstPos.h
    0 -> drawDepth
  endif
;

: beginDraw
  drawDepth 0= if
    SDL_LockSurface( sdlSurface ) drop
  endif
  1 ->+ drawDepth
;

: endDraw
  1 ->- drawDepth
  drawDepth 0= if
    SDL_UnlockSurface( sdlSurface )
    SDL_UpdateRects( sdlSurface 1 dstPos )
    SDL_UpdateWindowSurface(sdlWindow)
  else
    drawDepth 0< if
      "endDraw called with drawDepth " %s drawDepth %d %nl
    endif
  endif
;

0 -> int curX
0 -> int curY

: moveTo
  -> curY -> curX
;

// X Y screenAddress -> address of pixel on screen
: screenAddress
  sdlSurface.pitch * negate
  swap
  sdlSurface.format.BytesPerPixel *
  + _screenCenterPixelAddr +
;

// dX dY screenAddressRelative -> address of pixel on screen
: screenAddressRelative
  curY + sdlSurface.pitch * negate
  swap
  curX + sdlSurface.format.BytesPerPixel *
  + _screenCenterPixelAddr +
;


// drawPixel & drawPixelRelative assume that you are handling SDL_LockSurface/SDL_UnlockSurface/SDL_UpdateRects
// X Y drawPixel  - draw a dot at X,Y with color specified by pixelColor
: drawPixel
  screenAddress pixelColor swap pixel!
;

: drawPixelRelative
  screenAddressRelative pixelColor swap pixel!
;

// X Y getPixel -> fetches pixel value at X,Y
: getPixel
  screenAddress pixel@
;

// X Y isOnScreen -> true/false
: isOnScreen
  screenTopLimit screenBottomLimit within
  if
    screenLeftLimit screenRightLimit within
  else
    drop false
  endif
;

: sc -> pixelColor ;
: mt moveTo ;

previous

loaddone

