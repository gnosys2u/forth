//======== atcInputHandler ========

kNumCommandStates arrayOf op inputStateOps
  
class: atcInputHandler extends iAtcInputHandler

  int cmdChar    // converted to lowercase
  int oCmdChar    // original keyboard char
  
  : c2dir
    strchr(directionChars swap)
    if( ?dup )
      directionChars -
    else
      -1
    endif
  ;
  
  : c2dir$
    case
      `w` of "0" endof
      `e` of "45" endof
      `d` of "90" endof
      `c` of "135" endof
      `x` of "180" endof
      `z` of "225" endof
      `a` of "270" endof
      `q` of "315" endof
      "<bad direction>"
    endcase
  ;
  
  : setIdle
    displayString.clear
    commandString.clear
    kACSIdle -> commandState
  ;
  
  : executeCommand
    t{ "command: " %s displayString.get %s %nl }t
    setIdle
    display.showCommand("")
    display.showWarning("")
    true -> commandInfo.isReady
  ;
  
  : unrecognizedCommandWarning
    display.startWarning `"` %c cmdChar %c `"` %c "unrecognized, valid command letters are " %s %s
  ;

  : handleIdle
    false -> commandInfo.isReady
    -1 -> commandInfo.beaconNum
    0 -> commandInfo.forceDirection
    if( within( cmdChar `a` `z` 1+ ))
      cmdChar `a` - -> commandInfo.airplaneNum
      kACSPlaneSelected -> commandState
      commandString.appendChar( cmdChar )
      displayString.clear
      displayString.appendChar( cmdChar )
    else
      if( cmdChar `\r` =)
        // force an immediate game turn
        ms@ -> game.status.nextMoveTime
      else
        display.startWarning "airplane name a..z is required" %s
      endif
    endif
  ;
  
  : handlePlaneSelected
    // valid commands: amuict  altitude mark unmark ignore circle turn
    case( cmdChar )
      `a` of  // altitude
        commandString.appendChar( cmdChar )
        displayString.append( " altitude" )
        kACSAltitude -> commandState
        kACTAltitude -> commandInfo.command
      endof
      
      `m` of  // mark
        commandString.appendChar( cmdChar )
        displayString.append( " mark" )
        kACSCommandReady -> commandState
        kACTMark -> commandInfo.command
      endof
      
      `u` of  // unmark
        commandString.appendChar( cmdChar )
        displayString.append( " unmark" )
        kACSCommandReady -> commandState
        kACTUnmark -> commandInfo.command
      endof
      
      `i` of  // ignore
        commandString.appendChar( cmdChar )
        displayString.append( " ignore" )
        kACSCommandReady -> commandState
        kACTIgnore -> commandInfo.command
      endof
      
      `c` of  // circle
        commandString.appendChar( cmdChar )
        displayString.append( " circle" )
        kACSCircle -> commandState
        0 -> commandInfo.forceDirection
        kACTCircle -> commandInfo.command
      endof
      
      `t` of  // turn
        commandString.appendChar( cmdChar )
        displayString.append( " turn" )
        kACSTurn -> commandState
        kACTTurn -> commandInfo.command
        0 -> commandInfo.forceDirection
      endof
      
      unrecognizedCommandWarning( "actmui  altitude/circle/turn/mark/unmark/ignore" )
    endcase
  ;
  
  : handleCommandReady
    if( cmdChar `\r` =)
      executeCommand
    else
      //display.showWarning( "hit ENTER" )
      display.startWarning "hit ENTER" %s
    endif
  ;

  : handleAltitude
    cmdChar `0` - -> int altitude
    case( cmdChar )
      `-` of  `d` -> cmdChar endof
      `+` of  `c` -> cmdChar endof
    endcase
  
    case( cmdChar )
      `c` of
        commandString.appendChar( cmdChar )
        displayString.append( " climb" )
        kACSAltitudeClimb -> commandState
      endof
      
      `d` of
        commandString.appendChar( cmdChar )
        displayString.append( " descend" )
        kACSAltitudeDescend -> commandState
      endof

      if( within(altitude 0 10) )
        commandString.appendChar( cmdChar )
        displayString.appendChar( ` ` )
        displayString.appendChar( cmdChar )
        displayString.append( "000" )
        kACSCommandReady -> commandState
        altitude -> commandInfo.amount
        false -> commandInfo.isRelative
      else
        unrecognizedCommandWarning("cd0123456789")
      endif
      
    endcase
  ;
  
  : handleAltitudeClimb
    cmdChar `0` - -> int altitude
    if( within(altitude 1 10) )
      commandString.appendChar( cmdChar )
      displayString.appendChar( ` ` )
      displayString.appendChar( cmdChar )
      displayString.append( "000" )
      kACSCommandReady -> commandState
      altitude -> commandInfo.amount
      true -> commandInfo.isRelative
      // TODO: check climb is in range
    else
      unrecognizedCommandWarning("0123456789")
    endif
  ;
  
  : handleAltitudeDescend
    `0` cmdChar - -> int altitude
    if( within(altitude -9 0) )
      commandString.appendChar( cmdChar )
      displayString.appendChar( ` ` )
      displayString.appendChar( cmdChar )
      displayString.append( "000" )
      kACSCommandReady -> commandState
      altitude -> commandInfo.amount
      true -> commandInfo.isRelative
      // TODO: check descend is in range
    else
      // TODO: report error
    endif
  ;
  
  : handleCircle
    case( cmdChar )
      `l` of
        commandString.appendChar( cmdChar )
        displayString.append( " left" )
        kACSCircleReady -> commandState
        -1 -> commandInfo.forceDirection
      endof
    
      `r` of
        commandString.appendChar( cmdChar )
        displayString.append( " right" )
        kACSCircleReady -> commandState
        1 -> commandInfo.forceDirection
      endof
    
      `a` of
        commandString.appendChar( cmdChar )
        displayString.append( " at" )
        kACSAt -> commandState
      endof
    
      `@` of
        commandString.appendChar( cmdChar )
        displayString.append( " at" )
        kACSAt -> commandState
      endof
    
      `\r` of
        0 -> commandInfo.forceDirection
        executeCommand
      endof
    
      unrecognizedCommandWarning("lra or ENTER")
    endcase
  ;
  
  : handleDelayableReady
    case( cmdChar )
      `a` of
        commandString.appendChar( cmdChar )
        displayString.append( " at" )
        kACSAt -> commandState
      endof
      
      `@` of
        commandString.appendChar( cmdChar )
        displayString.append( " at" )
        kACSAt -> commandState
      endof
      
      `\r` of
        executeCommand
      endof
      
      // display.showWarning( "valid command letters are a or ENTER" )
      unrecognizedCommandWarning("a or ENTER")
    endcase
  ;
  
  : handleTurn
    case( oCmdChar )
      `-` of  `l` -> oCmdChar endof
      `+` of  `r` -> oCmdChar endof
    endcase
  
    case( oCmdChar )
      `l` of
        commandString.appendChar( oCmdChar )
        displayString.append( " left" )
        kACSTurnDirection -> commandState
        -1 -> commandInfo.amount
        true -> commandInfo.isRelative
        -1 -> commandInfo.forceDirection
      endof
      
      `r` of
        commandString.appendChar( oCmdChar )
        displayString.append( " right" )
        kACSTurnDirection -> commandState
        1 -> commandInfo.amount
        true -> commandInfo.isRelative
        1 -> commandInfo.forceDirection
      endof
      
      `L` of
        commandString.appendChar( oCmdChar )
        displayString.append( " left 90" )
        kACSTurnReady -> commandState
        -2 -> commandInfo.amount
        true -> commandInfo.isRelative
        -1 -> commandInfo.forceDirection
      endof
      
      `R` of
        commandString.appendChar( oCmdChar )
        displayString.append( " right 90" )
        kACSTurnReady -> commandState
        2 -> commandInfo.amount
        true -> commandInfo.isRelative
        1 -> commandInfo.forceDirection
      endof
      
      `t` of
        commandString.appendChar( oCmdChar )
        displayString.append( " towards" )
        kACSTurnTowards -> commandState
      endof
      
      c2dir( cmdChar )
      if( dup 0>= )
        -> commandInfo.amount
        commandString.appendChar( cmdChar )
        displayString.appendChar( ` ` )
        displayString.append( c2dir$( cmdChar ) )
        false -> commandInfo.isRelative
        kACSTurnReady -> commandState
      else
        drop unrecognizedCommandWarning("lrLRt or a direction key wedcxzaq")
      endif
    
    endcase
  ;
  
  : handleTurnDirection
    case( cmdChar )
      `a` of
        commandString.appendChar( cmdChar )
        displayString.append( " at" )
        kACSAt -> commandState
      endof
      
      `@` of
        commandString.appendChar( cmdChar )
        displayString.append( " at" )
        kACSAt -> commandState
      endof
      
      `\r` of
        executeCommand
      endof
      
      c2dir
      if( dup 0>= )
        -> commandInfo.amount
        commandString.appendChar( cmdChar )
        displayString.appendChar( ` ` )
        displayString.append( c2dir$( cmdChar ) )
        kACSTurnReady -> commandState
      else
        drop unrecognizedCommandWarning("a, ENTER or a direction key wedcxzaq")
      endif
    
    endcase
  ;
  
  : handleTurnTowards
    case( cmdChar )
      `a` of
        commandString.appendChar( cmdChar )
        displayString.append( " airport" )
        kACSTurnTowardsAirport -> commandState
      endof
      
      `b` of
        commandString.appendChar( cmdChar )
        displayString.append( " beacon" )
        kACSTurnTowardsBeacon -> commandState
      endof
      
      `*` of
        commandString.appendChar( cmdChar )
        displayString.append( " beacon" )
        kACSTurnTowardsBeacon -> commandState
      endof
      
      `e` of
        commandString.appendChar( cmdChar )
        displayString.append( " exit" )
        kACSTurnTowardsExit -> commandState
      endof
      
      drop unrecognizedCommandWarning("abe")
    
    endcase
  ;
  
  : handleTurnTowardsAirport
    cmdChar `0` - -> int targetNum
    if( within( targetNum 0 10))
      kACSTurnReady -> commandState
      targetNum -> commandInfo.targetNum
      kATTAirport -> commandInfo.targetType
      commandString.appendChar( cmdChar )
      displayString.appendChar( ` ` )
      displayString.appendChar( cmdChar )
    else
      unrecognizedCommandWarning("0123456789")
    endif
  ;
  
  : handleTurnTowardsBeacon
    handleTurnTowardsAirport
    kATTBeacon -> commandInfo.targetType
  ;
  
  : handleTurnTowardsExit
    handleTurnTowardsAirport
    kATTPortal -> commandInfo.targetType
  ;
  
  : handleAt
    if( cmdChar `b` = )
      kACSAtBeacon -> commandState
      commandString.appendChar( cmdChar )
      displayString.append( " beacon" )
    else
      //display.showWarning( "hit b" )
      unrecognizedCommandWarning("b")
    endif
  ;
  
  : handleAtBeacon
    cmdChar `0` - -> int beaconNum
    if( within( beaconNum 0 10))
      kACSCommandReady -> commandState
      beaconNum -> commandInfo.beaconNum
      commandString.appendChar( cmdChar )
      displayString.appendChar( ` ` )
      displayString.appendChar( cmdChar )
    else
      unrecognizedCommandWarning("ENTER")
    endif
  ;
  
  ' handleIdle -> inputStateOps( kACSIdle )
  ' handlePlaneSelected -> inputStateOps( kACSPlaneSelected )
  ' handleCommandReady -> inputStateOps( kACSCommandReady )
  ' handleAltitude -> inputStateOps( kACSAltitude )
  ' handleAltitudeClimb -> inputStateOps( kACSAltitudeClimb )
  ' handleAltitudeDescend -> inputStateOps( kACSAltitudeDescend )
  ' handleCircle -> inputStateOps( kACSCircle )
  ' handleDelayableReady -> inputStateOps( kACSCircleReady )
  ' handleTurn -> inputStateOps( kACSTurn )
  ' handleTurnDirection -> inputStateOps( kACSTurnDirection )
  ' handleDelayableReady -> inputStateOps( kACSTurnReady )
  ' handleTurnTowards -> inputStateOps( kACSTurnTowards )
  ' handleTurnTowardsAirport -> inputStateOps( kACSTurnTowardsAirport )
  ' handleTurnTowardsBeacon -> inputStateOps( kACSTurnTowardsBeacon )
  ' handleTurnTowardsExit -> inputStateOps( kACSTurnTowardsExit )
  ' handleAt -> inputStateOps( kACSAt )
  ' handleAtBeacon -> inputStateOps( kACSAtBeacon )

  : setCommandCharacter
    dup -> oCmdChar
    tolower -> cmdChar
  ;

  : handleBackspace
    // handle backspace/delete:
    //  set state back to idle
    //  remove last char from commandString
    mko String tmpCommandString
    tmpCommandString.set( commandString.get )
    tmpCommandString.resize( tmpCommandString.length 1- )
    tmpCommandString.get -> ptrTo byte pSrc
    setIdle
    // process commandString chars one at a times
    begin
      b@@++(ref pSrc) -> byte ch
    while( ch 0<> )
      setCommandCharacter( ch )
      inputStateOps( commandState )
    repeat
    display.showCommand( displayString.get )
    oclear tmpCommandString
  ;
  
  m: init     // REGION DISPLAY_OBJ  ...
    `inph` -> tag
    -> display
    ->o region
    new String -> displayString
    new String -> commandString
    kACSIdle -> commandState
    0 -> cmdChar
    false -> isGameOver
  ;m
  
  m: delete
    //t{ "deleting inputHandler " %s %nl }t
    oclear display
    oclear displayString
    oclear commandString
    super.delete
  ;m
  
  m: update
    if( key? )
      
      setCommandCharacter(key)

      if( or( cmdChar `\b` = cmdChar 0x7f = ) )

        handleBackspace
      
      else
        if( cmdChar 0x1b = )
          true -> isGameOver
        else
          if( within( commandState kACSIdle kNumCommandStates ) )
            inputStateOps( commandState )
          else
            // TODO: report bad command state error
          endif
        endif

      endif
      
      display.useDefaultColors
      display.showCommand( displayString.get )
   
    endif
  ;m
  
;class

