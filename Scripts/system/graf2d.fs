
autoforget graf2d
requires sdl2
also sdl2

: graf2d ;

720 -> screenW
640 -> screenH

// x y DrawPoint
: DrawPoint
  beginDraw
  
  drawPixel
  
  endDraw
;

// x y DrawLine
: DrawLine
  curY negate -> int y0
  curX -> int x0
  
  2dup -> curY -> curX
  
  negate -> int y1
  -> int x1
  
  y1 y0 - -> int dy
  x1 x0 - -> int dx
  int stepx  int stepy
  int fraction
  int mStepX  int mStepY
  if( dy 0< )
    dy negate -> dy
    -1 -> stepy
  else
    1 -> stepy
  endif
  sdlScreen.pitch stepy * -> mStepY
  if( dx 0< )
    dx negate -> dx
    -1 -> stepx
  else
    1 -> stepx
  endif
  stepx sdlScreen.format.BytesPerPixel * -> mStepX
  dy ->+ dy
  dx ->+ dx
  
  beginDraw

  x0 screenCenterX + sdlScreen.format.BytesPerPixel *
  y0 screenCenterY + sdlScreen.pitch *
  + sdlScreen.pixels +
  -> ptrTo byte pPixel

  pixelColor pPixel pixel!
  if( dx dy > )
    dy dx 2/ - -> fraction
    begin
      x0 x1 <>
    while
      if( fraction 0>= )
        stepy ->+ y0
        dx ->- fraction
        mStepY ->+ pPixel
      endif
      stepx ->+ x0
      mStepX ->+ pPixel
      dy ->+ fraction
      pixelColor pPixel pixel!
    repeat
  else
    dx dy 2/ - -> fraction
    begin
      y0 y1 <>
    while
      if( fraction 0>= )
        stepx ->+ x0
        dy ->- fraction
        mStepX ->+ pPixel
      endif
      stepy ->+ y0
      mStepY ->+ pPixel
      dx ->+ fraction
      pixelColor pPixel pixel!
    repeat
  endif
  endDraw
;

// XYPAIRS NUM_PAIRS DrawPolyLine
: DrawPolyLine
  0 do
    DrawLine
  loop
;

// X Y _draw4EllipsePoints ...
: _draw4EllipsePoints
  2dup
  -> int y
  -> int x
  negate -> int ny
  negate -> int nx
  
  x y drawPixelRelative
  x ny drawPixelRelative
  nx y drawPixelRelative
  nx ny drawPixelRelative
;

// WIDTH HEIGHT DrawEllipse ...
: DrawEllipse
  beginDraw
  
  -> int h
  -> int w
  int x      int y
  int dx     int dy
  int endX   int endY
  0 -> int ellipseError
  
  w w * dup -> int wSquared
  2*        -> int twoWSquared
  h h * dup -> int hSquared
  2*        -> int twoHSquared
  
  w -> x
  0 -> y
 
  // TBD: 1.2 
  1 w 2* - hSquared * -> dx
  wSquared -> dy
  twoHSquared w * -> endX
  0 -> endY

  begin
    endX endY >=
  while
    _draw4EllipsePoints( x y )
    1 ->+ y
    twoWSquared ->+ endY
    dy ->+ ellipseError
    twoWSquared ->+ dy
    if( ellipseError 2* dx + 0> )
      1 ->- x
      twoHSquared ->- endX
      dx ->+ ellipseError
      twoHSquared ->+ dx
    endif
  repeat
 
  // 1st point set is done; start the 2nd set of points
  0 -> x
  h -> y
  hSquared -> dx
  1 h 2* - wSquared * -> dy
  0 -> ellipseError
  0 -> endX
  twoWSquared h * -> endY
  begin
    endX endY <=
  while
    _draw4EllipsePoints( x y )
    1 ->+ x
    twoHSquared ->+ endX
    dx ->+ ellipseError
    twoHSquared ->+ dx
    if( ellipseError 2* dy + 0> )
    	1 ->- y
    	twoWSquared ->- endY
    	dy ->+ ellipseError
    	twoWSquared ->+ dy
    endif
  repeat
  
  endDraw
;


: _drawCirclePoints
  2dup
  -> int y
  -> int x
  negate -> int ny
  negate -> int nx
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

: DrawCircle
  beginDraw
  
  dup -> int r
  -> int y
  0 -> int x
  5 r 4* - 4 / -> int p
  
  x y _drawCirclePoints

  begin
    x y <
  while
    1 ->+ x
    if( p 0< )
      x 2* 1+ ->+ p
    else
      1 ->- y
      x y - 2* 1+ ->+ p
    endif
    x y _drawCirclePoints
  repeat
  
  endDraw
;

: DrawSquare
  -> int size
  curX curY
  
  curX size - curY size + moveTo
  curX curY size 2* - DrawLine
  curX size 2* + curY DrawLine
  curX curY size 2* + DrawLine
  curX size 2* - curY DrawLine
  moveTo
;  

: DrawRectangle
  -> int h
  -> int w
  curX curY
  
  curX curY h + DrawLine
  curX w + curY DrawLine
  curX curY h - DrawLine
  curX w - curY DrawLine
  moveTo
;

: dp DrawPoint ;
: dl DrawLine ;
: de DrawEllipse ;
: dc DrawCircle ;
: dsq DrawSquare ;
: dr DrawRectangle ;
: dpl DrawPolyLine ;

: test
  beginDraw
  256 0 do
    i -> pixelColor
//    i 20 + i 10 + DrawEllipse
    //i 10 + DrawCircle
    dp( i i )
  loop
  endDraw
;

3.14159g -> double pi

: test2
  //beginDraw
  pi -1.0g d* -> double angle
  pi 4.0g d* 200.0g d/ -> double dangle
  
  100.0g -> double yscale
  -1 -> pixelColor
  do( 200 -200 )
    //i %d
    //dp( i i )
    dsin( angle ) -> double y
    dp( i y y d* y d* yscale d* d2i )
    dangle ->+ angle
  loop
  //endDraw
;

previous

loaddone
