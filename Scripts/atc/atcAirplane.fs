//======== airplane ========

class: atcAirplane extends iAtcAirplane

  iAtcRegion region
  int nextUpdateTime
  int updateInterval        // 1 for jets, 2 for regular planes
    
  : updateAltitude
    int airportNum
    if( commandedAltitude altitude = )
      exit
    endif
    if( commandedAltitude altitude > )
      1000 ->+ altitude
    else
      1000 ->- altitude
    endif
  ;
  
  : updateHeading
    0 -> int rightTurns
    0 -> int leftTurns
    0 -> int deltaHeading
    
    if( circle )
      heading circle 2* + 7 and dup -> heading -> commandedHeading
      exit
    endif
    
    if( commandedHeading heading = )
      exit
    endif
    
    if( commandedHeading heading > )
      commandedHeading heading - dup -> rightTurns
      8 swap - -> leftTurns
    else
      heading commandedHeading - dup -> leftTurns
      8 swap - -> rightTurns
    endif
    t{ "old heading " %s heading 45 * %d " commandedHeading " %s commandedHeading 45 * %d }t
    
    case(forceDirection)

      of(1)
        rightTurns -> deltaHeading
      endof

      of(-1)
        negate(leftTurns) -> deltaHeading
      endof

      if( rightTurns leftTurns < )
        rightTurns -> deltaHeading
      else
        negate(leftTurns) -> deltaHeading
      endif
      
    endcase

    if(deltaHeading 2 >)
      2 -> deltaHeading
    else
      if(deltaHeading -2 <)
        -2 -> deltaHeading
      endif
    endif
   
    heading deltaHeading + 7 and -> heading
    t{ "  rightTurns " %s rightTurns %d " left turns " %s leftTurns %d "   new heading " %s heading 45 * %d %nl }t
  ;
  
  : calculateNextPosition
    // don't actually change position, atcWorld.moveAirplane will do that
    // return newX newY
    case( heading )
      kADNorth      of x        y 1-    endof
      kADNorthEast  of x 1+     y 1-    endof
      kADEast       of x 1+     y       endof
      kADSouthEast  of x 1+     y 1+    endof
      kADSouth      of x        y 1+    endof
      kADSouthWest  of x 1-     y 1+    endof
      kADWest       of x 1-     y       endof
      kADNorthWest  of x 1-     y 1-    endof
    endcase
  ;
  
  m: init   // REGION ID
    `plan` -> tag
    -> id
    ->o region  // break circular reference  region->tile->airplane->region
  ;m

  m: setPosition     // X Y ALTITUDE HEADING
    dup -> heading    -> commandedHeading
    dup -> altitude   -> commandedAltitude
    -> y
    -> x
  ;m
  
  m: setDestination    // DESTINATION DESTINATION_TYPE ...
    -> destinationType
    -> destination
  ;m
  
  m: activate    // UPDATE_TIME IS_JET IS_MOVING ...
    if kAASMovingMarked else kAASHolding endif -> status

    name.clear
    if
      1 -> updateInterval
      name.appendChar( `a` id +)
    else
      2 -> updateInterval
      name.appendChar( `A` id +)
    endif
  
    -> nextUpdateTime

    -1 -> beaconNum
    0 -> circle
    region.startingFuel -> fuel
  ;m
  
  m: delete
    //t{ "deleting plane " %s thisData %x %bl name.get %s %nl }t
    oclear name
    super.delete
  ;m
  
  m: update  // now ...
    -> int now
    
    if( now nextUpdateTime > )
      if( status kAASMovingMarked >= )
        updateInterval ->+ nextUpdateTime
        updateAltitude
        
        if( beaconNum 0< )
          updateHeading
        else
    
          if( region.getBeacon(beaconNum).at(x y) )
            // plane has arrived at beacon
            -1 -> beaconNum
            if(kAASMovingUnmarked status =)
              kAASMovingMarked -> status
            endif
            updateHeading
          endif
        endif
        game.moveAirplane( calculateNextPosition this  )
      endif
    
    endif
  ;m
  
  
  m: executeCommand // COMMAND_INFO_PTR DISPLAY_OBJECT ...
    ->o iAtcDisplay display
    -> ptrTo atcCommandInfo commandInfo
    if( status kAASMovingMarked >= )
      case( commandInfo.command)
      
        of(kACTAltitude)
          commandInfo.amount 1000 * -> commandedAltitude
          if(commandInfo.isRelative)
            altitude ->+ commandedAltitude
          endif
          if(commandedAltitude kMaxAltitude >)
            kMaxAltitude -> commandedAltitude
          else
            if(commandedAltitude 0<)
              0 -> commandedAltitude
            endif
          endif
        endof
    
        of(kACTMark)
          kAASMovingMarked -> status
        endof
    
        of(kACTIgnore)
          kAASMovingIgnored -> status
        endof
      
        of(kACTUnmark)
          kAASMovingUnmarked -> status
        endof
      
        of(kACTCircle)
          if(commandInfo.forceDirection 0<) -1 else 1 endif -> circle
          commandInfo.beaconNum -> beaconNum
        endof
    
        of(kACTTurn)
          0 -> circle
          commandInfo.amount -> commandedHeading
          commandInfo.beaconNum -> beaconNum
          if(commandInfo.isRelative)
            heading ->+ commandedHeading
          endif
          if(commandedHeading 0<)
            kNumDirections ->+ commandedHeading
          else
            if(commandedHeading kNumDirections >)
              kNumDirections ->- commandedHeading
            endif
          endif
        endof
      
        of(kACTTurnTowards)
          commandInfo.beaconNum -> beaconNum
        endof
   
        //display.showWarning("unexpected command ") %d
        display.startWarning "unexpected command " %s dup %d
      endcase
    
    else

      // plane not moving, only possible command is takeoff (altitude)
      if( commandInfo.command kACTAltitude = )
        if( commandInfo.amount 0> )
          // handle takeoff
          kAASMovingMarked -> status
          region.startingFuel -> fuel
          commandInfo.amount -> commandedAltitude
        endif
      else
        //display.showWarning("only valid command is altitude(takeoff)")
        display.startWarning "only valid command is altitude(takeoff)" %s
      endif
    endif
  
  ;m
  
;class


