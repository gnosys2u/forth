//======== atcConsoleDisplay ========

class: atcConsoleDisplay extends iAtcDisplay
  iAtcRegion region
  int rowBytes
  int statusColumn
  int statusRow
  int commandRow
  int helpRow
  int defaultColors
  
  m: delete
    //t{ "deleting display " %s %nl }t
    oclear text
    super.delete
  ;m
  
  // each tile corresponds to 2 horizontal chars
  
  enum: consoleColors
    kCCBlack  kCCDarkBlue  kCCDarkGreen  kCCDarkCyan  kCCDarkRed  kCCDarkPurple  kCCDarkYellow  kCCDimWhite
    kCCGray   kCCBlue      kCCGreen      kCCCyan      kCCRed      kCCPurple      kCCYellow      kCCWhite
  ;enum

  : setTextColors    // BACKGROUND_COLOR TEXT_COLOR ...
    swap 4 lshift or setConsoleColor
  ;

  m: useDefaultColors
    setTextColors( kCCBlack kCCDimWhite)
  ;m
  
  m: useHighlightColors
    setTextColors( kCCWhite kCCBlack )
  ;m
  
  : xyAddr        // X Y ... BYTE_ADDR
    rowBytes * swap 2* + text.base +
  ;

  : drawRadar
    do( region.rows 0 )
      xyAddr(0 i) %s %nl
    loop
  ;
  
  m: init         // REGION ...
    `disp` -> tag
    ->o region
    getConsoleColor -> defaultColors
    new ByteArray -> text
    region.columns -> int columns
    region.rows -> int rows
    columns 2* 1+ -> rowBytes
    text.resize(rowBytes columns *)
    text.base -> ptrTo byte pBase
    ref pBase -> ptrTo byte pDst

    columns 2* 2+ -> statusColumn
    3 -> statusRow      // row of first airplane status
    rows 2+ -> helpRow
    rows 1+ -> commandRow

    // each tile in our region will have 2 characters in a horizontal row
    // the extra space characters 

    // fill 'text' byte array with the ascii image of the airspace
    
    // draw the top line of dashes   ------
    do( rowBytes 2- 0 )
      `-` pDst b@!++
    loop
    bl pDst b@!++
    0 pDst b@!++

    // draw the lines below top line | . . . |
    do( rows 2- 0 )
      `|` pDst b@!++
      bl pDst b@!++
      do( columns 2- 0)
        `.` pDst b@!++
        bl pDst b@!++
      loop
      `|` pDst b@!++
      bl pDst b@!++
      0 pDst b@!++
    loop
    
    // draw the bottom line of dashes   ------
    do( rowBytes 2- 0 )
      `-` pDst b@!++
    loop
    bl pDst b@!++
    0 pDst b@!++

    // draw the lines
    region.lines.headIter -> Iter iter
    begin
    while( iter.next )
      -> iAtcLine line
      line.x0 -> int x
      line.y0 -> int y
      line.x1 line.x0 icmp -> int dx
      line.y1 line.y0 icmp -> int dy
      
      begin
        //x %d %bl y %d %nl
        `+` xyAddr(x y) c!
        dx ->+ x
        dy ->+ y
      until( and(x line.x1 =  y line.y1 =) )
      `+` xyAddr(x y) c!
      
      oclear line
    repeat
    oclear iter
  
    int id
  
    // draw the beacons
    region.beacons.headIter -> iter
    begin
    while( iter.next )
      -> iAtcBeacon beacon
      xyAddr(beacon.x beacon.y) `*` over c!
      1 ->- id
      1+ beacon.id `0` + swap c!
      oclear beacon
    repeat
    oclear iter

    // draw the portals
    region.portals.headIter -> iter
    begin
    while( iter.next )
      -> iAtcPortal portal
      portal.id `0` + xyAddr(portal.x portal.y)  c!
      oclear portal
    repeat
    oclear iter
  
    // draw the airports
    region.airports.headIter -> iter
    begin
    while( iter.next )
      -> iAtcAirport airport
      xyAddr(airport.x airport.y) airport.entryDirection "^^>>vv<<" + c@ over c!
      1+ airport.id `0` +  swap c!
      oclear airport
    repeat
    oclear iter

    // draw the airspace on the screen
    setConsoleCursor(0 0)
    drawRadar
  ;m
  
  : updateStatus
    useDefaultColors
    
    setConsoleCursor(statusColumn 0)
    "Time: " %s game.status.moves %d
    setConsoleCursor(statusColumn 12+ 0)
    "Safe: " %s game.status.score %d
  
    setConsoleCursor(statusColumn 2)
    "pl dt  comm" %s
  
    setConsoleCursor(statusColumn helpRow)
    "*******************" %s
    setConsoleCursor(statusColumn helpRow 1+)
    " ATC - by Ed James" %s
    setConsoleCursor(statusColumn helpRow 2+)
    "*******************" %s
  ;

  : startPlaneStatus      // TOS is row#
    setConsoleCursor(statusColumn over)
    44 bl %nc
    setConsoleCursor(statusColumn swap)
  ;
  
  : updatePlaneStatus
    ->o List activeAirplanes
    statusRow -> int planeRow
  
    // display flying airplanes
    activeAirplanes.headIter -> Iter iter
    begin
    while( iter.next )
      ->o iAtcAirplane plane
      if( plane.status kAASMovingMarked >= )
        setConsoleCursor(plane.x 2* plane.y)
        if( plane.status kAASMovingMarked =)
          useHighlightColors
          setTextColors( kCCWhite kCCBlack )
          plane.name.get %s plane.altitude 1000 / %d
          setConsoleColor(defaultColors)
          startPlaneStatus(planeRow)
          1 ->+ planeRow
          plane.name.get %s plane.altitude 1000 / %d
          if(plane.fuel region.warningFuel >) bl else `*` endif %c    // low fuel indicator
          if(plane.destinationType kATTPortal =) `E` else `A` endif %c
          plane.destination %d `:` %c
          if( plane.circle )
            " Circle " %s
            if( plane.circle 0< )
              "counter" %s
            endif
            "clockwise" %s
          else
            if( plane.heading plane.commandedHeading <>)
              case(plane.forceDirection)
                of(1) " Turn right" endof
                of(-1) " Turn left" endof
                " Turn" swap
              endcase
              %s " to " %s plane.commandedHeading 45 * %d
            endif
          endif
          if( plane.beaconNum 0>=)
            " at beacon " %s plane.beaconNum %d
          endif
        else
          // unmarked or ignored plane
          useDefaultColors
          plane.name.get %s plane.altitude 1000 / %d
        endif
      endif
    repeat
  
    startPlaneStatus(planeRow)
    1 ->+ planeRow

    // display airplanes waiting to takeoff
    activeAirplanes.headIter -> iter
    begin
    while( iter.next )
      ->o iAtcAirplane plane
      if( plane.status kAASMovingMarked < )
        startPlaneStatus(planeRow)
        1 ->+ planeRow
        plane.name.get %s 0 %d %bl
        if( plane.destinationType kATTPortal = )
          `E` %c
        else
          `A` %c
        endif
        plane.destination %d `:` %c
      " Holding @ A" %s region.getAirport@(plane.x plane.y).id %d
      endif
    repeat
    oclear iter

    begin
    while(planeRow region.rows <)
      startPlaneStatus(planeRow)
      1 ->+ planeRow
    repeat
  ;
  
  m: update      // ACTIVE_AIRPLANE_LIST ...
    //getConsoleCursor -> long oldPos

    updatePlaneStatus
    updateStatus
    //updateRadar

  ;m
  
  m: showWarning      // CSTRING ...
    getConsoleColor swap
  
    setConsoleCursor(0 helpRow)
    region.columns 2* 0 do %bl loop
    setTextColors(kCCRed  kCCWhite)
    setConsoleCursor(0 helpRow)
    %s

    setConsoleColor  // restore original console colors
  ;m
  
  m: startWarning
    setConsoleCursor(0 helpRow)
    region.columns 2* 0 do %bl loop
    setTextColors(kCCRed  kCCWhite)
    setConsoleCursor(0 helpRow)
  ;m
  
  m: showCommand    // CSTRING ...
    setConsoleCursor(0 commandRow)
    region.columns 2* 0 do %bl loop
    setConsoleCursor(0 commandRow)
    %s
  ;m
  
  m: hideAirplanes    // ACTIVE_AIRPLANE_LIST
    ->o List activeAirplanes
    // hide flying airplanes
    activeAirplanes.headIter -> Iter iter

    begin
    while( iter.next )
      ->o iAtcAirplane plane
      if( plane.status kAASMovingMarked >= )
        setConsoleCursor(plane.x 2* plane.y)
        xyAddr(plane.x plane.y) dup c@ %c 1+ c@ %c
      endif
    repeat
  
    oclear iter
  ;m
  
;class


