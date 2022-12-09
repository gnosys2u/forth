
autoforget sdltest
: sdltest ;

requires sdlscreen
also sdl2

#if 1
ptrTo int worldBM
SDL_LoadBMP( "SimFunWorld.bmp" ) -> worldBM

SDL_Rect srcPos
SDL_Rect dstPos

0 -> srcPos.x 0 -> srcPos.y
640 -> srcPos.w 480 -> srcPos.h

0 -> dstPos.x 0 -> dstPos.y
640 -> dstPos.w 480 -> dstPos.h

: test
  SDL_UpperBlit( worldBM srcPos sdlScreen dstPos ) drop
  SDL_UpdateRects( sdlScreen 1 dstPos )
;
#endif

		
: %sx swap %bl %bl %s "=0x" %s %x ;

: showKeyEvent
  -> ptrTo SDL_KeyboardEvent ke
  "type" ke.type %sx  "which" ke.which %sx   "state" ke.state %sx
  "scancode" ke.scancode %sx
  "sym" ke.sym %sx
  "mod" ke.scancode %sx
  "unicode" ke.unicode %sx
  %nl
;

: showButtonEvent
  -> ptrTo SDL_MouseButtonEvent mbe
  "type" mbe.type %sx "which" mbe.which %sx "button" mbe.button "state" mbe.state %sx
  "x" mbe.x %sx "y" mbe.y %sx
  %nl
;

: showMotionEvent
  -> ptrTo SDL_MouseMotionEvent mme
  "type" mme.type %sx "which" mme.which %sx "state" mme.state %sx
  "x" mme.x %sx "y" mme.y %sx
  "xrel" mme.xrel %sx "yrel" mme.yrel %sx
  %nl
;

: testEvents
  SDL_Event ev
  startSDL

  false -> int buttonDown
  sc( -1 )		// draw in white
  
  do( 800 0 )
    SDL_WaitEvent( ev ) drop
    //ds
    case( ev.type )
      of( SDL_KEYDOWN )
        "key down " %s showKeyEvent( ev.key )
      endof
      
      of( SDL_KEYUP )
        "key up " %s showKeyEvent( ev.key )
      endof
      
      of( SDL_MOUSEMOTION )			// Mouse moved
        "mouse motion " %s showMotionEvent( ev.motion )
        if( buttonDown )
          ev.motion.x screenW 2/ + screenH 2/ ev.motion.y - dp
        endif
      endof
      
      of( SDL_MOUSEBUTTONDOWN )		// Mouse button pressed
        "mouse down " %s showButtonEvent( ev.button )
        dp( ev.button.x screenW 2/ + screenH 2/ ev.button.y - )
	    true -> buttonDown
      endof
      
      of( SDL_MOUSEBUTTONUP )		// Mouse button released
        "mouse up " %s showButtonEvent( ev.motion )
	    false -> buttonDown
      endof
      
      // default:
      "event type 0x" %s dup %x %bl %bl
      dump( ev 16 )
    endcase
  loop
  
  endSDL
;
previous

