

require sdl2.fs
require sdl2_image.fs
require sdl2_ttf.fs

also sdl2
also sdl2_image
also sdl2_ttf


class: SDLSurface
  ptrTo SDL_Surface surface

  m: freeSurface
    if(surface)
      SDL_FreeSurface(surface)
      surface~
    endif
  ;m
  
  m: delete
    freeSurface
  ;m
  
  \ IMAGEFILE_PATH ...          load image from file into this surface
  m: imageLoad
    freeSurface
    IMG_Load surface!
  ;m
  
  \  ERROR_PREFACE_STRING ...       report error
  m: reportError
    "Error: " %s getClass.getName %s ":" %s %s " - " %s SDL_GetError %s %nl
  ;m
  
  \ ERROR_FLAG ERROR_PREFACE_STRING ...     report error if ERROR_FLAG is nonzero
  m: checkError
    swap
    if
      reportError
    else
      drop
    endif
  ;m
  
;class


512 int defaultWindowWidth!
512 int defaultWindowHeight!
4   int defaultWindowBytesPerPixel!

enum: SDLScreenOriginMode
  kScreenOriginCenter
  kScreenOriginTopLeft
;enum

class: SDLGraphics extends SDLSurface

  cell window           \ SDL_Window
  cell renderer         \ SDL_Renderer
  cell windowTexture    \ SDL_Texture

  int width
  int height
  int leftLimit
  int rightLimit
  int topLimit
  int bottomLimit
  op _getPixel
  op _setPixel
  op _setRawPixel
  int pixelColor
  int centerX 
  int centerY
  ptrTo ubyte centerPixelAddr
  ptrTo ubyte minPixelAddr
  ptrTo ubyte maxPixelAddr
  int drawDepth
  SDL_Rect srcPos
  SDL_Rect dstPos
  int sdlStarted
  int curX
  int curY
  int bytesPerPixel
  int bytesPerRow
  SDLScreenOriginMode originMode  \ must be set before init or initWithSize
  int xPitch        \ like bytesPerPixel, but takes x axis orientation into account
  int yPitch        \ like bytesPerRow, but takes y axis orientation into account
  ptrTo ubyte mFont
  String mFontFile
  int mFontSize

  : dummyFetchOp drop 0 ;
  
  m: delete
    mFontFile~
  ;m
  
  \ bottomLimit/topLimit/leftLimit/rightLimit are coordinate limits
  \ for a system where 0,0 is in the center of the main window
  : initLimits
    height   2/ dup bottomLimit!   dup centerY!   negate topLimit!
    width    2/ dup rightLimit!    dup centerX!   negate leftLimit!
  ;
  
  \ WIDTH HEIGHT BYTES_PER_PIXEL ...        set window size and pixel depth
  m: configure
    bytesPerPixel!
    height!
    width!
  ;m
  
  \ ...                 initialize window
  m: init
    if(bytesPerPixel 0=)
      \ configure wasn't called, use defaults
      defaultWindowWidth width!
      defaultWindowHeight height!
      defaultWindowBytesPerPixel bytesPerPixel!
    endif

    surface~
    window~
    renderer~
    windowTexture~
    
    initLimits
    
    case(bytesPerPixel)
      of( 1 )
        ['] c@ _getPixel!
        ['] c! dup _setPixel! _setRawPixel!
        $55 pixelColor!
      endof
      of( 2 )
        ['] s@ _getPixel!
        ['] s! dup _setPixel! _setRawPixel!
        $5555 pixelColor!
      endof
      of( 4 )
        ['] i@ _getPixel!
        ['] i! dup _setPixel! _setRawPixel!
        $00FFFF00 pixelColor!
      endof
      \ unhandled pixel size
      ['] dummyFetchOp _getPixel!
      ['] 2drop dup _setPixel! _setRawPixel!
      "startSDL: Unhandled pixel size " %s dup %d %nl
    endcase
    
    bytesPerPixel width * bytesPerRow!

    drawDepth~
    srcPos.x~   srcPos.y~
    dstPos.x~   dstPos.y~
    sdlStarted~
    curX~       curY~
    
    new String mFontFile!
    mFontFile.set("FreeSans.ttf")
    18 mFontSize!
    mFont~
  ;m
  
  \ ORIGIN_MODE ...             set origin mode
  \ kScreenOriginTopLeft        top left of screen is 0,0 and positive Y is down
  \ kScreenOriginCenter         center of screen is 0,0 and positive Y is up
  m: setOriginMode
    originMode!
    if(originMode kScreenOriginTopLeft =)
      \ set coordinates so top left of screen is 0,0 and positive Y is down
      centerX~   width rightLimit!     leftLimit~
      centerY~   height bottomLimit!   topLimit~
      surface.pitch yPitch!
    else
      \ set coordinates so center of screen is 0,0 and positive Y is up
      width 2/  dup rightLimit!    dup centerX!   negate leftLimit!
      height 2/ dup bottomLimit!   dup centerY!   negate topLimit!
      surface.pitch negate yPitch!
    endif
    
    surface.pixels minPixelAddr!
    width height * bytesPerPixel * minPixelAddr + maxPixelAddr!
    bytesPerPixel xPitch!
    centerY surface.pitch *
    centerX bytesPerPixel *
    + minPixelAddr + centerPixelAddr!
  ;m
  
  \ FONTFILE_PATH FONT_SIZE ...         select current font and fontsize
  m: selectFont
    mFontSize!
    mFontFile.set

    mko String fullPath
    findResource(mFontFile.get)   ptrTo byte containingDir!
    if(containingDir)
      fullPath.set(containingDir)
      fullPath.append(mFontFile.get)
    endif
    
    TTF_OpenFont(fullPath.get mFontSize) mFont!
    if(mFont null =)
      reportError("selectFont")
      "Font file: " %s mFontFile.get %s " and size " %s mFontSize %d %nl
    endif

    fullPath~
  ;m
  
  \ FONT_SIZE ...               select current fontsize
  m: selectFontSize
    mFontSize!

    mko String fullPath
    findResource(mFontFile.get)   ptrTo byte containingDir!
    if(containingDir)
      fullPath.set(containingDir)
      fullPath.append(mFontFile.get)
    endif
    
    TTF_OpenFont(fullPath.get mFontSize) mFont!
    if(mFont null =)
      reportError("selectFontSize")
      "Font file: " %s mFontFile.get %s " and size " %s mFontSize %d %nl
    endif
    fullPath~
  ;m
  
  \ ...                 shutdown graphics system
  m: finish
    if( sdlStarted )
      SDL_QuitSubSystem(SDL_INIT_VIDEO)
      sdlStarted~
      TTF_Quit
    endif
  ;m
  
  \ ...                 startup graphics system
  m: start
    if( sdlStarted )
      end
    endif
    true sdlStarted!
  
    SDL_InitSubSystem(SDL_INIT_VIDEO) checkError("start: SDL_Init")
    
    if(SDL_CreateWindowAndRenderer(width height 0 window& renderer&))
      reportError("SDLGraphics:start - SDL_CreateWindowAndRenderer")
    else

      SDL_CreateRGBSurface(SDL_SWSURFACE width height bytesPerPixel 8 * 0 0 0 0) surface!
      checkError(surface 0= "SDLGraphics:start - SDL_CreateRGBSurface")
      SDL_CreateTexture(renderer SDL_PIXELFORMAT_ARGB8888 SDL_TEXTUREACCESS_STREAMING
        width height) windowTexture!
      checkError(windowTexture 0= "SDLGraphics:start - SDL_CreateTexture of windowTexture")

      \ surface %x %bl renderer %x %bl windowTexture %x %nl
      \ surface.format.BitsPerPixel
      \ "bits per pixel: " %s dup %d %nl

      setOriginMode(originMode)
      
      \ setup draw count and full-screen dirty rect for beginDraw/endDraw
      width srcPos.w!
      height srcPos.h!
      width dstPos.w!
      height dstPos.h!
      drawDepth~
      \ Initialize SDL_ttf library
      TTF_Init
      checkError("SDLGraphics:start - TTF_Init")

      selectFont(mFontFile.get mFontSize)
      
    endif
  ;m

  : beginDraw
    drawDepth++
  ;

  : endDraw
    if( drawDepth--@ 0= )
      \ At the end of the frame, we want to upload to the texture like this:
      SDL_UpdateTexture(windowTexture null surface.pixels bytesPerRow) drop
      SDL_RenderClear(renderer) drop
      SDL_RenderCopy(renderer windowTexture null null) drop
      SDL_RenderPresent(renderer)
  
    else
      if( drawDepth 0< )
        "endDraw called with drawDepth " %s drawDepth %d %nl
      endif
    endif
  ;

  \ beginFrame is called once per frame, when endFrame is called the result
  \ of all drawing after beginFrame was called is copied to the screen
  m: beginFrame
    beginDraw
  ;m
  
  m: endFrame
    endDraw
  ;m
  
  m: moveTo
    curY! curX!
  ;m

  m: positionWindow
    SDL_SetWindowPosition(window rot rot)
  ;m

  m: setWindowTitle
    SDL_SetWindowTitle(window swap)
  ;m
  
  \ X Y ... PIXEL_ADDRESS        get address of pixel(X, Y) on screen
  : screenAddress
    yPitch *
    swap
    xPitch * + centerPixelAddr +
  ;

  \ DX DY ... PIXEL_ADDRESS     get address of pixel(curX+DX, curY+DY) on screen 
  : screenAddressRelative
    curY + yPitch *
    swap
    curX + xPitch * + centerPixelAddr +
  ;

  \ PIXEL_ADDRESS ...           draw pixel at address if it is onscreen
  : setClippedPixel
    if(within(dup minPixelAddr maxPixelAddr))
      pixelColor swap _setRawPixel
    else
      drop
    endif
  ;
  
  \ drawPixel & drawPixelRelative assume that you are handling SDL_LockSurface/SDL_UnlockSurface/SDL_UpdateRects
  \ X Y drawPixel  - draw a dot at X,Y with color specified by pixelColor
  : drawPixel
    screenAddress setClippedPixel
  ;

  \ drawPixelRelative relies on its caller to set _setPixel to either _setRawPixel or setClippedPixel
  : drawPixelRelative
    screenAddressRelative _setPixel
  ;

  \ X Y getPixel -> fetches pixel value at X,Y
  m: getPixel
    screenAddress _getPixel
  ;m

  \ X Y isOnScreen -> true/false
  m: isOnScreen
  
    if( topLimit bottomLimit within )
      leftLimit rightLimit within
    else
      drop false
    endif
  ;m

  m: setColor
    pixelColor!
  ;m
  
  m: getColor
    pixelColor
  ;m
  
  \ x y drawPoint
  m: drawPoint
    beginDraw
  
    drawPixel
  
    endDraw
  ;m

  \ x y DrawLine
  m: drawLine
    curY int y0!
    curX int x0!
  
    2dup curY! curX!
  
    int y1!
    int x1!
  
    y1 y0 - int dy!
    x1 x0 - int dx!
    int stepx  int stepy
    int fraction
    int mStepX  int mStepY
    if( dy 0< )
      dy negate dy!
      -1 stepy!
    else
      1 stepy!
    endif
    yPitch stepy * mStepY!
    if( dx 0< )
      dx negate dx!
      -1 stepx!
    else
      1 stepx!
    endif
    stepx xPitch * mStepX!
    dy dy!+
    dx dx!+
  
    beginDraw

    screenAddress(x0 y0) ptrTo byte pPixel!

    if(within(pPixel minPixelAddr maxPixelAddr))
    andif(within(screenAddress(x1 y1) minPixelAddr maxPixelAddr))
      \ the line is completely on screen, do no clip checking
      fetch _setRawPixel _setPixel!
    else
      \ the line is at least partially off screen, check each pixel write
      ['] setClippedPixel _setPixel!
    endif
    
    pixelColor pPixel _setPixel
    if( dx dy > )
      dy dx 2/ - fraction!
      begin
      while( x0 x1 <> )
        if( fraction 0>= )
          stepy y0!+
          dx fraction!-
          mStepY pPixel!+
        endif
        stepx x0!+
        mStepX pPixel!+
        dy fraction!+
        pixelColor pPixel _setPixel
      repeat
    else
      dx dy 2/ - fraction!
      begin
      while( y0 y1 <> )
        if( fraction 0>= )
          stepx x0!+
          dy fraction!-
          mStepX pPixel!+
        endif
        stepy y0!+
        mStepY pPixel!+
        dx fraction!+
        pixelColor pPixel _setPixel
      repeat
    endif
    
    ['] setClippedPixel _setPixel!
    endDraw
  ;m

  \ XYPAIRS NUM_PAIRS drawPolyLine
  m: drawPolyLine
    \ TBD: reverse order of points?
    0 do
      drawLine
    loop
  ;m

  \ XYPAIRS NUM_LINES drawMultiLine
  m: drawMultiLine
    0 do
      2swap moveTo drawLine
    loop
  ;m

  \ X Y _draw4EllipsePoints ...
  : _draw4EllipsePoints
    2dup
    int y!
    int x!
    negate int ny!
    negate int nx!
  
    drawPixelRelative( x y )
    drawPixelRelative( x ny )
    drawPixelRelative( nx y )
    drawPixelRelative( nx ny )
  ;

  \ WIDTH HEIGHT ...        draw unfilled ellipse @ curX,curY
  m: drawEllipse
    beginDraw
  
    int h!
    int w!
    int x      int y
    int dx     int dy
    int endX   int endY
    0 int ellipseError!
  
    w w * dup int wSquared!
    2*        int twoWSquared!
    h h * dup int hSquared!
    2*        int twoHSquared!

    w 2/ 1+ x!
    h 2/ 1+ y!
    if(within(curX leftLimit x + rightLimit x -))
    andif(within(curY bottomLimit y + topLimit y -))
      \ the ellipse is completely on screen, do no clip checking
      fetch _setRawPixel _setPixel!
    else
      \ the line is at least partially off screen, check each pixel write
      ['] setClippedPixel _setPixel!
    endif
    
    w x!
    0 y!
 
   \ TBD: 1.2 
    1 w 2* - hSquared * dx!
    wSquared dy!
    twoHSquared w * endX!
    0 endY!

    begin
    while( endX endY >= )
      _draw4EllipsePoints( x y )
      y++
      twoWSquared endY!+
      dy ellipseError!+
      twoWSquared dy!+
      if( ellipseError 2* dx + 0> )
        x--
        twoHSquared endX!-
        dx ellipseError!+
        twoHSquared dx!+
      endif
    repeat
 
    \ 1st point set is done; start the 2nd set of points
    x~
    h y!
    hSquared dx!
    1 h 2* - wSquared * dy!
    ellipseError~
    endX~
    twoWSquared h * endY!
    begin
    while( endX endY <= )
      _draw4EllipsePoints( x y )
      x++
      twoHSquared endX!+
      dx ellipseError!+
      twoHSquared dx!+
      if( ellipseError 2* dy + 0> )
      	y--
      	twoWSquared endY!-
      	dy ellipseError!+
      	twoWSquared dy!+
      endif
    repeat
  
    ['] setClippedPixel _setPixel!
    endDraw
  ;m


  : _drawCirclePoints
    2dup
    int y!
    int x!
    negate int ny!
    negate int nx!
    if( x 0= )
      0 y drawPixelRelative
      0 ny drawPixelRelative
      y 0 drawPixelRelative
      ny 0 drawPixelRelative
    else
      if( x y = )
        x y drawPixelRelative
        nx y drawPixelRelative
        x ny drawPixelRelative
        nx ny drawPixelRelative
      else
        if( x y < )
          x y drawPixelRelative
          nx y drawPixelRelative
          x ny drawPixelRelative
          nx ny drawPixelRelative
          y x drawPixelRelative
          y nx drawPixelRelative
          ny x drawPixelRelative
          ny nx drawPixelRelative
        endif
      endif
    endif
  ;

  m: drawCircle     \ RADIUS ...        draw unfilled circle @ curX,curY
    beginDraw
  
    dup int r!
    int y!
    0 int x!
    5 r 4* - 4 / int p!

    r++
    if(within(curX leftLimit r@+ rightLimit r@-))
    andif(within(curY bottomLimit r@+ topLimit r@-))
      \ the circle is completely on screen, do no clip checking
      fetch _setRawPixel _setPixel!
    else
      \ the line is at least partially off screen, check each pixel write
      ['] setClippedPixel _setPixel!
    endif
    r--
    
    begin
    while( x y < )
      x++
      if( p 0< )
        x 2* 1+ p!+
      else
        y--
        x y - 2* 1+ p!+
      endif
      _drawCirclePoints( x y )
    repeat
  
    ['] setClippedPixel _setPixel!
    endDraw
  ;m

  m: drawSquare \ SIZE ...          draw unfilled square @ curX,curY
    beginDraw
    int size!
    curX curY
  
    drawLine( curX curY size + )
    drawLine( curX size + curY )
    drawLine( curX curY size - )
    drawLine( curX size - curY )
    moveTo
    endDraw
  ;m

  m: drawRectangle  \ WIDTH HEIGHT ...      draw unfilled rectangle @ curX,curY
    beginDraw
    int h!
    int w!
    curX curY
  
    drawLine( curX curY h@+ )
    drawLine( curX w@+ curY )
    drawLine( curX curY h@- )
    drawLine( curX w@- curY )
    moveTo
    endDraw
  ;m

  m: fillRectangle  \ WIDTH HEIGHT ...      draw filled rectangle @ curX,curY
    beginDraw
    SDL_Rect rect
    rect.h!
    rect.w!
    curX centerX@+ rect.x!
    centerY curY@- rect.h@- rect.y!
    SDL_FillRect( surface rect pixelColor ) drop
    endDraw
  ;m

  m: clear  \ FILL_COLOR ...                clear entire screen to FILL_COLOR
    beginDraw
    SDL_FillRect( surface null rot ) drop
    endDraw
  ;m
  
  m: flip
    \ SDL_Flip( surface ) drop
  ;m

  m: drawBitmap     \ BITMAP_FILE_PATH ...      draw bitmap from file @ curX,curY
    \ load entire bitmap into srcSurface
    SDL_LoadBMP ptrTo SDL_Surface srcSurface!
    \ calculate where to copy bitmap to (curX,curY -> dstPos)
    SDL_Rect dstPos
    curX centerX@+ dstPos.x!
    if(originMode kScreenOriginTopLeft =)
      centerY curY@+ dstPos.y!
    else
      centerY curY@- srcSurface.h@- dstPos.y!
    endif
     
    beginDraw
    SDL_BlitSurface(srcSurface null surface dstPos)
    SDL_FreeSurface(srcSurface)
    endDraw
  ;m

  m: imageLoad  \ IMAGEFILE_PATH ...    draw bitmap from file using IMG library at origin
    IMG_Load ptrTo SDL_Surface srcSurface!
    SDL_BlitSurface(srcSurface null surface null)
    checkError("imageLoad - SDL_BlitSurface")
    SDL_FreeSurface(srcSurface)
  ;m
  
  m: drawText   \ TEXT_STR ...          draw text string @ curX,curY
    SDL_Rect dstPos
    curX centerX + dstPos.x!
    if(originMode kScreenOriginTopLeft =)
      centerY curY@+ dstPos.y!
    else
      centerY curY@- dstPos.y!
    endif

    beginDraw
    TTF_RenderText_Solid(mFont swap pixelColor ) int textSurface!
    \ textSurface %x %bl gfxInstance.surface %x %nl
  
    if( textSurface null = )
      reportError("drawText - TTF_RenderText_Solid")
    else
      SDL_BlitSurface( textSurface null surface dstPos )
      checkError("drawText - SDL_BlitSurface")
    endif
  
    endDraw
  ;m
  
  m: getTextSize        \ TEXT_STR ... WIDTH HEIGHT
    ptrTo byte pText!
    int w   int h

    TTF_SizeText(mFont pText w& h&))
    checkError("calculateSizeOfText")

    w h
  ;m
  
;class

loaddone
