autoforget showMidi



\ 0 setTrace



\ enableMidi( BOOL )
\ disableMidi( BOOL )

\ midiInGetNumDevices ... NUM_INPUT_DEVICES
\ midiInGetDeviceName( DEVICE_NUM ) ... NAME_STR_PTR
\ midiInGetDeviceCapabilities( DEVICE_NUM ) ... IN_DEVICE_CAPABILITIES_PTR
\ midiInOpen( DEVICE_NUM CALLBACK_OPCODE CALLBACK_DATA ) ... MMSYSERR
\ midiInClose( DEVICE_NUM ) ... MMSYSERR
\ midiInStart( DEVICE_NUM ) ... MMSYSERR
\ midiInStop( DEVICE_NUM ) ... MMSYSERR
\ midiInGetErrorText( MMSYSERR BUFFER_PTR BUFFER_SIZE )

\ midiOutGetNumDevices ... NUM_OUTPUT_DEVICES
\ midiOutGetDeviceName( DEVICE_NUM ) ... NAME_STR_PTR
\ midiOutGetDeviceCapabilities( DEVICE_NUM ) ... OUT_DEVICE_CAPABILITIES_PTR
\ midiOutOpen( DEVICE_NUM CALLBACK_OPCODE CALLBACK_DATA ) ... MMSYSERR
\ midiOutClose( DEVICE_NUM ) ... MMSYSERR
\ midiOutShortMsg( DEVICE_NUM LONG ) ... MMSYSERR
\ midiOutGetErrorText( MMSYSERR BUFFER_PTR BUFFER_SIZE )
\ midiOutPrepareHeader( DEVICE_NUM MIDIHDR_PTR ) ... MMSYSERR
\ midiOutUnprepareHeader( DEVICE_NUM MIDIHDR_PTR ) ... MMSYSERR
\ midiOutLongMsg( DEVICE_NUM MIDIHDR_PTR ) ... MMSYSERR

\ MIDIHDR_SIZE ... <sizeof midiHdr>

\ MIDI_CALLBACK( WMSG CALLBACK_DATA PARAM1 PARAM2 )

enum: eMidiStatus
  $80	kMSNoteOff
  $90	kMSNoteOn
  $A0	kMSPolyPressure
  $B0	kMSControlChange
  $C0	kMSProgramChange
  $D0	kMSAftertouch
  $E0	kMSPitchBend
  $F0	kMSSysex
  $F2	kMSSysexPositionSelect
  $F3	kMSSysexProgramSelect
  
  $F6	kMSSysexTuneRequest
  $F7	kMSSysexEnd
  $F8	kMSSysexTimingClock

  $FA	kMSSysexStart
  $FB	kMSSysexContinue
  $FC	kMSSysexStop

  $FE	kMSSysexActiveSensing
  $FF	kMSSysexSystemReset
  
  $F0	kMSStatusMask
  $0F	kMSChannelMask
;enum

\ status returns from midi commands
enum: MMSYSERR
  MMSYSERR_NOERROR
  MMSYSERR_ERROR        \ unspecified error
  MMSYSERR_BADDEVICEID  \ device ID out of range
  MMSYSERR_NOTENABLED   \ driver failed enable
  MMSYSERR_ALLOCATED    \ device already allocated
  MMSYSERR_INVALHANDLE  \ device handle is invalid
  MMSYSERR_NODRIVER     \ no device driver present
  MMSYSERR_NOMEM        \ memory allocation error
  MMSYSERR_NOTSUPPORTED \ function isn't supported
  MMSYSERR_BADERRNUM    \ error value out of range
  MMSYSERR_INVALFLAG    \ invalid flag passed
  MMSYSERR_INVALPARAM   \ invalid parameter passed
  MMSYSERR_HANDLEBUSY   \ handle being used simultaneously on another thread (eg callback)
  MMSYSERR_INVALIDALIAS \ specified alias not found
  MMSYSERR_BADDB        \ bad registry database
  MMSYSERR_KEYNOTFOUND  \ registry key not found
  MMSYSERR_READERROR    \ registry read error
  MMSYSERR_WRITEERROR   \ registry write error
  MMSYSERR_DELETEERROR  \ registry delete error
  MMSYSERR_VALNOTFOUND  \ registry value not found
  MMSYSERR_NODRIVERCB   \ driver does not call DriverCallback
  MMSYSERR_MOREDATA     \ more data to be returned
  MMSYSERR_LASTERROR    \ last error in range
;enum

\ midi callback codes
enum: midiCallbackCodes
  \ midi input
  $3C1 MM_MIM_OPEN
  MM_MIM_CLOSE
  MM_MIM_DATA
  MM_MIM_LONGDATA
  MM_MIM_ERROR
  MM_MIM_LONGERROR

  \ midi output
  MM_MOM_OPEN
  MM_MOM_CLOSE
  MM_MOM_DONE
;enum

struct: midiHdr
  ptrTo ubyte 		lpData     			\ pointer to locked data block
  uint 				dwBufferLength    	\ length of data in data block
  uint    			dwBytesRecorded     \ used for input only
  cell				dwUser              \ for client's use
  uint       		dwFlags             \ assorted flags (see defines)
  cell				lpNext   			\ reserved for driver
  cell				reserved            \ reserved for driver
  uint				dwOffset            \ Callback offset into buffer
  8 arrayOf cell	dwReserved        	\ Reserved for MMSYSTEM
;struct

: showMidi
  midiInGetNumDevices -> int nIns
  midiOutGetNumDevices -> int nOuts
  "============ Midi Inputs ============" %s %nl
  nIns 0 do
    i %d "    " %s i midiInGetDeviceName %s %nl
  loop
  "============ Midi Outputs ============" %s %nl
  nOuts 0 do
    i %d "    " %s i midiOutGetDeviceName %s %nl
  loop
;

-1 -> cell midiInDev
0 -> cell midiOutDev
0 -> int midiOutChannel
96 -> int midiOutVelocity

cell sp1
cell sp2
-1 -> cell scbd
cell lp1
cell lp2
-1 -> cell lcbd
-1 -> cell msg

cell boo

: echoShortMessage
  if( midiOutDev 0>= )
    "echo $" %s dup %x %nl
    midiOutShortMsg( midiOutDev swap )
  else
    drop
  endif
;

: midiInCallback
  -> cell dwParam2
  -> cell dwParam1
  -> cell cbData
  -> cell wMsg
  
#if 0
  "MIDI_IN   wMsg $" %s wMsg %x
  "  cbData $" %s cbData %x
  "  dwParam1 $" %s dwParam1 %x
  "  dwParam2 $" %s dwParam2 %x
  %nl
#endif

  wMsg -> msg
  
  case( wMsg )
    MM_MIM_OPEN of
    endof
  
    MM_MIM_CLOSE of
    endof
  
    MM_MIM_DATA of
      cbData -> scbd
      dwParam1 -> sp1
      dwParam2 -> sp2
      \ dwParam1 is the midi message, lowest 8 bits is status byte
      case( dwParam1 kMSStatusMask and )
      
	    kMSNoteOff of
	      if( midiOutDev 0>= )
	        'a' %c echoShortMessage( dwParam1 )
	      endif
	    endof
	    
	    kMSNoteOn of
	      if( midiOutDev 0>= )
	        'b' %c echoShortMessage( dwParam1 )
	      endif
	    endof
	    
	    kMSPolyPressure of
	    endof
	    
	    kMSControlChange of
	      if( midiOutDev 0>= )
	        'c' %c echoShortMessage( dwParam1 )
	      endif
	    endof
	    
	    kMSProgramChange of
	    endof
	    
	    kMSAftertouch of
	    endof
	    
	    kMSSysex of
	    endof
	    
	    \ default case
	    drop
	  endcase
    endof
  
    MM_MIM_LONGDATA of
      cbData -> lcbd
      dwParam1 -> lp1
      dwParam2 -> lp2
    endof
  
    MM_MIM_ERROR of
    endof
  
    MM_MIM_LONGERROR of
    endof
    
    \ default case
    drop
  endcase
;

: dd
  "short cbd " %s scbd %x " p1 " %s sp1 %x " p2 " %s sp2 %x %nl
  "long  cbd " %s lcbd %x " p1 " %s lp1 %x " p2 " %s lp2 %x %nl
;

: midiOutCallback
  -> cell dwParam2
  -> cell dwParam1
  -> cell cbData
  -> cell wMsg
  
#if 0
  "MIDI_OUT   wMsg $" %s wMsg %x
  "  cbData $" %s cbData %x
  "  dwParam1 $" %s dwParam1 %x
  "  dwParam2 $" %s dwParam2 %x
  "\nSTACK:  " %s
  ds
  %nl
#endif

  case( wMsg )
    MM_MOM_OPEN of
    endof
  
    MM_MOM_CLOSE of
    endof
  
    MM_MOM_DONE of
    endof
    
    \ default case
    drop
  endcase
;


: miMonitor
  -> midiInDev
   
  if( midiInDev midiInGetNumDevices < )
    enableMidi
    midiInOpen( midiInDev lit midiInCallback 55 )
    "midiInOpen result: $" %s %x %nl
  endif
;

: miMonitorOff
  if( midiInDev 0>= )
    midiInClose( midiInDev )
    disableMidi
    -1 -> midiInDev
  endif
;

\ miOutStart( OUT_DEVICE )
: miOutStart
  -> midiOutDev
  if( midiOutDev midiOutGetNumDevices < )
    enableMidi
    midiOutOpen( midiOutDev lit midiOutCallback 44 )
    "midiOutOpen result: $" %s %x %nl
  endif
;  

: miOutStop
  if( midiOutDev 0>= )
    midiOutClose( midiOutDev )
    disableMidi
    -1 -> midiOutDev
  endif
;

\ miInStart( IN_DEVICE )
: miInStart
  -> midiInDev
  if( midiInDev midiInGetNumDevices < )
    enableMidi
    midiInOpen( midiInDev lit midiInCallback 33 )
    "midiInOpen result: $" %s %x %nl
    midiInStart( midiInDev )
    "midiInStart result: $" %s %x %nl
  endif
;  

: miInStop
  if( midiInDev 0>= )
    midiInStop( midiInDev ) drop
    midiInClose( midiInDev ) drop
    -1 -> midiInDev
  endif
;

: non
  8 lshift midiOutVelocity 16 lshift or kMSNoteOn midiOutChannel or or
  midiOutShortMsg( midiOutDev swap )
;

: noff
  8 lshift kMSNoteOn midiOutChannel or or
  midiOutShortMsg( midiOutDev swap )
;

: progSel
  8 lshift kMSProgramChange midiOutChannel or or
  midiOutShortMsg( midiOutDev swap )
;

: start
  0 miOutStart 8 miInStart
;

: stop
  miInStop miOutStop
;


loaddone

demo   case
demo     0 of "zero" endof
demo     1 of "one" endof
demo     2 of "two" endof
demo     drop "whatever"
demo   endcase
