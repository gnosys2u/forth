\ reader for atc files using a hacky forth vocabulary

\ sample file:

\ update = 7;
\ newplane = 12;
\ width = 15;
\ height = 15;
\ exit:     (  7  0 x ) ( 14  0 z ) ( 12 14 q ) (  0 14 e ) ;
\ beacon:   ( 12  7 ) ;
\ airport:  (  7  8 w ) ;
\ line:     [ (  1  1 ) (  6  6 ) ]
\           [ (  1  7 ) ( 11  7 ) ] ;

vocabulary atcFileReader
also atcFileReader definitions

atcGame daGame

\ directions are based on the 8 keyboard keys surrounding the 's' key
\ for airports this is runway direction, for portals this is entry direction
enum: atcFileDirections
  w e d c x z a q
;enum

enum: atcFileCommands
  kCmdExit
  kCmdBeacon
  kCmdAirport
  kCmdLine
;enum

: getValue
  \ rest of line should be like " = 11;"
  if( strcmp( blword "=" ) )
    error( 2 )
    addErrorText( "missing equals in atc file" )
  endif
  $evaluate( $word( ';' ) )
;

int regionWidth
int regionHeight

: update    getValue daGame.updateRate! ;
: newplane  getValue daGame.newplaneRate! ;
: width
  getValue regionWidth!
  if( regionHeight )
    daGame.region.init(regionWidth regionHeight)
  endif
;

: height
  getValue regionHeight!
  if( regionWidth )
    daGame.region.init(regionWidth regionHeight)
  endif
;

\ turn off normal processing of parentheses

int numArgs
atcFileCommands command

kFFParenIsExpression removeFeatures
: ( ;
: ) numArgs++ ;
kFFParenIsExpression addFeatures

: [ ;
: ] ;
: exit: kCmdExit command! numArgs~ ;
: beacon: kCmdBeacon command! numArgs~ ;
: airport: kCmdAirport command! numArgs~ ;
: line: kCmdLine command! numArgs~ ;
: # 0 $word drop ;  \ eat all comments

: ;
  numArgs 1- int id!
  case(command)

    of(kCmdExit)
      do( numArgs 0 )
        id daGame.region.addPortal
        id--
      loop
    endof

    of(kCmdBeacon)
      do( numArgs 0 )
        id daGame.region.addBeacon
        id--
      loop
    endof

    of(kCmdAirport)
      do( numArgs 0 )
        id daGame.region.addAirport
        id--
      loop
    endof

    of(kCmdLine)
      do( numArgs 0 )
        daGame.region.addLine
      +loop(2)
    endof
      
    "unexpected command in atc file" %s dup %d
  endcase
  
  numArgs~
forth:;

kFFParenIsExpression removeFeatures
: );
  ) ;
forth:;
kFFParenIsExpression addFeatures

: ];
  ] ;
forth:;

\ only forth

\ ATC_FILENAME ATC_GAME_OBJ ... TRUE   if successfuly read file
: loadAtcFile
  daGame!
  ptrTo byte gameName!
  
  if( fexists(gameName))
  
    regionWidth~    regionHeight~
  
  
    also atcFileReader
    decimal
    kFFParenIsExpression removeFeatures
  
    $runFile( gameName )
  
    only forth
    kFFParenIsExpression addFeatures
    true
  else
    "Could not find atc game file " %s gameName %s %nl
    false
  endif
  
  oclear daGame
forth:;

only forth definitions
