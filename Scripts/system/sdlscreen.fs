
autoforget sdlscreen
requires sdl2
requires sdl2_image
requires sdl2_ttf
also sdl2
also sdl2_image
also sdl2_ttf

: sdlscreen ;

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
  
  m: imageLoad  // takes ptr to image filename string
    freeSurface
    IMG_Load surface!
  ;m
  
  m: reportError
    "Error: " %s %s " - " %s SDL_GetError %s %nl
  ;m
  
  m: checkError  // take error flag on TOS
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

class: SDLScreen extends SDLSurface

  cell window           // SDL_Window
  cell renderer         // SDL_Renderer
  cell windowTexture    // SDL_Texture

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
  cell centerPixelAddr
  cell minPixelAddr
  cell maxPixelAddr
  int drawDepth
  SDL_Rect srcPos
  SDL_Rect dstPos
  int sdlStarted
  int curX
  int curY
  int bytesPerPixel
  int bytesPerRow
  SDLScreenOriginMode originMode  // must be set before init or initWithSize
  int xPitch        // like bytesPerPixel, but takes x axis orientation into account
  int yPitch        // like bytesPerRow, but takes y axis orientation into account
  cell mFont
  String mFontFile
  int mFontSize

  : dummyFetchOp drop 0 ;
  
  m: delete
    mFontFile~
  ;m
  
  m: imageLoad  // takes ptr to image filename string
    IMG_Load ptrTo SDL_Surface srcSurface!
    SDL_BlitSurface(srcSurface surface null null)
    checkError("SDL_BlitSurface")
    SDL_FreeSurface(srcSurface)
  ;m
  
  m: initLimits
    dup height!   2/ dup bottomLimit!   dup centerY!   negate topLimit!
    dup width!    2/ dup rightLimit!    dup centerX!   negate leftLimit!
  ;m
  
  m: initWithSize
    int windowHeight!
    int windowWidth!
    surface~
    window~
    renderer~
    windowTexture~
    
    initLimits( windowWidth windowHeight )
    
    ['] dummyFetchOp _getPixel!
    ['] 2drop dup _setPixel! _setRawPixel!
    0x00FFFF00 pixelColor!
    
    defaultWindowBytesPerPixel bytesPerPixel!
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
  
  m: init
    initWithSize(defaultWindowWidth defaultWindowHeight)
  ;m

  m: setOriginMode
    originMode!
    if(originMode kScreenOriginTopLeft =)
      // set coordinates so top left of screen is 0,0 and positive Y is down
      centerX~   width rightLimit!     leftLimit~
      centerY~   height bottomLimit!   topLimit~
      surface.pitch yPitch!
    else
      // set coordinates so center of screen is 0,0 and positive Y is up
      width 2/ centerX!     rightLimit negate leftLimit!
      height 2/ centerY!    bottomLimit negate topLimit!
      surface.pitch negate yPitch!
    endif
    
    surface.pixels minPixelAddr!
    width height * bytesPerPixel * minPixelAddr + maxPixelAddr!
    bytesPerPixel xPitch!
    centerY surface.pitch *
    centerX bytesPerPixel *
    + minPixelAddr + centerPixelAddr!
  ;m
  
  m: selectFont // fontFilename fontSize ...
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
      reportError("SDLScreen:selectFont")
      "Font file: " %s mFontFile.get %s " and size " %s mFontSize %d %nl
    endif

    fullPath~
  ;m
  
  m: selectFontSize // fontSize ...
    mFontSize!

    mko String fullPath
    findResource(mFontFile.get)   ptrTo byte containingDir!
    if(containingDir)
      fullPath.set(containingDir)
      fullPath.append(mFontFile.get)
    endif
    
    TTF_OpenFont(fullPath.get mFontSize) mFont!
    if(mFont null =)
      reportError("SDLScreen:selectFontSize")
      "Font file: " %s mFontFile.get %s " and size " %s mFontSize %d %nl
    endif
    fullPath~
  ;m
  
  m: end
    if( sdlStarted )
      SDL_Quit
      sdlStarted~
      TTF_Quit
    endif
  ;m
  
  m: start
    if( sdlStarted )
      end
    endif
    true sdlStarted!
  
    SDL_Init(SDL_INIT_VIDEO) checkError("start: SDL_Init")
    
    //SDL_GetVideoInfo ptrTo SDL_VideoInfo videoInfo!
    //"SDL_GetVideoInfo returns " %s videoInfo.current_w %d "x" %s videoInfo.current_h %d %bl
    //videoInfo.vfmt.BitsPerPixel %d " bits/pixel " %s %bl videoInfo.video_mem %d " bytes of video memory\n" %s
   
    //initLimits( videoInfo.current_w videoInfo.current_h )
    
    //SDL_SetVideoMode( width height 0 SDL_SWSURFACE ) surface!
    
    //SDL_CreateWindow( "My Game Window" SDL_WINDOWPOS_UNDEFINED_MASK SDL_WINDOWPOS_UNDEFINED_MASK width height SDL_WINDOW_SHOWN ) window!
    if(SDL_CreateWindowAndRenderer(width height 0 window& renderer&))
      reportError("SDLScreen:start - SDL_CreateWindowAndRenderer")
    else

      SDL_CreateRGBSurface(SDL_SWSURFACE width height bytesPerPixel 8 * 0 0 0 0) surface!
      checkError(surface 0= "SDLScreen:start - SDL_CreateRGBSurface")
      //SDL_CreateRenderer(window -1 0) renderer!
      SDL_CreateTexture(renderer SDL_PIXELFORMAT_ARGB8888 SDL_TEXTUREACCESS_STREAMING width height) windowTexture!
      checkError(windowTexture 0= "SDLScreen:start - SDL_CreateTexture of windowTexture")

      //surface %x %bl renderer %x %bl windowTexture %x %nl
      //surface.format.BitsPerPixel
      //"bits per pixel: " %s dup %d %nl
      bytesPerPixel
      case
        of( 1 )
          ['] c@ _getPixel!
          ['] c! dup _setPixel! _setRawPixel!
          0x55 pixelColor!
        endof
        of( 2 )
          ['] s@ _getPixel!
          ['] s! dup _setPixel! _setRawPixel!
          0x5555 pixelColor!
        endof
        of( 4 )
          ['] i@ _getPixel!
          ['] i! dup _setPixel! _setRawPixel!
          0x00FFFF00 pixelColor!
        endof
        // unhandled pixel size
        ['] dummyFetchOp _getPixel!
        ['] 2drop dup _setPixel! _setRawPixel!
        "startSDL: Unhandled pixel size " %s dup %d %nl
      endcase

      setOriginMode(originMode)
      
      // setup draw count and full-screen dirty rect for beginDraw/endDraw
      width srcPos.w!
      height srcPos.h!
      width dstPos.w!
      height dstPos.h!
      drawDepth~
      // Initialize SDL_ttf library
      TTF_Init
      checkError("SDLScreen:start - TTF_Init")

      selectFont(mFontFile.get mFontSize)
      
    endif
  ;m

  : beginDraw
    drawDepth++
  ;

  : endDraw
    if( drawDepth--@ 0= )
      // At the end of the frame, we want to upload to the texture like this:
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
  
  // X Y screenAddress -> address of pixel on screen
  : screenAddress
    yPitch *
    swap
    xPitch * + centerPixelAddr +
  ;

  // dX dY screenAddressRelative -> address of pixel on screen
  : screenAddressRelative
    curY + yPitch *
    swap
    curX + xPitch * + centerPixelAddr +
  ;

  : setClippedPixel
    if(within(dup minPixelAddr maxPixelAddr))
      pixelColor swap _setRawPixel
    else
      drop
    endif
  ;
  
  // drawPixel & drawPixelRelative assume that you are handling SDL_LockSurface/SDL_UnlockSurface/SDL_UpdateRects
  // X Y drawPixel  - draw a dot at X,Y with color specified by pixelColor
  : drawPixel
    screenAddress setClippedPixel
  ;

  // drawPixelRelative relies on its caller to set _setPixel to either _setRawPixel or setClippedPixel
  : drawPixelRelative
    screenAddressRelative _setPixel
  ;

  // X Y getPixel -> fetches pixel value at X,Y
  m: getPixel
    screenAddress _getPixel
  ;m

  // X Y isOnScreen -> true/false
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
  
  // x y drawPoint
  m: drawPoint
    beginDraw
  
    drawPixel
  
    endDraw
  ;m

  // x y DrawLine
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
      // the line is completely on screen, do no clip checking
      fetch _setRawPixel _setPixel!
    else
      // the line is at least partially off screen, check each pixel write
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

  // XYPAIRS NUM_PAIRS drawPolyLine
  m: drawPolyLine
    // TBD: reverse order of points?
    0 do
      drawLine
    loop
  ;m

  // XYPAIRS NUM_LINES drawMultiLine
  m: drawMultiLine
    0 do
      2swap moveTo drawLine
    loop
  ;m

  // X Y _draw4EllipsePoints ...
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

  // WIDTH HEIGHT DrawEllipse ...
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
      // the ellipse is completely on screen, do no clip checking
      fetch _setRawPixel _setPixel!
    else
      // the line is at least partially off screen, check each pixel write
      ['] setClippedPixel _setPixel!
    endif
    
    w x!
    0 y!
 
   // TBD: 1.2 
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
 
    // 1st point set is done; start the 2nd set of points
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

  m: drawCircle
    beginDraw
  
    dup int r!
    int y!
    0 int x!
    5 r 4* - 4 / int p!

    r++
    if(within(curX leftLimit r@+ rightLimit r@-))
    andif(within(curY bottomLimit r@+ topLimit r@-))
      // the circle is completely on screen, do no clip checking
      fetch _setRawPixel _setPixel!
    else
      // the line is at least partially off screen, check each pixel write
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

  m: drawSquare
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

  m: drawRectangle
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

  m: fillRectangle
    beginDraw
    SDL_Rect rect
    rect.h!
    rect.w!
    curX centerX@+ rect.x!
    centerY curY@- rect.h@- rect.y!
    SDL_FillRect( surface rect pixelColor ) drop
    endDraw
  ;m

  m: clear
    beginDraw
    SDL_FillRect( surface null rot ) drop
    endDraw
  ;m
  
  m: flip
    //SDL_Flip( surface ) drop
  ;m

  m: drawBitmap
    SDL_LoadBMP() ptrTo SDL_Surface bmSurface!
    SDL_Rect dstPos
    curX centerX@+ dstPos.x!
    if(originMode kScreenOriginTopLeft =)
      centerY curY@+ dstPos.y!
    else
      centerY curY@- bmSurface.h@- dstPos.y!
    endif
     
    beginDraw
    SDL_BlitSurface(bmSurface null surface dstPos)
    SDL_FreeSurface(bmSurface)
    endDraw
  ;m

  m: drawText
    SDL_Rect dstPos
    curX centerX + dstPos.x!
    if(originMode kScreenOriginTopLeft =)
      centerY curY@+ dstPos.y!
    else
      centerY curY@- dstPos.y!
    endif

    beginDraw
    TTF_RenderText_Solid(mFont swap pixelColor ) int textSurface!
    //textSurface %x %bl screenInstance.surface %x %nl
  
    if( textSurface null = )
      "SDLScreen:drawText - TTF_RenderText_Solid() Failed: " %s SDL_GetError %s %nl
    else
      if( SDL_BlitSurface( textSurface null surface dstPos ) )
        "SDLScreen:drawText - SDL_BlitSurface() Failed: " %s SDL_GetError %s %nl
      endif
    endif
  
    endDraw
  ;m
  
;class

SDLScreen screenInstance

: startSDLWithSize
  new SDLScreen screenInstance!
  screenInstance.initWithSize
  screenInstance.start
;

: startSDL
  new SDLScreen screenInstance!
  screenInstance.init
  screenInstance.start
;

: endSDL
  if( screenInstance objNotNull )
    screenInstance.end
    screenInstance~
  endif
;

: sc screenInstance.setColor ;
: mt screenInstance.moveTo ;

: dp screenInstance.drawPoint ;
: dl screenInstance.drawLine ;
: de screenInstance.drawEllipse ;
: dc screenInstance.drawCircle ;
: dsq screenInstance.drawSquare ;
: dr screenInstance.drawRectangle ;
: dpl screenInstance.drawPolyLine ;
: dml screenInstance.drawMultiLine ;
: fs screenInstance.flip ;
: cl screenInstance.clear( 0 ) fs ;
: dbmap screenInstance.drawBitmap ;
: fr screenInstance.fillRectangle ;
: fsq dup screenInstance.fillRectangle ;
: beginFrame screenInstance.beginFrame ;
: endFrame screenInstance.endFrame ;
: dt screenInstance.drawText ;

: munch
  int v  int x  int y
  
  beginFrame
  do(256 0)
    do(256 0)
      //screenInstance.getPixel(i j) v!
      //xor(i j) 0x10101 * v!
      xor(i j) 0x10101 * v!
      sc(v)
      dp(i j)
    loop
  loop
  endFrame
;

: test
  sc( 0xFFFF00 )
  de( 100 200 )
  sc( 0x00FFFF )
  dsq( 55 )
  do( 256 0 )
    sc( i )
    dp( i i )
    sc( lshift( i  8 ) )
    dp( i negate i )
    sc( lshift( i  16 ) )
    dp( i i negate )
    sc( i 0x10101 * )
    dp( i negate i negate )
  loop
;

3.14159d double pi!

: test2
  pi -1.0d d* double angle!
  pi 4.0d d* 200.0d d/ double dangle!
  
  100.0d double yscale!
  sc( -1 )
  do( 200 -200 )
    //i %d
    //dp( i i )
    dsin( angle ) double y!
    dp( i y y d* y d* yscale d* d2i )
    dangle ->+ angle
  loop
;

: test3
  mt(0 0)
  dbmap("SimFunWorld.bmp")
;

previous

loaddone

