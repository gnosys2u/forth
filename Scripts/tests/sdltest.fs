requires sdl2
requires sdl2_ttf

autoforget sdltest

forth definitions
also sdl2
also sdl2_ttf

: sdltest ;

"FreeSans.ttf" -> 40 string fontName
18 constant DEFAULT_PTSIZE
"The quick brown fox jumped over the lazy dog" -> 100 string DEFAULT_TEXT
256 constant NUM_COLORS
640 constant WIDTH
480 constant HEIGHT

struct: Scene
  cell captionTexture          // SDL_Texture*
  SDL_Rect captionRect
  cell messageTexture          // SDL_Texture*
  SDL_Rect messageRect
;struct

: drawScene  // SDL_Renderer *renderer, Scene *scene
  ptrTo Scene scene!
  cell renderer!     // SDL_Renderer*
  
  // Clear the background to background color
  SDL_SetRenderDrawColor(renderer 0xFF 0xFF 0xFF 0xFF) drop
  SDL_RenderClear(renderer) drop

  SDL_RenderCopy(renderer scene.captionTexture null scene.captionRect) drop
  SDL_RenderCopy(renderer scene.messageTexture null scene.messageRect) drop
  SDL_RenderPresent(renderer)
;

: cleanup
  TTF_Quit
  SDL_Quit
;

: failed
  %s %bl SDL_GetError %s %nl
;

: test
  cell window        // SDL_Window *window;
  cell renderer      // SDL_Renderer *renderer;
  cell font          // TTF_Font *font;
  ptrTo SDL_Surface text          // SDL_Surface *text;
  Scene scene
  DEFAULT_PTSIZE int ptsize!
  0xFFFFFF int white!
  0 int black!
  white int backcol!
  black int forecol!
  
  SDL_Event event
  0 int rendersolid!
  TTF_STYLE_NORMAL int renderstyle!
  0 int outline!
  TTF_HINTING_NORMAL int hinting!
  1 int kerning!
  
  // Initialize the TTF library
  if(TTF_Init() 0<)
    failed("TTF_Init")
    SDL_Quit
    exit
  endif

  // Open the font file with the requested point size
  TTF_OpenFont(fontName ptsize) font!
  if(font null =)
    failed("TTF_OpenFont")
    exit
  endif

  TTF_SetFontStyle(font renderstyle)
  TTF_SetFontOutline(font outline)
  TTF_SetFontKerning(font kerning)
  TTF_SetFontHinting(font hinting)

  // Create a window
  if(SDL_CreateWindowAndRenderer(WIDTH HEIGHT 0 window& renderer&) 0<)
    failed("SDL_CreateWindowAndRenderer")
    exit
  endif

  // Show which font file we're looking at
  if(rendersolid)
    TTF_RenderText_Solid(font fontName forecol) text!
  else
    TTF_RenderText_Shaded(font fontName forecol backcol) text!
  endif

  if(text null <>)
    4 scene.captionRect.x!
    4 scene.captionRect.y!
	text.w scene.captionRect.w!
    text.h scene.captionRect.h!
    SDL_CreateTextureFromSurface(renderer text) scene.captionTexture!
    SDL_FreeSurface(text)
  endif

  // Render and center the message
  //message = DEFAULT_TEXT

  if(rendersolid)
    TTF_RenderText_Solid(font DEFAULT_TEXT forecol) text!
  else
    TTF_RenderText_Shaded(font DEFAULT_TEXT forecol backcol) text!
  endif

  if(text null =)
    failed("Couldn't render text")
    exit
  endif
  
  WIDTH text.w - 2/ scene.messageRect.x!
  HEIGHT text.h - 2/ scene.messageRect.y!
  text.w scene.messageRect.w!
  text.h scene.messageRect.h!
  SDL_CreateTextureFromSurface(renderer text) scene.messageTexture!

  drawScene(renderer scene)

  // Wait for a keystroke and blit text on mouse press
  true int notDone!
  begin
  while(notDone)
    "notDone\n" %s
    
    if(SDL_WaitEvent(event) 0<)
      failed("SDL_WaitEvent")
      false notDone!
   	endif
    
    case(event.common.type)
      of(SDL_MOUSEBUTTONDOWN)
        event.button.x text.w 2/ - scene.messageRect.x!
        event.button.y text.h 2/ - scene.messageRect.y!
        //event -> ptrTo SDL_MouseButtonEvent mmb
        //mmb.x text.w 2/ - scene.messageRect.x!
        //mmb.y text.h 2/ - scene.messageRect.y!
        text.w scene.messageRect.w!
        text.h scene.messageRect.h!
        drawScene(renderer scene)
      endof

      of(SDL_KEYDOWN)
        "KEYDOWN!" %s %nl
        false notDone!
      endof
      
      of(SDL_QUIT)
        "QUIT!" %s %nl
        false notDone!
      endof
      
    endcase
    
  repeat

  SDL_FreeSurface(text)
  TTF_CloseFont(font)
  SDL_DestroyTexture(scene.captionTexture)
  SDL_DestroyTexture(scene.messageTexture)
  cleanup
;

previous

loaddone


autoforget sdltest
: sdltest ;

requires sdlscreen
also sdl2

#if 1
cell worldBM
SDL_LoadBMP( "SimFunWorld.bmp" ) worldBM!

SDL_Rect srcPos
SDL_Rect dstPos

0 srcPos.x!     0 srcPos.y!
640 srcPos.w!   480 srcPos.h!

0 dstPos.x!     0 dstPos.y!
640 dstPos.w!   480 dstPos.h!

: test
  SDL_UpperBlit( worldBM srcPos screenInstance dstPos ) drop
  SDL_UpdateWindowSurfaceRects(screenInstance.window dstPos 1)
;
#endif

		
: %sx swap %bl %bl %s "=0x" %s %x ;

: showKeyEvent
  ptrTo SDL_KeyboardEvent ke!
  "type" ke.type %sx  "window" ke.windowID %sx   "state" ke.state %sx
  "repeat" ke.repeat %sx
  "scancode" ke.keysym.scancode %sx
  "sym" ke.keysym.sym %sx
  "mod" ke.keysym.mod %sx
  %nl
;

: showButtonEvent
  ptrTo SDL_MouseButtonEvent mbe!
  "type" mbe.type %sx "which" mbe.which %sx "button" mbe.button "state" mbe.state %sx
  "x" mbe.x %sx "y" mbe.y %sx
  %nl
;

: showMotionEvent
  ptrTo SDL_MouseMotionEvent mme!
  "type" mme.type %sx "which" mme.which %sx "state" mme.state %sx
  "x" mme.x %sx "y" mme.y %sx
  "xrel" mme.xrel %sx "yrel" mme.yrel %sx
  %nl
;

: testEvents
  SDL_Event ev
  startSDL

  false int buttonDown!
  sc( -1 )		// draw in white
  
  do( 800 0 )
    SDL_WaitEvent( ev ) drop
    //ds
    case( ev.common.type )
      of( SDL_KEYDOWN )
        "key down " %s showKeyEvent( ev.key )
      endof
      
      of( SDL_KEYUP )
        "key up " %s showKeyEvent( ev.key )
      endof
      
      of( SDL_MOUSEMOTION )			// Mouse moved
        "mouse motion " %s showMotionEvent( ev.motion )
        if( buttonDown )
          ev.motion.x screenInstance.width 2/ + screenInstance.height 2/ ev.motion.y - dp
        endif
      endof
      
      of( SDL_MOUSEBUTTONDOWN )		// Mouse button pressed
        "mouse down " %s showButtonEvent( ev.button )
        dp( ev.button.x screenInstance.width 2/ + screenInstance.height 2/ ev.button.y - )
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

