

require sdlgraphics.fs

autoforget sdlscreen
: sdlscreen ;


SDLGraphics gfxInstance

: startSDLWithSize      \ width height bytesPerPixel
  new SDLGraphics gfxInstance!
  gfxInstance.configure
  gfxInstance.init
  gfxInstance.start
;

: startSDL
  new SDLGraphics gfxInstance!
  gfxInstance.init
  gfxInstance.start
;

: endSDL
  if( gfxInstance objNotNull )
    gfxInstance.finish
    gfxInstance~
  endif
;

: sc gfxInstance.setColor ;
: mt gfxInstance.moveTo ;

: dp gfxInstance.drawPoint ;
: dl gfxInstance.drawLine ;
: de gfxInstance.drawEllipse ;
: dc gfxInstance.drawCircle ;
: dsq gfxInstance.drawSquare ;
: dr gfxInstance.drawRectangle ;
: dpl gfxInstance.drawPolyLine ;
: dml gfxInstance.drawMultiLine ;
: fs gfxInstance.flip ;
: cl gfxInstance.clear( 0 ) fs ;
: dbmap gfxInstance.drawBitmap ;
: fr gfxInstance.fillRectangle ;
: fsq dup gfxInstance.fillRectangle ;
: beginFrame gfxInstance.beginFrame ;
: endFrame gfxInstance.endFrame ;
: dt gfxInstance.drawText ;

: munch
  int v  int x  int y
  
  beginFrame
  do(256 0)
    do(256 0)
      \ gfxInstance.getPixel(i j) v!
      \ xor(i j) $10101 * v!
      xor(i j) $10101 * v!
      sc(v)
      dp(i j)
    loop
  loop
  endFrame
;

: test
  sc( $FFFF00 )
  de( 100 200 )
  sc( $00FFFF )
  dsq( 55 )
  do( 256 0 )
    sc( i )
    dp( i i )
    sc( lshift( i  8 ) )
    dp( i negate i )
    sc( lshift( i  16 ) )
    dp( i i negate )
    sc( i $10101 * )
    dp( i negate i negate )
  loop
;

3.14159E sfloat pi!

: test2
  pi -1.0E sf* sfloat angle!
  pi 4.0E sf* 200.0E sf/ sfloat dangle!
  
  100.0E sfloat yscale!
  sc( -1 )
  do( 200 -200 )
    \ i %d
    \ dp( i i )
    sfsin( angle ) sfloat y!
    dp( i y y sf* y sf* yscale sf* df>i )
    dangle angle!+
  loop
;

: test3
  mt(0 0)
  dbmap("SimFunWorld.bmp")
;

previous

loaddone

