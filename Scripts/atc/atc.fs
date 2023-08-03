
autoforget ATC
: ATC ;

FileOutStream traceFileStream
: openTrace
  new FileOutStream traceFileStream!
  traceFileStream.open( "traceOut.txt" "w")
;

: closeTrace
  traceFileStream.close
  oclear traceFileStream
;

: tf{ setConsoleOut( traceFileStream ) ;
: }tf setConsoleOut( getDefaultConsoleOut ) ;

enum: atcTileType  byte
  kATTEmpty
  kATTBoundary
  kATTBeacon
  kATTAirport
  kATTPortal
;enum

\  7 0 1    q w e    315   0    45
\  6   2    a   d    270        90
\  5 4 3    z x c    225  180  135

"wedcxzaq" 9 string directionChars!

enum: atcDirection  byte
  kADNorth
  kADNorthEast
  kADEast
  kADSouthEast
  kADSouth
  kADSouthWest
  kADWest
  kADNorthWest
  kNumDirections
;enum

enum: atcAirplaneStatus byte
  kAASHolding
  kAASMovingMarked
  kAASMovingUnmarked
  kAASMovingIgnored
;enum

9000 constant kMaxAltitude

enum: atcCommandType  \ uses command info:
  kACTAltitude        \ isRelative amount
  kACTMark
  kACTIgnore
  kACTUnmark
  kACTCircle          \ forceDirection beaconNum
  kACTTurn            \ forceDirection isRelative amount beaconNum
  kACTTurnTowards     \ targetType target beaconNum
  kNumCommands
;enum

struct: atcCommandInfo      \ used by:
  int airplaneNum
  atcCommandType command    \ all
  int isRelative            \ altitude turn
  int amount                \ altitude turn
  int forceDirection        \ circle turn (1 for clockwise/right, 0 for closest direction, -1 for counterclockwise/left)
  int targetNum             \ turnTowards
  atcTileType targetType    \ turnTowards
  int beaconNum             \ circle turn turnTowards
  int isReady
;struct

enum: atcCommandState
  kACSIdle                  \ valid input is plane identifier in a..z
  kACSPlaneSelected
  kACSCommandReady          \ return is only valid input
  kACSAltitude              \ *a - valid is cd# - climb, descend or altitude
  kACSAltitudeClimb         \ *ac - valid is #
  kACSAltitudeDescend       \ *ad - valid is #
  kACSCircle                \ *c - valid is lr
  kACSCircleReady
  kACSTurn
  kACSTurnDirection
  kACSTurnTowards
  kACSTurnTowardsAirport
  kACSTurnTowardsBeacon
  kACSTurnTowardsExit
  kACSTurnReady
  kACSAt
  kACSAtBeacon
  kNumCommandStates
;enum

struct: atcStatus
  int score
  int gameStartTime         \ system time at game start in milliseconds
  int nextMoveTime          \ system time in milliseconds
  int moves                 \ game time in moves
  int nextPlaneMove
  int moveInterval          \ time between moves in milliseconds
  int keepPlaying
;struct

\ ======== atcTile ========

class: atcTile
  int tag
  
  byte altitude
  byte index                \ index of airport, portal or beacon
  atcTileType tileType      \ airport, portal, boundary, beacon
  atcDirection direction    \ for airports, currently unused

  List airplanes

  m: addAirplane       \ airplane ...
    airplanes.addTail
  ;m

  m: removeAirplane    \ airplane ...
    \ TODO
    airplanes.remove
  ;m
  
  m: init
    'tile' tag!
    altitude~
    kATTEmpty tileType!
    new List airplanes!
  ;m
    
  m: reset
    airplanes.clear
  ;m
    
  m: delete
    tf{ "deleting tile\n" %s }tf
    oclear airplanes
    super.delete
  ;m
    
;class


class: iAtcDisplay
  int tag
  ByteArray text
  ptrTo byte textBase
  
  m: init                  ;m \ REGION ...
  m: update                ;m \ ACTIVE_AIRPLANE_LIST ...
  m: showWarning           ;m \ CSTRING ...
  m: startWarning          ;m
  m: showCommand           ;m \ CSTRING ...
  m: restorePos            ;m \ X Y ...
  m: hideAirplanes         ;m \ ACTIVE_AIRPLANE_LIST ...
  m: useDefaultColors      ;m
  m: useHighlightColors    ;m
  
;class

class: iAtcGame
  int tag
  Array airplanes
  List activeAirplanes
  List inactiveAirplanes
  iAtcDisplay display
  atcStatus status
  String name
  
  m: init                ;m   \ GAME_NAME
  m: reset               ;m
  m: activateAirplane    ;m   \ PLANE_OBJ ...
  m: deactivateAirplane  ;m   \ PLANE_OBJ ...
  m: moveAirplane        ;m   \ newPlaneX newPlaneY plane ...
  m: play                ;m
;class

iAtcGame game

\ ======== interfaces ========

class: iAtcAirplane
  int tag
  int fuel
  String name
  int id
  int altitude
  int commandedAltitude
  int x
  int y
  int destination                 \ index of destination airport or portal
  atcAirplaneStatus status        \ holding for takeoff, marked/unmarked/ignored
  atcTileType destinationType     \ kATTPortal or kATTAirport
  atcDirection heading
  atcDirection commandedHeading
  int beaconNum             \ -1 for no delaying beacon
  int circle                \ 0 for not circling
  int forceDirection
  
  m: init            ;m \ ID REGION ...
  m: setPosition     ;m \ X Y ALTITUDE HEADING
  m: setDestination  ;m \ DESTINATION DESTINATION_TYPE ...
  m: activate        ;m \ UPDATE_TIME IS_JET IS_MOVING ...

  m: update          ;m \ NOW ...
  m: executeCommand  ;m \ COMMAND_INFO_PTR ...
  m: getPos  x y     ;m \ ... X Y
;class

class: iAtcAirport
  int tag
  int id
  int altitude
  int x
  int y
  atcDirection entryDirection

  m: init            ;m \ X Y ENTRY_DIRECTION ID REGION
  m: updateMove      ;m
  m: updateStatus    ;m
;class

class: iAtcPortal
  int tag
  int id
  int x
  int y
  int exitAltitude
  int entryAltitude
  int lastEntryTime
  atcDirection entryDirection
  
  m: init            ;m \ X Y ENTRY_DIRECTION ID REGION
  m: update          ;m
;class

class: iAtcBeacon
  int tag
  int x
  int y
  int id
  
  m: init  \ X Y ID
    id!    y!    x!
    'becn' tag!
  ;m
  
  m: at returns int  \ X Y BOOL
    and( y = swap x = )
  ;m
  
;class

class: iAtcLine
  int tag
  int x0
  int y0
  int x1
  int y1
  
  m: init        \ x0 y0 x1 y1
    y1!  x1!  y0!  x0!
    'line' tag!
  ;m

;class

class: iAtcRegion
  int tag
  int rows
  int columns
  int startingFuel
  int warningFuel
  
  Array tiles
  Array airports
  Array portals
  Array beacons
  Array lines
  
  m: init                                ;m \ width height game
  m: reset                               ;m
  m: getTile returns atcTile             ;m \ x y ... atcTile@x,y
  m: setTile                             ;m \ atcTile x y ...  
  m: update                              ;m
  m: getBeacon returns iAtcBeacon        ;m \ BEACON_NUM ... BEACON_OBJ
  m: getAirport returns iAtcAirport      ;m \ AIRPORT_NUM ... AIRPORT_OBJ
  m: getPortal returns iAtcPortal        ;m \ PORTAL_NUM ... PORTAL_OBJ
  m: getAirport@ returns iAtcAirport     ;m \ X Y ... AIRPORT_OBJ
  m: getPortal@ returns iAtcPortal       ;m \ X Y ... PORTAL_OBJ  
  m: getBeacon@ returns iAtcBeacon       ;m \ X Y ... BEACON_OBJ  
  m: checkCollisions                     ;m \ PLANE ... COLLIDED_PLANE_ID  returns -1 if no collision
  m: addPortal                           ;m \ X Y ENTRY_ALTITUDE EXIT_ALTITUDE ...
  m: addAirport                          ;m \ X Y ALTITUDE ...
  m: addBeacon                           ;m \ X Y ...
  m: addLine                             ;m \ X0 Y0 X1 Y1 ...
;class

class: iAtcInputHandler
  int tag
  iAtcDisplay display
  iAtcRegion region
  String displayString
  String commandString
  atcCommandInfo commandInfo
  atcCommandState commandState
  int isGameOver
  
  m: init                              ;m \ REGION DISPLAY_OBJ ...
  m: update                            ;m \ ...
  m: getString returns String         ;m \ ... STRING_OBJ
  m: getCommandString returns String  ;m \ ... STRING_OBJ

;class

\ debugging aid
: sh setConsoleCursor(90 0) "                                    " %s setConsoleCursor(90 0) %s ;
  
lf atcPortal

lf atcConsoleDisplay

lf atcAirplane

lf atcAirport

lf atcRegion

lf atcInputHandler

lf atcGame

lf atcFileReader

: listATCGames
  getFilesInDirectory("games") -> Array gameFilenames
  
  gameFilenames.headIter Iter iter!
  begin
  while( iter.next )
    String gameFilename!
    "   " %s gameFilename.get %s %nl
    gameFilename~
  repeat
  iter~
  
  gameFilenames~
;

: atc
  openTrace
  getConsoleColor int color!
  new atcGame game!
  game.init( blword )
#if FORTH64
  srand(xor(time dup 32 rshift swap MAXUINT and))
#else
  srand(xor(time))
#endif
  if(atcFileReader:loadAtcFile( game.name.get game))
    clearConsole
    \ tf{ game.show "\n\n\n=====================================\n\n\n" %s }tf
    game.play
    \ tf{ game.show }tf
    game.reset
  else
    "Available games:" %s %nl
    listATCGames
  endif
  \ tf{ game.show }tf
  game~
  setConsoleColor( color )
  closeTrace
;

"Type:\n\+
  atc            to play an atc game with the default map\n\+
  listATCGames   to see a list of atc maps\n\+
  atc MAPNAME    to play an atc game with specified map\n" %s
  
loaddone

: listGames
  int dirHandle
  dirent entry
  ptrTo byte pName
  
  opendir("games") -> dirHandle
  if( dirHandle )
    begin
    while( readdir(dirHandle entry) )
      entry.d_name(0 ref) -> pName \ ref pName !
      if(pName c@ '.' <>)
        "   " %s pName %s %nl
      endif
    repeat
    closedir(dirHandle) drop
  endif
;


+ handle backspace
+ only jets are being created
+ when number of planes decreases, there are stale lines at bottom of status display
+ circle commmand does nothing
+ add delayable commands
+ display commands in plane status
+ implement unmark
+ implement ignore
+ check for airplane collisions
+ fuel consumption isn't tracked

- implement turn towards
- invalid airport/beacon/portal number isn't reported in commands
- having an 'if' without an ending 'endif' at the top level of a method
  did not trigger an error - change <m: ;m> pair to do this checking



7-----------------------0---------------------------------1 Time: 667  Safe: 56
| + . . . . . . . . . . + . . . . . . . . . . . . . . . + |
| . + . . . . . . . . . + . . . . . . . . . . . . . . + . | pl dt  comm
| . . + . . . . . . . . + . . . . . . . . . . . . . + . . | e9 E0: ---------
| . . . + . g9. . . . . + . . . . . . . . . . . . + . . . | f9 E7:
| . . . . f9. . . . . . e9. . . . . . . . . . . + . . . . | g9 E0: Circle
| . . . . . + . . . . . + . . . . . . . . . . + . . . . . |
6 + + + + + + + + + + + *0+ + + + + + + + + + + + + + + + 2 h0 A1: Holding @ A0
| . . . . . . . . . . . + . . . . . . . . + . . . . . . . |
| . . . . . . . . . . . + . . . . . . . + . . . . . . . . |
| . . . . . . . . . . . + . . . . . . + . . . . . . . . . |
| . . . . . . . . . . . + . . . . . + . . . . . . . . . . |
| . . . . . . . . . . . + . . . . + . . . . . . . . . . . |
5 + + + + + + + + + + + + . . . + . . . . . . . . . . . . |
| . . . . . . . . . . . + . . + . . . . . . . . . . . . . |
| . . . . . . . . . . . + . + . . . . . ^0. . . . . . . . |
| . . . . . . . . . . . + + . . . . . . . . . . . . . . . |
| . . . . . . . . . . . *1+ + + + + + + + + + + + + + + + 3
| . . . . . . . . . . + . . . . . . . . >1. . . . . . . . |
| . . . . . . . . . + . . . . . . . . . . . . . . . . . . |
------------------4----------------------------------------

Plane 'f' collided with plane 'g'.                          *******************
                                                             ATC - by Ed James
Hit space for top players list...                           *******************
     # This is the default game.

     update = 5;
     newplane = 5;
     width = 30;
     height = 21;

     exit:           ( 12  0 x ) ( 29  0 z ) ( 29  7 a ) ( 29 17 a )
                     (  9 20 e ) (  0 13 d ) (  0  7 d ) (  0  0 c ) ;

     beacon:         ( 12  7 ) ( 12 17 ) ;

     airport:        ( 20 15 w ) ( 20 18 d ) ;

     line:           [ (  1  1 ) (  6  6 ) ]
                     [ ( 12  1 ) ( 12  6 ) ]
                     [ ( 13  7 ) ( 28  7 ) ]
                     [ ( 28  1 ) ( 13 16 ) ]
                     [ (  1 13 ) ( 11 13 ) ]
                     [ ( 12  8 ) ( 12 16 ) ]
                     [ ( 11 18 ) ( 10 19 ) ]
                     [ ( 13 17 ) ( 28 17 ) ]
                     [ (  1  7 ) ( 11  7 ) ] ;




------------0-----------------------1---------------------- Time: 285  Safe: 44
| . . . . . + . . . . . . . . . . . + . . . . . . . . . . |
| . . . . . + . . . . . . . . . . . + . . . . . . . . . . | pl dt  comm
| . . . . . + . . . . . . . . . . . + . . . . . . . . . . | b9 E3:
| . . . . . + . . . . . . . . . . . + . . . . . . . . . . | c9 E2:
| . . . . . *0+ + + + + x1+ + + + + *1+ + + + + + D7+ u9+ 2 D7 E4:
| . . . . . + . . . . . . . . . . . . . . . . . . . . . b9| E7 E3:
| . . . . . + . . . . . . . . . . . . . . . . . . . . + . | P9 E3: ---------
| . . . . . + . >0. . . . . . . . Z7. . . . . . . . + . . | u9 E2: ---------
| . . . . . c9. . . . . . . . . . . . . . . . . . + . . . | v8 E4:
| . . . . . + . . . . . . . . . . . . . . . . . + . . . . | x1 A0:
| . . . . . + . . . . . . . . . . . . . . . . + . . . . . | Z7 E0:
| . . . . . + . . . . . . . . . . . . . . . + . . . . . . |
| . . . . . *2+ + + + + + + + + P9+ + + + + + + + + + + + 3 A0 E0: Holding @ A0
| . . . . . + . . . . . . . . . . . . . + . . . . . . . . | F0 E1: Holding @ A0
| . . . . . + . . . . . . . . . . . . + . . . . . . . . . | s0 E3: Holding @ A0
| . . . . . + . . . . . . v8. . . . + . . . . . . . . . . | T0 E5: Holding @ A0
| . . . . . + . . . . . . . . . . + . . . . . . . . . . . | Y0 E2: Holding @ A0
| . . . . . + . . . . . . . . . E7. . . . . . . . . . . . |
| . . . . . + . . . . . . . . + . . . . . . . . . . . . . |
------------5---------------4------------------------------
Plane 'b' collided with plane 'u'.                          *******************
                                                             ATC - by Ed James
Hit space for top players list...                           *******************

