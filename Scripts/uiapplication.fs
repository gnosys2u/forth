
autoforget ui_application   : ui_application ;

requires sdlscreen
requires uielement
requires eventqueue

also sdl2
		

class: UIApplication extends UIGroup
  UIElement mTextFocus
  String mAppName
  int mNextElementID
  bool mQuit

  m: addChild
    -> int y
    -> int x
    -> UIElement newChild
    mNextElementID -> newChild.mElementID
    1 ->+ mNextElementID
    super.addChild(newChild x y)
    oclear newChild
  ;m
  
  m: init
    -> h
    -> w
    new String -> mAppName
    mAppName.set
    0 -> yOff
    0 -> xOff
    1 -> mNextElementID
    
    this -> mTextFocus
    false -> mQuit
  ;m

  m: delete
    //"UIApplication delete\n" %s
    oclear mTextFocus
    oclear mAppName
    super.delete
  ;m
  
  m: setTextFocus
    -> mTextFocus
  ;m
  
  m: onEvent
    //"UIApplication:onEvent " %s this %x %bl %x %nl
    ->o Event ev
    if(ev.param1 `quit` =)
      true -> mQuit
    endif
  ;m
  
  m: runLoop
    SDL_Event sdlEvent

    begin
      SDL_WaitEvent( sdlEvent ) drop
#if 0
      "runLoop got event of type " %s
      if(findEnumSymbol(sdlEvent.common.type ref SDL_EventType))
        %s %nl
      else
        "0x" %s sdlEvent.common.type %x %nl
      endif
#endif

      case( sdlEvent.common.type )
        of( SDL_KEYDOWN )
          if(objNotNull(mTextFocus))
            mTextFocus.onKeyDown(sdlEvent)
          endif
        endof
      
        of( SDL_KEYUP )
          if(objNotNull(mTextFocus))
            mTextFocus.onKeyUp(sdlEvent)
          endif
        endof
      
        dispatchEvent(sdlEvent)
      
      endcase
      
    until(mQuit)

  ;m
  
  m: start
    startSDLWithSize(w h)
    screenInstance.setOriginMode(kScreenOriginTopLeft)
    screenInstance.setWindowTitle(mAppName.get)
  ;m
  
  m: finish
    endSDL
  ;m
  
;class

loaddone

