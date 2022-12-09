
autoforget ui_element       : ui_element ;

also sdl2

#if(0)
: %sx swap %bl %bl %s "=0x" %s %x ;

: showKeyEvent
  -> ptrTo SDL_KeyboardEvent ke
  "Key type" ke.type %sx  "state" ke.state %sx
  "scancode" ke.keysym.scancode %sx
  "sym" ke.keysym.sym %sx %bl ke.keysym.sym %c %bl
  "mod" ke.keysym.mod %sx
  %nl
;

: showButtonEvent
  -> ptrTo SDL_MouseButtonEvent mbe
  "Button type" mbe.type %sx "which" mbe.which %sx "button" mbe.button "state" mbe.state %sx
  " x " %s mbe.x %d " y " %s mbe.y %d
  %nl
;

: showMotionEvent
  -> ptrTo SDL_MouseMotionEvent mme
  "Motion type" mme.type %sx "which" mme.which %sx "state" mme.state %sx
  " x " %s mme.x %d " y " %s mme.y %d
  " xrel " %s mme.xrel %d " yrel " %s mme.yrel %d
  %nl
;
#endif

class: UIElement
  UIElement mParent
  int mElementID
  int xOff      // offset within parent
  int yOff
  int xScreenLeft   // position within main window
  int yScreenTop
  int xScreenRight
  int yScreenBottom
  int w
  int h
  int backgroundColor
  int flags

  m: delete
    //"UIElement delete\n" %s
    oclear mParent
  ;m
  
  m: setParent
    -> mParent
    if(objNotNull(mParent))
      mParent.xScreenLeft xOff + -> xScreenLeft
      mParent.yScreenTop yOff + -> yScreenTop
      xScreenLeft w + -> xScreenRight
      yScreenTop h + -> yScreenBottom
    endif
  ;m
  
  m: shutdown
    oclear mParent
  ;m
  
  m: onEvent
    drop
  ;m
  
  m: sendToParent
    if(objNotNull(mParent))
      mParent.onEvent
    else
      drop
    endif
  ;m
  
  m: onTouchStart   drop  ;m
  m: onTouchEnd     drop  ;m
  m: onTouchMove    drop  ;m
  m: onKeyDown      drop  ;m
  m: onKeyUp        drop  ;m
  
  m: draw
    //"UIElement:draw @" %s xScreenLeft %d %bl yScreenTop %d %bl w %d "x" %s h %d %nl
    screenInstance.beginFrame
    screenInstance.moveTo(xScreenLeft yScreenTop)
    screenInstance.setColor(backgroundColor)
    screenInstance.fillRectangle(w h)
    screenInstance.endFrame
  ;m
  
;class
  
class: UIGroup extends UIElement
  Array of UIElement mChildren

  m: delete
    //"UIGroup delete\n" %s
    oclear mChildren
    super.delete
  ;m
  
  m: addChild
    if(objIsNull(mChildren))
      new Array -> mChildren
    endif
    
    -> int y
    -> int x
    ->o UIElement child
    x -> child.xOff
    y -> child.yOff
    child.setParent(this)
    mChildren.push(child)
  ;m
  
  m: shutdown
    ArrayIter iter
    if(objNotNull(mChildren))
      mChildren.headIter -> iter
      begin
      while(iter.next)
        <UIElement>.shutdown
      repeat
      oclear iter
    endif
  ;m
  
  m: draw
    //"UIGroup:draw @" %s xScreenLeft %d %bl yScreenTop %d %bl w %d "x" %s h %d %nl
    ArrayIter iter
    if(objNotNull(mChildren))
      mChildren.headIter -> iter
      begin
      while(iter.next)
        <UIElement>.draw
      repeat
      oclear iter
    endif
  ;m
  
  m: dispatchEvent
    -> ptrTo SDL_Event sdlEvent
    ArrayIter iter
    int x   int y
    
    case( sdlEvent.common.type )
      of( SDL_MOUSEMOTION )			// Mouse moved
        mChildren.headIter -> iter
        sdlEvent.motion.x -> x
        sdlEvent.motion.y -> y
        begin
        while(iter.next)
          -> UIElement child
          if(within(x child.xScreenLeft child.xScreenRight))
          andif(within(y child.yScreenTop child.yScreenBottom))
            child.onTouchMove(sdlEvent)
            iter.seekTail
          endif
        repeat
        oclear iter
      endof
      
      of( SDL_MOUSEBUTTONDOWN )			// Mouse button pressed
        mChildren.headIter -> iter
        sdlEvent.button.x -> x
        sdlEvent.button.y -> y
        begin
        while(iter.next)
          -> UIElement child
          if(within(x child.xScreenLeft child.xScreenRight))
          andif(within(y child.yScreenTop child.yScreenBottom))
            child.onTouchStart(sdlEvent)
            iter.seekTail
          endif
        repeat
        oclear iter
      endof
      
      of( SDL_MOUSEBUTTONUP )			// Mouse button released
        mChildren.headIter -> iter
        sdlEvent.button.x -> x
        sdlEvent.button.y -> y
        begin
        while(iter.next)
          -> UIElement child
          if(within(x child.xScreenLeft child.xScreenRight))
          andif(within(y child.yScreenTop child.yScreenBottom))
            child.onTouchEnd(sdlEvent)
            iter.seekTail
          endif
        repeat
        oclear iter
      endof
      
    endcase
  ;m
  
  m: onTouchStart
    dispatchEvent
  ;m
  
  m: onTouchEnd
    dispatchEvent
  ;m
  
  m: onTouchMove
    dispatchEvent
  ;m
  

;class


class: Button extends UIElement
  String text
  op onButtonDown
  op onButtonUp
  op onButtonMove
  int borderColor
  
  m: init
    -> h
    -> w
    new String -> text
    text.set
  ;m
  
  m: delete
    oclear text
  ;m
  
  m: draw
    screenInstance.beginFrame
    super.draw
    //"Button:draw @" %s xScreenLeft %d %bl yScreenTop %d %bl w %d "x" %s h %d %nl
    screenInstance.moveTo(xScreenLeft yScreenTop)
    screenInstance.setColor(borderColor)
    screenInstance.drawRectangle(w h)
    screenInstance.drawText(text.get)
    screenInstance.endFrame
  ;m

  m: onTouchStart
    -> ptrTo SDL_Event sdlEvent
    if(fetch onButtonDown)
      onButtonDown(this sdlEvent.button.x sdlEvent.button.y)
    endif
  ;m
  
  m: onTouchEnd
    -> ptrTo SDL_Event sdlEvent
    if(fetch onButtonUp)
      onButtonUp(this sdlEvent.button.x sdlEvent.button.y)
    endif
  ;m
  
  m: onTouchMove
    -> ptrTo SDL_Event sdlEvent
    if(fetch onButtonMove)
      onButtonMove(this sdlEvent.button.x sdlEvent.button.y)
    endif
  ;m
  
;class

previous

loaddone

 

// simple buttons
// toggle buttons
// radio buttons
// sliders
// edit boxes
