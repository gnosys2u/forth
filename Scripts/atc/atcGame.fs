//======== atcGame ========

class: atcGame extends iAtcGame

  iAtcRegion region
  //Array pendingTakeoffs
  atcInputHandler inputHandler
  String message
  
  int updateRate
  int newplaneRate
 
  int commandKey
  
  m: init
    `game` -> tag
    new String -> name
    name.set("games/")
    if( dup strlen 0=)
      drop "default"
    endif
    name.append
    //name.get %s `!` %c %nl
    new Array -> airplanes
    new List -> activeAirplanes
    new List -> inactiveAirplanes
    new atcConsoleDisplay -> display
    new atcRegion -> region
    new atcInputHandler -> inputHandler
    new String -> message
    inputHandler.init( region display )
  ;m
  
  : reset
    oclear name
    oclear activeAirplanes
    oclear inactiveAirplanes
    oclear airplanes
    oclear display
    region.reset
    oclear region
    oclear inputHandler
    oclear message
  ;
  
  m: delete
    //t{ "deleting game\n" %s }t
    reset
    super.delete
  ;m
  
  : startGame
    atcAirplane plane
    updateRate 1000 * -> status.moveInterval
    display.init(region)
    ms@ dup -> status.gameStartTime
            -> status.nextMoveTime
    0 -> status.moves
    0 -> status.nextPlaneMove
    true -> status.keepPlaying
    display.showWarning( "" )
    do(26 0)
      // create all airplanes and add to inactive list
      new atcAirplane -> plane
      plane.init( region i )
      new String -> plane.name
      plane.name.appendChar(`A` i +)
      airplanes.push(plane)
      inactiveAirplanes.addTail(plane)
      oclear plane
    loop
  ;
  
  : exitWithFailure
    display.showWarning(message.get)
    key drop
    false -> status.keepPlaying
  ;
  
  // newPlaneX newPlaneY plane ...
  m: moveAirplane
    -> iAtcAirplane plane
    -> int newY
    -> int newX
    //display.showWarning(plane.name.get) %bl newX %d %bl newY %d %bl
    region.getTile( plane.x plane.y ) ->o atcTile oldTile
    oldTile.removeAirplane( plane )
    t{ "plane " %s plane.name.get %s " moved to " %s newX %d `,` %c newY %d " at " %s plane.altitude %d %nl }t
    if( and(within(newX 0 region.columns) within(newY 0 region.rows)) )
      // plane still in airspace
      region.getTile(newX newY) ->o atcTile newTile
      
      message.set("Plane ")
      message.append(plane.name.get)
      if( plane.altitude 0=)
        true -> int legalLanding
        // plane either landed or crashed
        message.append(" crashed ")
        if(newTile.tileType kATTAirport <>)
          message.append("- must land at an airport")
          false -> legalLanding
        endif
        if(plane.destinationType kATTAirport <>)
          message.append("- destination was an exit")
          false -> legalLanding
        endif
        // see if landed at destination airport
        if(legalLanding)
          if( newTile.index plane.destination =)
            // legal landing
            1 ->+ game.status.score
            game.deactivateAirplane(plane)
          else
            // illegal landing
            message.append("- landed at wrong airport")
            false -> legalLanding
          endif
        endif
        if(not(legalLanding))
          // crashed
          exitWithFailure
        endif
      else
        // plane still flying
        newX -> plane.x   newY -> plane.y
        newTile.addAirplane( plane )
        if( plane.fuel 0<)
          message.append(" ran out of fuel")
          exitWithFailure
        else
          1 ->- plane.fuel
        endif
      endif
    else
      // plane exited airspace
      true -> int legalExit
      message.set("Plane ")
      message.append(plane.name.get)
      message.append(" exited illegaly")
      
      if(plane.destinationType kATTPortal <>)
        false -> legalExit
        message.append(" - destination was airport")
      endif
      if(plane.altitude 9000 <>)
        false -> legalExit
        message.append(" - wrong altitude")
      endif
      
      if(oldTile.tileType kATTPortal =)
        region.portals.get(oldTile.index) ->o iAtcPortal portal
        if( portal.id plane.destination <>)
          false -> legalExit
          message.append(" - wrong exit")
        endif
      else
        false -> legalExit
        message.append(" - not at an exit")
      endif
      if(legalExit)
        // legal exit
        1 ->+ game.status.score
        game.deactivateAirplane(plane)
      else
        // illegal exit
        exitWithFailure
      endif
    endif
    oclear plane  
  ;m

  m: activateAirplane  // PLANE_OBJ ...
    ->o atcAirplane plane
    activeAirplanes.addTail( plane )
    inactiveAirplanes.remove( plane )
  ;m
  
  m: deactivateAirplane  // PLANE_OBJ ...
    ->o atcAirplane plane
    inactiveAirplanes.addTail( plane )
    activeAirplanes.remove( plane )
  ;m
  
  : getGameTime
    ms@ status.gameStartTime -
  ;
  
  : updateTime
    //display.showWarning("time ") status.now %d %bl status.nextUpdateTime %d %bl status.updateInterval %d
    if( ms@ status.nextMoveTime >= )
      status.moveInterval ->+ status.nextMoveTime
      1 ->+ status.moves
      true
    else
      false
    endif
  ;

  : generateNewAirplanes
    // add airplanes 
    iAtcAirplane plane
    int destinationNum
    int x  int y   int altitude   int heading
    int destination
    int destinationType
    false -> int isJet
    false -> int isMoving
  
    if( status.moves status.nextPlaneMove >= )
      newplaneRate ->+ status.nextPlaneMove
      // TODO: create a plane at a portal or airport
      inactiveAirplanes.unrefHead -> plane
      if(objNotNull(plane))
        region.airports.count -> int numAirports
        region.portals.count -> int numPortals
        numAirports numPortals + -> int numDestinations
    
        mod( rand numDestinations ) -> int source
        rand 1 and 0<> -> isJet
        if( source numPortals < )
          // generate plane at a portal
          region.portals.get( source ) ->o iAtcPortal portal
          // TODO: don't allow portal to generate planes too close together
          portal.entryAltitude -> altitude
          true -> isMoving
          portal.x -> x
          portal.y -> y
          portal.entryDirection -> heading
        else
          // generate plane at an airport
          region.airports.get( source numPortals - ) ->o iAtcAirport airport
          airport.altitude -> altitude
          airport.x -> x
          airport.y -> y
          airport.entryDirection -> heading
        endif
    
        source -> int destination
        begin
        while(source destination =)   // make sure source and destination are different
          mod( rand numDestinations ) -> destination
        repeat
        
        if( destination numPortals < )
          // destination is a portal
          kATTPortal -> destinationType
        else
          // destination is an airport
          kATTAirport -> destinationType
          numPortals ->- destination
        endif
      
        // x y altitude heading isJet destination destinationType isMoving ...
        plane.setPosition( x y altitude heading )
        plane.setDestination( destination destinationType )
        plane.activate( status.moves isJet isMoving )
      
        activeAirplanes.addTail( plane )
        oclear plane
      endif
    endif
  ;
  
  : checkAirplanes
    activeAirplanes.headIter -> Iter iter
    -1 -> int collidedPlaneId
    setConsoleCursor(0 40)
    begin
    while( iter.next )
      -> atcAirplane plane
      if(plane.status kAASMovingMarked >=)
        region.checkCollisions(plane) -> collidedPlaneId
        if(collidedPlaneId 0>=)
          airplanes.get(collidedPlaneId) -> iAtcAirplane collidedPlane
          message.set("Plane ")
          message.append(plane.name.get)
          message.append(" collided with Plane ")
          message.append(collidedPlane.name.get)
          oclear collidedPlane
          exitWithFailure
        endif
      endif
      oclear plane
    repeat
    oclear iter
  ;
  
  : reportGameResults
  ;
  
  : updatePlanes
    activeAirplanes.headIter -> Iter iter
    begin
    while( iter.next )
      -> atcAirplane plane
      plane.update( status.moves )
      oclear plane
    repeat
    oclear iter
  ;
  
  : handleArrivals
    activeAirplanes.headIter -> Iter iter
    begin
    while( iter.next )
      -> atcAirplane plane
      plane.getPos -> long oldPos
      region.getTile( oldPos ) -> atcTile oldTile
      
      oclear plane
      oclear oldTile
    repeat
    oclear iter
  ;
  
  : cleanupGame
  ;
  
  : updateGame
    if( updateTime )
      display.hideAirplanes( activeAirplanes )
      updatePlanes
      //handleArrivals
      generateNewAirplanes
      checkAirplanes
      display.update( activeAirplanes )
      // setConsoleCursor(0 40) ds
    endif
  ;

  // COMMAND_INFO_PTR ...
  m: executeCommand
    -> ptrTo atcCommandInfo commandInfo
    if( commandInfo.airplaneNum airplanes.count < )
      airplanes.get(commandInfo.airplaneNum) -> iAtcAirplane plane
      //t{ "executeCommand " %s  commandInfo.airplaneNum %d %bl plane.beaconNum %d %nl }t
      if( and(commandInfo.command kNumCommands <   commandInfo.command 0>=) )
        plane.executeCommand( commandInfo display )
      else
        //display.showWarning("unexpected command ") commandInfo.command %d
        display.startWarning "unexpected command " %s commandInfo.command %d
      endif
      oclear plane
    endif
  ;m

  : updateInputs
    inputHandler.update
    inputHandler.isGameOver not -> status.keepPlaying
    if( and(status.keepPlaying inputHandler.commandInfo.isReady) )
      executeCommand( inputHandler.commandInfo )
    endif
  ;
 
  m: play
    startGame
    begin
    while( status.keepPlaying )
      updateInputs
      updateGame
      ms( 16 )   // sleep for about a sixtieth of a second between time checks
    repeat
    
    reportGameResults
    cleanupGame
  ;m
  
;class
