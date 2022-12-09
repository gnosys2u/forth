//======== atcRegion ========

class: atcRegion extends iAtcRegion

  m: init
    `regn` -> tag
    2dup
    -> rows
    -> columns
    rows columns + -> startingFuel
    startingFuel 3+ 4 / -> warningFuel
    new Array -> tiles
    do(* 0)
      new atcTile
      dup <atcTile>.init
      tiles.push
    loop
    new Array -> airports
    new Array -> portals
    new Array -> beacons
    new Array -> lines
  ;m
  
  // x y ... atcTile@x,y
  : tile@
    columns * + tiles.get
  ;

  m: getTile returns atcTile
    columns * + tiles.get
  ;m

  // atcTile x y ...  
  m: setTile
    columns * + tiles.set
  ;m

  : resetTiles
    tiles.headIter -> Iter iter
    begin
    while( iter.next )
      ->o atcTile tile
      tile.reset
    repeat
    oclear iter
  ;
  
  m: reset
    if(objNotNull(tiles))
      resetTiles
      oclear tiles
      t{ airports.show }t
      oclear airports
      oclear portals
      oclear beacons
      oclear lines
    endif
  ;m
    
  m: delete
    //t{ "deleting region\n" %s }t
    reset
    super.delete
  ;m

  m: update
  ;m

  m: getBeacon returns iAtcBeacon
    beacons.get
  ;m
  
  m: getAirport returns iAtcAirport
    airports.get
  ;m // AIRPORT_NUM ... AIRPORT_OBJ
  
  m: getPortal returns iAtcPortal
    portals.get
  ;m // PORTAL_NUM ... PORTAL_OBJ
  
  m: getAirport@ returns iAtcAirport  // X Y ... AIRPORT_OBJ
    -> int y
    -> int x

    airports.headIter -> Iter iter
    begin
    while( iter.next )
      ->o iAtcAirport airport
      if(and( airport.x x =  airport.y y =))
        airport
        oclear iter
        exit
      endif
    repeat
    oclear iter
    null

  ;m
  
  m: getPortal@ returns iAtcPortal  // X Y ... PORTAL_OBJ
    -> int y
    -> int x

    portals.headIter -> Iter iter
    begin
    while( iter.next )
      ->o iAtcPortal portal
      if(and( portal.x x =  portal.y y =))
        portal
        oclear iter
        exit
      endif
    repeat
    oclear iter
    null

  ;m
  
  m: getBeacon@ returns iAtcBeacon  // X Y ... BEACON_OBJ
    -> int y
    -> int x

    beacons.headIter -> Iter iter
    begin
    while( iter.next )
      ->o iAtcBeacon beacon
      if(and( beacon.x x =  beacon.y y =))
        beacon
        oclear iter
        exit
      endif
    repeat
    oclear iter
    null
  
  ;m
  
  : setTileType
    getTile ->o atcTile tile
    -> tile.tileType
  ;
  
  m: checkCollisions    // PLANE ... COLLIDED_PLANE_INDEX  returns -1 if no collision
    ->o iAtcAirplane plane
    -1 -> int collidedPlaneIndex
    int minRow int maxRow int minColumn int maxColumn
    
    plane.x 1- -> minColumn
    if(minColumn 0<) 0 -> minColumn endif
    plane.x 1+ -> maxColumn
    if(maxColumn columns >=) 1 ->- maxColumn endif
    
    plane.y 1- -> minRow
    if(minRow 0<) 0 -> minRow endif
    plane.y 1+ -> maxRow
    if(maxRow rows >=) 1 ->- maxRow endif
    
    minRow -> int checkRow
    begin
    while(and(collidedPlaneIndex 0<  checkRow maxRow <=))
      minColumn -> int checkColumn
      begin
      while(and(collidedPlaneIndex 0<  checkColumn maxColumn <=))
        //"checking at " %s checkColumn %d %bl checkRow %d %nl
        getTile(checkColumn checkRow) ->o atcTile tile
        tile.airplanes.headIter -> Iter iter
        begin
        while(iter.next)
          ->o iAtcAirplane checkPlane
          //"checking " %s checkPlane.name.get %s " against " %s plane.name.get %s %nl
          if(checkPlane.id plane.id <>)
            if(within(checkPlane.altitude plane.altitude - -1 2))
              checkPlane.id -> collidedPlaneIndex
            endif
          endif
        repeat
        1 ->+ checkColumn
        oclear iter
      repeat
      1 ->+ checkRow
    repeat

    collidedPlaneIndex
  ;m
  
  // x y direction id ...
  m: addPortal
    -> int id
    -> int dir
    -> int y
    -> int x
    //"new portal at " %s x %d %bl y %d "  direction: " %s dir %d %nl
    mko atcPortal portal
    portal.init(x y dir id)
    portals.insert(portal 0)
    //portals.push(portal)
    oclear portal
    getTile(x y) ->o atcTile tile
    kATTPortal -> tile.tileType
    id -> tile.index
  ;m

  // x y direction id ...
  m: addAirport
    -> int id
    -> int dir
    -> int y
    -> int x
    t{ "new airport at " %s x %d %bl y %d "  direction: " %s dir %d %nl }t
    mko atcAirport airport
    airport.init(x y dir id)
    airports.insert(airport 0)
    //airports.push(airport)
    oclear airport
    getTile(x y) ->o atcTile tile
    kATTAirport -> tile.tileType
    id -> tile.index
  ;m
  
  // x y id ...
  m: addBeacon
    -> int id
    -> int y
    -> int x
    //"new beacon at " %s x %d %bl y %d %nl
    mko iAtcBeacon beacon
    beacon.init(x y id)
    beacons.insert(beacon 0)
    //beacons.push(beacon)
    oclear beacon
    getTile(x y) ->o atcTile tile
    kATTBeacon -> tile.tileType
    id -> tile.index
  ;m
  
  // x0 y0 x1 y1 ...
  m: addLine
    -> int y1
    -> int x1
    -> int y0
    -> int x0
    //"new line from " %s x0 %d `,` %c y0 %d " to " %s x1 %d `,` %c y1 %d %nl
    mko iAtcLine line
    line.init(x0 y0 x1 y1)
    lines.insert(line 0)
    //lines.push(line)
    oclear line
  ;m
  
;class
