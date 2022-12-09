autoforget sdl_events

requires sdl2
also sdl2
sdl2 definitions

: sdl_events ;

// Include file for SDL event handling


// General keyboard/mouse state definitions
0 constant SDL_RELEASED
1 constant SDL_PRESSED

// Event enumerations
enum: SDL_EventType
       SDL_NOEVENT			// Unused (do not remove)
       SDL_ACTIVEEVENT			// Application loses/gains visibility
       SDL_KEYDOWN			// Keys pressed
       SDL_KEYUP			// Keys released
       SDL_MOUSEMOTION			// Mouse moved
       SDL_MOUSEBUTTONDOWN		// Mouse button pressed
       SDL_MOUSEBUTTONUP		// Mouse button released
       SDL_JOYAXISMOTION		// Joystick axis motion
       SDL_JOYBALLMOTION		// Joystick trackball motion
       SDL_JOYHATMOTION		// Joystick hat position change
       SDL_JOYBUTTONDOWN		// Joystick button pressed
       SDL_JOYBUTTONUP			// Joystick button released
       SDL_QUIT			// User-requested quit
       SDL_SYSWMEVENT			// System specific event
       SDL_EVENT_RESERVEDA		// Reserved for future use..
       SDL_EVENT_RESERVEDB		// Reserved for future use..
       SDL_VIDEORESIZE			// User resized video mode
       SDL_VIDEOEXPOSE			// Screen needs to be redrawn
       SDL_EVENT_RESERVED2		// Reserved for future use..
       SDL_EVENT_RESERVED3		// Reserved for future use..
       SDL_EVENT_RESERVED4		// Reserved for future use..
       SDL_EVENT_RESERVED5		// Reserved for future use..
       SDL_EVENT_RESERVED6		// Reserved for future use..
       SDL_EVENT_RESERVED7		// Reserved for future use..
       // Events SDL_USEREVENT through SDL_MAXEVENTS-1 are for your use
       24 SDL_USEREVENT
       // This last event is only for bounding internal arrays
 	   // It is the number of bits in the event mask datatype -- Uint32
       
       32 SDL_NUMEVENTS
;enum

// Predefined event masks
: SDL_EVENTMASK	1 swap lshift ;

enum:	SDL_EventMask
	SDL_EVENTMASK( SDL_ACTIVEEVENT )		SDL_ACTIVEEVENTMASK	
	SDL_EVENTMASK( SDL_KEYDOWN )			SDL_KEYDOWNMASK		
	SDL_EVENTMASK( SDL_KEYUP )				SDL_KEYUPMASK		
	SDL_KEYDOWNMASK SDL_KEYUPMASK or			SDL_KEYEVENTMASK
	SDL_EVENTMASK( SDL_MOUSEMOTION )		SDL_MOUSEMOTIONMASK	
	SDL_EVENTMASK( SDL_MOUSEBUTTONDOWN )	SDL_MOUSEBUTTONDOWNMASK	
	SDL_EVENTMASK( SDL_MOUSEBUTTONUP )		SDL_MOUSEBUTTONUPMASK
	
	SDL_MOUSEMOTIONMASK SDL_MOUSEBUTTONDOWNMASK or SDL_MOUSEBUTTONUPMASK or
											SDL_MOUSEEVENTMASK	
											
	SDL_EVENTMASK( SDL_JOYAXISMOTION )		SDL_JOYAXISMOTIONMASK	
	SDL_EVENTMASK( SDL_JOYBALLMOTION )		SDL_JOYBALLMOTIONMASK	
	SDL_EVENTMASK( SDL_JOYHATMOTION )		SDL_JOYHATMOTIONMASK	
	SDL_EVENTMASK( SDL_JOYBUTTONDOWN )		SDL_JOYBUTTONDOWNMASK	
	SDL_EVENTMASK( SDL_JOYBUTTONUP )		SDL_JOYBUTTONUPMASK	
	SDL_JOYAXISMOTIONMASK SDL_JOYBALLMOTIONMASK or SDL_JOYHATMOTIONMASK or
	SDL_JOYBUTTONDOWNMASK or SDL_JOYBUTTONUPMASK or 
											SDL_JOYEVENTMASK
	SDL_EVENTMASK( SDL_VIDEORESIZE )		SDL_VIDEORESIZEMASK	
	SDL_EVENTMASK( SDL_VIDEOEXPOSE )		SDL_VIDEOEXPOSEMASK	
	SDL_EVENTMASK( SDL_QUIT )				SDL_QUITMASK		
	SDL_EVENTMASK( SDL_SYSWMEVENT)			SDL_SYSWMEVENTMASK	
	0xFFFFFFFF SDL_ALLEVENTS
;enum

//  Keysym structure
//   - The scancode is hardware dependent, and should not be used by general
//     applications.  If no hardware scancode is available, it will be 0.
//
//   - The 'unicode' translated character is only available when character
//     translation is enabled by the SDL_EnableUNICODE() API.  If non-zero,
//     this is a UNICODE character corresponding to the keypress.  If the
//     high 9 bits of the character are 0, then this maps to the equivalent
//     ASCII character:
//	char ch;
//	if ( (keysym.unicode & 0xFF80) == 0 ) {
//		ch = keysym.unicode & 0x7F;
//	} else {
//		An international character..
//	}
	
#if 0
struct: SDL_keysym
	byte scancode			// hardware specific scancode
	short sym				// SDL virtual keysym 
	short mod				// current key modifiers
	short unicode			// translated character
;struct
#endif

// I'm not sure about sym and mod size, they are enums

// Application visibility event structure
struct: SDL_ActiveEvent
	byte type	// SDL_ACTIVEEVENT
	byte gain	// Whether given states were gained or lost (1/0)
	byte state	// A mask of the focus states
;struct

// Keyboard event structure
struct: SDL_KeyboardEvent
	byte type	// SDL_KEYDOWN or SDL_KEYUP
	byte which	// The keyboard device index
	byte state	// SDL_PRESSED or SDL_RELEASED
	// this next part was a SDL_keysym, but that caused 'sym' to be misaligned
	byte scancode			// hardware specific scancode
	short sym				// SDL virtual keysym 
	short mod				// current key modifiers
	short unicode			// translated character
;struct

// Mouse motion event structure
struct: SDL_MouseMotionEvent
	byte type	// SDL_MOUSEMOTION
	byte which	// The mouse device index
	byte state	// The current button state
	short x short y	// The X/Y coordinates of the mouse
	short xrel	// The relative motion in the X direction
	short yrel	// The relative motion in the Y direction
;struct

// Mouse button event structure
struct: SDL_MouseButtonEvent
	byte type	// SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP
	byte which	// The mouse device index
	byte button	// The mouse button index
	byte state	// SDL_PRESSED or SDL_RELEASED
	short x short y	// The X/Y coordinates of the mouse at press time
;struct

// Joystick axis motion event structure
struct: SDL_JoyAxisEvent
	byte type	// SDL_JOYAXISMOTION
	byte which	// The joystick device index
	byte axis	// The joystick axis index
	short value	// The axis value (range: -32768 to 32767)
;struct

// Joystick trackball motion event structure
struct: SDL_JoyBallEvent
	byte type	// SDL_JOYBALLMOTION
	byte which	// The joystick device index
	byte ball	// The joystick trackball index
	short xrel	// The relative motion in the X direction
	short yrel	// The relative motion in the Y direction
;struct

// Joystick hat position change event structure
struct: SDL_JoyHatEvent
	byte type	// SDL_JOYHATMOTION
	byte which	// The joystick device index
	byte hat	// The joystick hat index
	byte value	// The hat position value:
			    // SDL_HAT_LEFTUP   SDL_HAT_UP       SDL_HAT_RIGHTUP
			    // SDL_HAT_LEFT     SDL_HAT_CENTERED SDL_HAT_RIGHT
			    // SDL_HAT_LEFTDOWN SDL_HAT_DOWN     SDL_HAT_RIGHTDOWN
			   // Note that zero means the POV is centered.
;struct

// Joystick button event structure
struct: SDL_JoyButtonEvent
	byte type	// SDL_JOYBUTTONDOWN or SDL_JOYBUTTONUP
	byte which	// The joystick device index
	byte button	// The joystick button index
	byte state	// SDL_PRESSED or SDL_RELEASED
;struct

// The "window resized" event
//   When you get this event, you are responsible for setting a new video
//   mode with the new width and height.

struct: SDL_ResizeEvent
	byte type	// SDL_VIDEORESIZE
	int w		// New width
	int h		// New height
;struct

// The "screen redraw" event
struct: SDL_ExposeEvent
	byte type	// SDL_VIDEOEXPOSE
;struct

// The "quit requested" event
struct: SDL_QuitEvent
	byte type	// SDL_QUIT
;struct

// A user-defined event type
struct: SDL_UserEvent
	byte type	// SDL_USEREVENT through SDL_NUMEVENTS-1
	int code	// User defined event code
	ptrTo byte data1	// User defined data pointer
	ptrTo byte data2	// User defined data pointer
;struct

// If you want to use this event, you should include SDL_syswm.h
struct: SDL_SysWMEvent
	byte type
	ptrTo byte msg		// should be SDL_SysWMmsg *msg
;struct

// General event structure
struct: SDL_Event
	byte type
	union SDL_ActiveEvent active
	union SDL_KeyboardEvent key
	union SDL_MouseMotionEvent motion
	union SDL_MouseButtonEvent button
	union SDL_JoyAxisEvent jaxis
	union SDL_JoyBallEvent jball
	union SDL_JoyHatEvent jhat
	union SDL_JoyButtonEvent jbutton
	union SDL_ResizeEvent resize
	union SDL_ExposeEvent expose
	union SDL_QuitEvent quit
	union SDL_UserEvent user
	union SDL_SysWMEvent syswm
;struct


// Function prototypes

// Pumps the event loop, gathering events from the input devices.
// This function updates the event queue and internal input device state.
// This should only be run in the thread that sets the video mode.

DLLVoid dll_0 SDL_PumpEvents	// extern DECLSPEC void SDLCALL SDL_PumpEvents(void)

// Checks the event queue for messages and optionally returns them.
// If 'action' is SDL_ADDEVENT, up to 'numevents' events will be added to
// the back of the event queue.
// If 'action' is SDL_PEEKEVENT, up to 'numevents' events at the front
// of the event queue, matching 'mask', will be returned and will not
// be removed from the queue.
// If 'action' is SDL_GETEVENT, up to 'numevents' events at the front 
// of the event queue, matching 'mask', will be returned and will be
// removed from the queue.
// This function returns the number of events actually stored, or -1
// if there was an error.  This function is thread-safe.

enum: SDL_eventaction
	SDL_ADDEVENT
	SDL_PEEKEVENT
	SDL_GETEVENT
;enum 

//
dll_4 SDL_PeepEvents		// SDL_Event *events, int numevents, SDL_eventaction action, Uint32 mask

// Polls for currently pending events, and returns 1 if there are any pending
// events, or 0 if there are none available.  If 'event' is not NULL, the next
// event is removed from the queue and stored in that area.

dll_1 SDL_PollEvent		// (SDL_Event *event)

// Waits indefinitely for the next available event, returning 1, or 0 if there
// was an error while waiting for events.  If 'event' is not NULL, the next
// event is removed from the queue and stored in that area.

dll_1 SDL_WaitEvent		// (SDL_Event *event)

// Add an event to the event queue.
// This function returns 0 on success, or -1 if the event queue was full
// or there was some other error.

dll_1 SDL_PushEvent		// (SDL_Event *event)

//
//This function sets up a filter to process all events before they
//change internal state and are posted to the internal event queue.
// The filter is protypted as:

//typedef int (SDLCALL *SDL_EventFilter)(const SDL_Event *event)

//If the filter returns 1, then the event will be added to the internal queue.
//If it returns 0, then the event will be dropped from the queue, but the 
//internal state will still be updated.  This allows selective filtering of
//dynamically arriving events.

// WARNING:  Be very careful of what you do in the event filter function, as 
//          it may run in a different thread!
// There is one caveat when dealing with the SDL_QUITEVENT event type.  The
// event filter is only called when the window manager desires to close the
// application window.  If the event filter returns 1, then the window will
// be closed, otherwise the window will remain open if possible.
// If the quit event is generated by an interrupt signal, it will bypass the
// internal queue and be delivered to the application at the next event poll.

//extern DECLSPEC void SDLCALL SDL_SetEventFilter(SDL_EventFilter filter)

//  Return the current event filter - can be used to "chain" filters.
//  If there is no event filter set, this function returns NULL.
//extern DECLSPEC SDL_EventFilter SDLCALL SDL_GetEventFilter(void)

//This function allows you to set the state of processing certain events.
//If 'state' is set to SDL_IGNORE, that event will be automatically dropped
//from the event queue and will not event be filtered.
//If 'state' is set to SDL_ENABLE, that event will be processed normally.
//If 'state' is set to SDL_QUERY, SDL_EventState() will return the 
//current processing state of the specified event.

-1 constant SDL_QUERY
0 constant SDL_IGNORE
0 constant SDL_DISABLE
1 constant SDL_ENABLE
dll_2 SDL_EventState		// (byte type, int state)

previous definitions

