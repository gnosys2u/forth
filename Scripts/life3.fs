\ Conway's Game of Life, or Occam's Razor Dulled

\ The original ANS Forth version by Leo Wong (see bottom) 
\   has been modified slightly to allow it to run under 
\   kForth. Also, delays have been changed from 1000 ms to 
\   100 ms for faster update --- K. Myneni, 12-26-2001

\ modified this to work in my forth without requiring ansi compatability mode
\   Pat McElhatton February 28 2017

requires sdlscreen
also sdl2

autoforget LIFE3
: LIFE3 ;

\ check stack depth matching with s[ CODE_TO_CHECK "message" ]s
IntArray _depths
: s[
  if(objIsNull(_depths))
    new IntArray _depths!
  endif
  _depths.push(depth)
;
  
\ check that stack depth matches what it was when previous s[ happened
: ]s
  depth _depths.pop - 1-
  ?dup if
    swap %s " depth mismatch of " %s %d %nl
  else
    drop
  endif
;

\ check that stack depth is N+ what it was when previous s[ happened
: ]sN   \ stringAddr N ...
  depth _depths.pop - 2- -
  ?dup if
    swap %s " depth mismatch of " %s %d %nl
  else
    drop
  endif
;

\ check that stack depth change since "s[" is N
\ use this to do multiple depth checks within an op/method
\ NOTE: there should be a ]s or ]sN somewhere after this
: ]scheck   \ stringAddr N ...
  depth _depths.pop dup _depths.push - 2- -
  ?dup if
    swap %s " depth mismatch of " %s %d %nl
  else
    drop
  endif
;

1024 constant windowHeight
1024 constant windowWidth
windowWidth 2/ 1- constant xMax
windowWidth 2/ negate constant xMin
windowHeight 2/ 1- constant yMax
windowHeight 2/ negate constant yMin

\ begin hexadecimal numbering
hex  \ hex xy : x holds life , y holds neighbors count
\ high nibble is 1 for alive, 0 for dead
\ low nibble is count of living neighbors

$10 constant Alive  \ 0y = not alive

\ Conway's rules:
\ a cell's life depends on the number of its next-door neighbors
\ it dies if it has fewer than 2 neighbors
\ it dies if it has more than 3 neighbors
\ it is born if it has exactly 3 neighbors

\ back to decimal
decimal
int cellRowsOffset
int cellColumnsOffset

2 int zoomFactor!

: drawGrid      \ dim cellColor --
  \ draw 4 lines around central LifeGrid
  sc
  int dim!
  cellColumnsOffset zoomFactor * int px!
  cellRowsOffset zoomFactor * int py!
  px dim zoomFactor * + int zx!
  py dim zoomFactor * + int zy!
  mt(px py)
  dl(px zy)
  dl(zx zy)
  dl(zx py)
  dl(px py)
;

: displayCell   \ x y cellColor --
  sc
  cellRowsOffset - zoomFactor * int py!
  cellColumnsOffset -  zoomFactor * int px!
  \ if(within(px xMin xMax)) andif(within(py yMin yMax))
  \ px . py . %nl
    mt(px py) fsq(zoomFactor)
  \ endif
  \ px . py .
  \ dp(px py)
;

: displayLiveCell displayCell($FFFFFF) ;

: displayEmptyCell displayCell(0) ;

: clearDisplay
  sc(0)
  cl
   \ clearConsole
;

\ grids are identified by gridId, which is a 32-bit int
\   whose low 16 bits are x, high 16 bits are y

$10 constant Alive

enum: neighborDirs
  nDir neDir eDir seDir sDir swDir wDir nwDir
;enum

: printGridId
  int gridId!
  short ss
  gridId ss! ss %d ':' %c gridId 16 rshift ss! ss %d
;

'#' constant LiveCellChar

\ ======================================================
\
\  ILifeGame - abstract interface to Life game engine
\ 
class: ILifeGame
  cell dim      \ length of a grid edge
  
  m: init ;m        \ gridDimensions ...
  m: update ;m
  m: clear ;m
  m: display ;m
  m: findOrAddGrid ;m     \ gridId ... LifeGrid
  m: addCell ;m       \ x y ...
  m: findNeighbor ;m \ LifeGrid neighborDir ... LifeGrid_of_Neighbor
  \ creates the requested LifeGrid if it doesn't exist
  m: shutdown ;m
  
;class


\ ======================================================
\
\  LifeGrid - a square grid of life cells
\ 
class: LifeGrid
  ILifeGame game
  int gridId
  cell topY
  cell leftX
  cell dim              \ width and height
  \ these 8 variables are grid offsets of corner cells and start of edge rows
  cell nwCorner     \ dim^2 - dim
  cell neCorner     \ dim^2 - 1
  cell seCorner     \ dim - 1
  cell swCorner     \ 0
  cell nEdge    \ leftmost cell in north edge ((dim^2 - dim) + 1)
  cell sEdge    \ leftmost cell in south edge (1)
  cell wEdge    \ topmost cell in west edge (dim^2 - dim)
  cell eEdge    \ topmost cell in east edge (dim^2 - 2)
  
  LifeGrid nNeighbor
  LifeGrid eNeighbor
  LifeGrid sNeighbor
  LifeGrid wNeighbor
  LifeGrid neNeighbor
  LifeGrid nwNeighbor
  LifeGrid seNeighbor
  LifeGrid swNeighbor

  ByteArray grid
  ptrTo byte gridBase
  cell liveCount
  cell oldCount
  cell updatedFromOutside
 
  : mem2xy      \ bytePtr ... x y
    gridBase - cell offset!
    offset dim /mod
    topY@+ swap leftX@+ swap
  ;

  : offset2xy      \ gridOffset ... x y
    dim /mod
    topY@+ swap leftX@+ swap
  ;
  
  m: gridId2xy   \ gridId ... x y (of bottomLeft pixel)
    short ss
    int gridId!
    gridId ss! ss dim *
    gridId 16 rshift ss! ss dim *
  ;m

  : drawLive    \ bytePtr ...
    mem2xy displayLiveCell
  ;

  : drawEmpty   \ bytePtr ...
    mem2xy displayEmptyCell
  ;
  
  \ increment neighbor count of cell in this same LifeGrid (slightly faster than incx)
  : inc  \ gridOffset ...
    gridBase + dup b@ 1+ swap b!
  ;

  \ increment neighbor count of cell in a different LifeGrid
  m: incx  \ gridOffset ...
    gridBase@+ dup b@ 1+ swap b!
    updatedFromOutside++
  ;m
  
  \ only use incxx when debugging
  m: incxx
    cell offset!
    t[ offset offset2xy "inc cell@" %s swap %d ',' %c %d " grid " %s gridId printGridId ]t
    inc(offset)
    t[ gridBase offset@+ b@ " new value:" %s %d %nl ]t
  ;m
  
  m: addCell        \ gridOffset ...
    gridBase@+ dup
    if(b@ 0=)
      Alive swap b!
      liveCount++
    else
      drop
    endif
  ;m
  
  m: init   \ LifeWorld gridId ...
    gridId!    game!
    game.dim dim!
    gridId gridId2xy topY! leftX!
    new ByteArray grid!
    grid.resize(dim dim *)
    grid.base gridBase!
    
    dim dup * 1- neCorner!
    dim dup 1- * nwCorner!
    swCorner~
    dim 1- seCorner!
    nwCorner 1+ nEdge!
    1 sEdge!
    dim wEdge!
    dim 2* 1- eEdge!
  ;m

  m: shutdown
    \ clear all circular references
    game~
    nNeighbor~ eNeighbor~ sNeighbor~ wNeighbor~ 
    nwNeighbor~ neNeighbor~ seNeighbor~ swNeighbor~ 
  ;m
  
  m: delete
    shutdown    \ should be redundant
    grid~
  ;m

  \ touchXX assures that neighbor XX exists and is hooked up
  : touchN
    if(objIsNull(nNeighbor))
      game.findNeighbor(this nDir) nNeighbor!
      this nNeighbor.sNeighbor!
    endif
  ;
  
  : touchNW
    if(objIsNull(nwNeighbor))
      game.findNeighbor(this nwDir) nwNeighbor!
      this nwNeighbor.seNeighbor!
    endif
  ;
  
  : touchNE
    if(objIsNull(neNeighbor))
      game.findNeighbor(this neDir) neNeighbor!
      this neNeighbor.swNeighbor!
    endif
  ;
  
  : touchS
    if(objIsNull(sNeighbor))
      game.findNeighbor(this sDir) sNeighbor!
      this sNeighbor.nNeighbor!
    endif
  ;
  
  : touchSW
    if(objIsNull(swNeighbor))
      game.findNeighbor(this swDir) swNeighbor!
      this swNeighbor.neNeighbor!
    endif
  ;
  
  : touchSE
    if(objIsNull(seNeighbor))
      game.findNeighbor(this seDir) seNeighbor!
      this seNeighbor.nwNeighbor!
    endif
  ;
  
  : touchE
    if(objIsNull(eNeighbor))
      game.findNeighbor(this eDir) eNeighbor!
      this eNeighbor.wNeighbor!
    endif
  ;
  
  : touchW
    if(objIsNull(wNeighbor))
      game.findNeighbor(this wDir) wNeighbor!
      this wNeighbor.eNeighbor!
    endif
  ;
      
  : updateNWCorner
    if(grid.get(nwCorner) Alive and)
     t[ "nw corner live cell@" %s nwCorner offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
     \ update NW neighbor
      touchNW
      nwNeighbor.incx(seCorner)
      
      \ update N neighbor (2)
      touchN
      nNeighbor.incx(swCorner)
      nNeighbor.incx(swCorner 1+)

      \ update W neighbor (2)
      touchW
      wNeighbor.incx(neCorner)
      wNeighbor.incx(neCorner dim@-)
      
      \ update rest
      inc(nEdge)
      inc(nwCorner dim@- dup)
      inc(1+)
    endif
  ;

  : updateNECorner
    if(grid.get(neCorner) Alive and)
     t[ "ne corner live cell@" %s neCorner offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
      \ update NE neighbor
      touchNE
      neNeighbor.incx(swCorner)
      
      \ update N neighbor (2)
      touchN
      nNeighbor.incx(seCorner)
      nNeighbor.incx(seCorner 1-)

      \ update E neighbor (2)
      touchE
      eNeighbor.incx(nwCorner)
      eNeighbor.incx(nwCorner dim@-)
      
      \ update rest
      inc(neCorner 1-)
      inc(nwCorner 1- dup)
      inc(1-)
    endif
  ;
    
 
  : updateSWCorner
    if(grid.get(swCorner) Alive and)
     t[ "sw corner live cell@" %s swCorner offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
      \ update SW neighbor
      touchSW
      swNeighbor.incx(neCorner)
      
      \ update S neighbor (2)
      touchS
      sNeighbor.incx(nwCorner)
      sNeighbor.incx(nwCorner 1+)

      \ update W neighbor (2)
      touchW
      wNeighbor.incx(seCorner)
      wNeighbor.incx(seCorner dim@+)
      
      \ update rest
      inc(sEdge)
      inc(dim)
      inc(dim 1+)
    endif
  ;
  
  : updateSECorner
    if(grid.get(seCorner) Alive and)
     t[ "se corner live cell@" %s seCorner offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
      \ update SE neighbor
      touchSE
      seNeighbor.incx(nwCorner)
      
      \ update S neighbor (2)
      touchS
      sNeighbor.incx(neCorner)
      sNeighbor.incx(neCorner 1-)

      \ update E neighbor (2)
      touchE
      eNeighbor.incx(swCorner)
      eNeighbor.incx(dim)
      
      \ update rest
      inc(seCorner 1-)
      inc(eEdge)
      inc(eEdge 1-)
    endif
  ;

  : updateNEdge
    \ cell offsets are nwCorner+1...neCorner-1
    nEdge cell offset!

    if(objIsNull(nNeighbor))
      \ there is no north neighbor, scan along north edge until we find living cell,
      \ then get north neighbor from game
      begin
      while(offset neCorner <)
        if(gridBase offset@+ b@ Alive and)
          touchN
          break
        endif
        offset++
      repeat
    endif
    
    begin
    while(offset neCorner <)
      if(gridBase offset@+ b@ Alive and)
       t[ "north edge live cell@" %s offset offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
        \ found a live cell on north edge
        \ update N neighbor: off-N,off-N+1,off-N+2
        nNeighbor.incx(offset nEdge@- dup)
        nNeighbor.incx(1+ dup)
        nNeighbor.incx(1+)
        \ update cells in this grid: off-1 off+1, off-dim-1 off-dim off-dim+1
        inc(offset 1-)
        inc(offset 1+)
        inc(offset dim@- 1- dup)
        inc(1+ dup)
        inc(1+)
      endif
      offset++
    repeat
  ;
  
  : updateSEdge
    \ cell offsets are swCorner+1...seCorner-1
    swCorner 1+ cell offset!

    if(objIsNull(sNeighbor))
      \ there is no south neighbor, scan along south edge until we find living cell,
      \ then get south neighbor from game
      begin
      while(offset seCorner <)
        if(gridBase offset@+ b@ Alive and)
          touchS
          break
        endif
        offset++
      repeat
    endif
    
    begin
    while(offset seCorner <)
      if(gridBase offset@+ b@ Alive and)
       t[ "south edge live cell@" %s offset offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
        \ found a live cell on south edge
        \ update S neighbor: NW+off-1,NW+off,NW+off+1
        sNeighbor.incx(nwCorner offset@+ 1- dup)
        sNeighbor.incx(1+ dup)
        sNeighbor.incx(1+)
        \ update cells in this grid: off-1, off+1, dim+off-1,dim+off,dim+1+2
        inc(offset 1-)
        inc(offset 1+)
        inc(dim offset@+ 1- dup)
        inc(1+ dup)
        inc(1+)
      endif
      offset++
    repeat
  ;
  
  : updateEEdge
    \ cell offsets are eEdge...neCorner-dim, step by dim
    eEdge cell offset!

    if(objIsNull(eNeighbor))
      \ there is no east neighbor, scan along east edge until we find living cell,
      \ then get east neighbor from game
      begin
      while(offset neCorner <)
        if(gridBase offset@+ b@ Alive and)
          touchE
          break
        endif
        dim offset!+
      repeat
    endif
    
    begin
    while(offset neCorner <)
      if(gridBase offset@+ b@ Alive and)
       t[ "east edge live cell@" %s offset offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
        \ found a live cell on east edge
        \ update E neighbor: off-2dim+1 off-dim+1 off+dim+1
        
        eNeighbor.incx(offset eEdge@- dup)
        eNeighbor.incx(dim@+ dup)
        eNeighbor.incx(dim@+)
        \ update cells in this grid: off-dim-1,off-dim,off-1,off+dim-1,off+dim
        inc(offset dim@- 1- dup)
        inc(1+)
        inc(offset 1-)
        inc(offset dim@+ 1- dup)
        inc(1+)
      endif
      dim offset!+
    repeat
  ;

  : updateWEdge
    \ cell offsets are swCorner-dim...nwCorner+dim, step by dim
    wEdge cell offset!

    if(objIsNull(wNeighbor))
      \ there is no west neighbor, scan along west edge until we find living cell,
      \ then get west neighbor from game
      begin
      while(offset nwCorner <)
        if(gridBase offset@+ b@ Alive and)
          touchW
          break
        endif
        dim offset!+
      repeat
    endif
    
    begin
    while(offset nwCorner <)
      if(gridBase offset@+ b@ Alive and)
       t[ "west edge live cell@" %s offset offset2xy swap %d ',' %c %d
          " grid " %s gridId printGridId %nl ]t
        \ found a live cell on west edge
        \ update w neighbor: off-1 off+dim-1 off+2dim-1
        wNeighbor.incx(offset 1- dup)
        wNeighbor.incx(dim@+ dup)
        wNeighbor.incx(dim@+)
        \ update cells in this grid: off-dim,off-dim+1,off+1,off+dim,off+dim+1
        inc(offset dim@- dup)
        inc(1+)
        inc(offset 1+)
        inc(offset dim@+ dup)
        inc(1+)
      endif
      dim offset!+
    repeat
  ;
  
  m: updateNeighborCounts
    liveCount oldCount!
    \ if(updatedFromOutside liveCount or)
    \ update core cells
    t[ "updateNeighborCounts core " %s gridId printGridId %nl ]t
    cell offset
#if compileTrace
    \ use slow method when debugging - change incx ops to incxx also
    dim 1+ offset!
    do(dim 2-  0)
      do(dim 2-  0)
        if(gridBase offset@+ b@ Alive and)
          t[ "core live cell@" %s offset offset2xy swap %d ',' %c %d
            " grid " %s gridId printGridId %nl ]t
          \ cell is live, increment count of 8 neighbors
          offset dim@- 1-
          dup     incx
          1+ dup  incx
          1+      incx

          offset 1- incx
          offset 1+ incx

          offset dim@+ 1-
          dup     incx
          1+ dup  incx
          1+      incx
        endif
        offset++
      loop
      2 offset!+
    loop
#else
    dim 1+ gridBase@+ ptrTo byte pCell!
    do(dim 2-  0)
      do(dim 2-  0)
        if(pCell b@ Alive and)
          \ cell is live, increment count of 8 neighbors
          pCell dim@- 1-
          dup     dup b@ 1+ swap b!     \ inline 'inc' op for efficiency
          1+ dup  dup b@ 1+ swap b!
          1+      dup b@ 1+ swap b!

          pCell 1-  dup b@ 1+ swap b!
          pCell 1+  dup b@ 1+ swap b!

          pCell dim@+ 1-
          dup     dup b@ 1+ swap b!
          1+ dup  dup b@ 1+ swap b!
          1+      dup b@ 1+ swap b!
        endif
        pCell++
      loop
      2 pCell!+
    loop
#endif
    \ update boundary cells
    t[ "updateNeighborCounts boundary " %s gridId printGridId %nl ]t
    updateNWCorner
    updateNECorner
    updateSWCorner
    updateSECorner
    updateNEdge
    updateSEdge
    updateEEdge
    updateWEdge
    \ endif
  ;m

  m: updateLivenessAndDisplay
    \ if(updatedFromOutside liveCount or)
    gridBase ptrTo byte pCell!
    gridBase neCorner@+ ptrTo byte pLastCell!
    byte cellVal

    begin
    while(pCell pLastCell <=)
      pCell b@ cellVal!
      if(cellVal Alive and)
        if(cellVal $13 > cellVal $12 < or)
          \ doesn't have 2 or 3 neighbors, overcrowded or lonely, will die
          t[ "killing cell @" %s pCell mem2xy swap %d ',' %c %d " count:" %s cellVal %d %nl ]t
          drawEmpty(pCell)
          liveCount--
          \ 0 cellVal!    cellVal~
          0 pCell b!
        else
          t[ "still living cell @" %s pCell mem2xy swap %d ',' %c %d " count:" %s cellVal %d %nl ]t
          \ Alive cellVal!
          Alive pCell b!
        endif
      else
        if(cellVal 3 =)
          \ has 3 neighbors, will become alive
          drawLive(pCell)
          t[ "birthing cell @" %s pCell mem2xy swap %d ',' %c %d " count:" %s cellVal %d %nl ]t
          \ Alive cellVal!
          Alive pCell b!
          liveCount++
        else
          \ 0 cellVal!    cellVal~
          0 pCell b!
        endif
      endif

      \ cellVal pCell& b@!++
      \ cellVal pCell b! pCell++
      pCell++
    repeat

    t[ "LifeGrid:updateLivenessAndDisplay " %s printGridId(gridId) " live: " %s liveCount %d %nl ]t
    \ endif
    updatedFromOutside~
  ;m
  
  m: display
    gridBase ptrTo byte pCell!
    gridBase neCorner@+ ptrTo byte pLastCell!

    begin
    while(pCell pLastCell <=)
      if(pCell b@ Alive and)
        drawLive(pCell)
      endif
      
      pCell++
    repeat
  ;m
  
  m: clear
    grid.fill(0)
    liveCount~
    oldCount~
    updatedFromOutside~
  ;m
  
;class

: /moddy  \ deals with negative denom better for our needs
  dup >r
  /mod
  if(over 0<)
    1- swap r> + swap
  else
    r> drop
  endif
;

\ ======================================================
\
\  LifeWorld - Life game engine implementation
\ 
class: LifeWorld extends ILifeGame

  IntMap of LifeGrid grids
  cell updateCount

  : xy2grid      \ x y ... offsetInGrid gridId
    cell y!   cell x!
    cell gridId   cell offset
    y dim /moddy $10 lshift gridId! dim * offset!
    x dim /moddy $FFFF and gridId!+
    offset@+ gridId
  ;
  
  m: init  \ gridDimensions ...
    dim!
    new IntMap grids!
  ;m
  
  : addGrid    \ gridId ... newLifeGrid
    int gridId!
    short ss
    mko LifeGrid newGrid
    
    newGrid.init(this gridId)
    t[ "adding grid " %s printGridId(gridId) %bl %bl gridId %x %bl %nl ]t
    grids.set(newGrid gridId)
    newGrid@~
  ;
  
  m: findOrAddGrid   \ gridId ... LifeGrid
    int gridId!
    if(grids.grab(gridId) not)
      addGrid(gridId)
    endif
    t[ "found grid " %s printGridId(gridId) %bl %bl gridId %x %bl %nl ]t
  ;m
  
  m: addCell        \ x y ...
    cell y!  cell x!
    xy2grid(x y) int gridId!  int offset!
    t[ "LifeGrid:addCell " %s x %d ',' %c y %d
      " grid:" %s printGridId(gridId) " off:" %s offset %d %nl ]t
    LifeGrid grid
    findOrAddGrid(gridId)  grid!o
    grid.addCell(offset)
  ;m
  
  m: findNeighbor \ LifeGrid neighborDir ... LifeGrid_of_Neighbor
    neighborDirs dir!
    <LifeGrid>.gridId int gridId!
    \ compute global pos of neighbor grid from direction
    case(dir)
      nDir of   0 1     endof
      sDir of   0 -1    endof
      eDir of   1 0     endof
      wDir of   -1 0    endof
      neDir of  1 1     endof
      seDir of  1 -1    endof
      nwDir of  -1 1    endof
      swDir of  -1 -1   endof
      
      "LifeWorld:findNeighbor - unexpected direction " %s
      dup %d %nl
      null exit
    endcase

    ?dup if
      0> if
        $10000 gridId!+
      else
        $10000 gridId!-
      endif
    endif

    ?dup if
      gridId + $FFFF and gridId $FFFF0000 and or gridId!
    endif

    findOrAddGrid(gridId)
  ;m

  m: update
    grids.headIter IntMapIter iter!

    begin
    while(iter.next)
      <LifeGrid>.updateNeighborCounts
    repeat

    iter.seekHead
    beginFrame
    begin
    while(iter.next)
      <LifeGrid>.updateLivenessAndDisplay
    repeat
    endFrame

    updateCount++
    iter~
  ;m
  
  m: display
    \ this is only used to draw the initial frame
    grids.headIter IntMapIter iter!

    beginFrame
    sc(0) cl

    drawGrid(dim $ff00ff)

    begin
    while(iter.next)
      <LifeGrid>.display
    repeat

    endFrame
    
    iter~
  ;m
  
  m: shutdown
    grids.headIter IntMapIter iter!
    begin
    while(iter.next)
      <LifeGrid>.shutdown
    repeat
    iter~
  ;m
  
  m: clear
    grids.headIter IntMapIter iter!
    begin
    while(iter.next)
      <LifeGrid>.clear
    repeat
    updateCount~
    iter~
  ;m
  
  m: delete
    shutdown
    grids~
  ;m
  
  m: printStats
    cell totalLive
    cell numGrids
    cell numEmptyGrids
    LifeGrid grid
    int minX
    int minY
    int maxX
    int maxY
    
    "Stats:\n" %s
    
    grids.headIter IntMapIter iter!
    begin
    while(iter.next)
      grid!o
      if(grid.liveCount)
        grid.liveCount totalLive!+
      else
        numEmptyGrids++
      endif
      numGrids++

      grid.gridId2xy(grid.gridId) int bottomY! int leftX!
      if(leftX minX <)
        leftX minX!
      endif
      if(leftX maxX >)
        leftX maxX!
      endif
      if(bottomY minY <)
        bottomY minY!
      endif
      if(bottomY maxY >)
        bottomY maxY!
      endif

    repeat
    iter~
    
    "Update count: " %s updateCount %d %nl
    "Total live: " %s totalLive %d %nl
    "Number of grids: " %s numGrids %d %nl
    "Number of empty grids: " %s numEmptyGrids %d %nl
    "x range: " %s minX %d %bl maxX %d %nl
    "y range: " %s minY %d %bl maxY %d %nl
   
  ;m
  
;class


\ ======================================================
\
\  LifeShape - a common shape for the life game, such as gliders, pentominos
\ 
class: LifeShape
  String body
  cell width
  cell height

  m: delete    body~    ;m

  m: init   \ bodyString width ...
    new String body!
    width!
    body.set
    body.length width / height!
  ;m

;class


\ ======================================================
\
\  several LifeShape definitions which LifeApp uses
\ 

\ r pentomino
mko LifeShape rPentomino
"\+
.##\+
##.\+
.#." 3 rPentomino.init

\ glider
mko LifeShape glider
"\+
.#.\+
..#\+
###" 3 glider.init

\ herschel - good building block for bigger structures
mko LifeShape herschel
"\+
#..\+
###\+
#.#\+
..#"  3 herschel.init

\ 16x16 methusaleh object from page 32 of "Conways Game of Life"
\          by Nathaniel Johnston & Dave Greene
\ takes 52514 iterations to stabilize
mko LifeShape meth
"\+
.#.##..#####.##.\+
##...###..#...##\+
..#.#.......#..#\+
.....#..#.###..#\+
###...#..##...##\+
....###.#...###.\+
....#..###.....#\+
...#.##.#.....#.\+
#..###....##.###\+
#.#.#..#..#.....\+
#......#..##....\+
.##.#..#....#.#.\+
.#..##.#...#..##\+
..#...#..##....#\+
#.##.#..#.#.....\+
##......#......." 16 meth.init


\ ======================================================
\
\  LifeApp - user interface for LifeWorld
\ 
class: LifeApp
  LifeWorld world
  cell updatesToDo
  bool keepGoing
  cell updateDelayMilliseconds

  m: delete
    world~
  ;m

  : setLiveCell   \ x y ...   make cell at x, y live
    world.addCell
  ;

  : setLiveCells \ lifeShape xOffset yOffset xFlip yFlip ...
    int yFlip!
    int xFlip!
    int yOffset!
    int xOffset!
    LifeShape shape!
  
    shape.body.get ptrTo byte pCell!
    
    do(shape.height 0)
      do(shape.width 0)
        if(pCell b@ LiveCellChar = )
          i j
          if(yFlip) negate endif yOffset + swap
          if(xFlip) negate endif xOffset + swap
          \ i if(xFlip) negate endif xOffset + swap
          \ j if(yFlip) negate endif yOffset + swap
          setLiveCell
        endif
        pCell++
      loop
    loop
    
    shape~
  ;
  
  : onePentominoAtOrigin
    \ this pentomino will be split across 4 LifeGrids
    setLiveCells(rPentomino 0 0 false false)
  ;
  
  : onePentomino
    \ this pentomino should start in the middle of a single LifeGrid
    setLiveCells(rPentomino world.dim 2/ dup false false)
  ;
  
  : manyPentominos
    setLiveCells(rPentomino 0 0 false false)
    
    setLiveCells(rPentomino 100 100 false false)
    setLiveCells(rPentomino -100 100 false true)
    setLiveCells(rPentomino 100 -100 true false)
    setLiveCells(rPentomino -100 -100 true true)
    
    setLiveCells(rPentomino 0 100 false false)
    setLiveCells(rPentomino 0 -100 false true)
    
    setLiveCells(rPentomino 100 0 true false)
    setLiveCells(rPentomino -100 0 true true)
  ;
  
  : gliderE
    setLiveCells(glider 40 40 false false)        \ heading southeast, crosses east first
  ;
  
  : gliderS
    setLiveCells(glider 40 20 true false)         \ heading southwest, crosses south first
  ;
  
  : gliderN
    setLiveCells(glider 20 40 false true)         \ heading northeast, crosses north first
  ;
  
  : gliderW
    setLiveCells(glider 20 20 true true)          \ heading northwest, crosses west first
  ;
  
  : fourGliders
    setLiveCells(glider 40 40 false false)        \ heading southeast, crosses east first
    setLiveCells(glider 40 20 true false)         \ heading southwest, crosses south first
    setLiveCells(glider 20 40 false true)         \ heading northeast, crosses north first
    setLiveCells(glider 20 20 true true)          \ heading northwest, crosses west first
  ;
  
  : fourHerschels
    setLiveCells(herschel 100 100 false false)
    setLiveCells(herschel 100 50 true false)
    setLiveCells(herschel 50 100 false true)
    setLiveCells(herschel 50 50 true true)
  ;
  
  : methuseleh
    setLiveCells(meth 40 40 false false)
  ;
  
  : initLife  \ opToMakeCells ...
    world.clear
    execute
    world.display
  ;
  
  : optionalDelay
    if(updateDelayMilliseconds)
      ms(updateDelayMilliseconds)
    endif
  ;
  
  : handleInput  \ -- char 
     key?  dup
     if
       drop key
     endif
  ;
  
  : redisplay
    world.display
  ;
  
  : showHelp
    "g   run continuous updates\n" %s
    "1-9 run 1-9 updates\n\n" %s
    "a   restart with one r-pentomino at origin\n" %s
    "b   restart with one r-pentomino away from origin\n" %s
    "c   restart with many r-pentominos\n" %s
    "d   restart with four gliders\n" %s
    "e   restart with methuseleh object\n" %s
    "f   restart with four herschels\n\n" %s
    "w   restart with one glider heading northeast\n" %s
    "x   restart with one glider heading southeast\n" %s
    "y   restart with one glider heading southwest\n" %s
    "z   restart with one glider heading northwest\n\n" %s
    "s   print statistics\n" %s
    "+   increase zoom factor\n" %s
    "-   decrease zoom factor\n\n" %s
    "h   show this message\n" %s
    "ESCAPE - exit Life\n" %s
  ;
  
  
  : handleCommand
    byte ch!
    case(ch)
      'a'   of   initLife(['] onePentominoAtOrigin) endof
      'b'   of   initLife(['] onePentomino)         endof
      'c'   of   initLife(['] manyPentominos)       endof
      'd'   of   initLife(['] fourGliders)          endof
      'e'   of   initLife(['] methuseleh)           endof
      'f'   of   initLife(['] fourHerschels)        endof
        
      'g'   of   -1 updatesToDo!                    endof
      'h'   of   showHelp                           endof
        
      's'   of   world.printStats                   endof
  
      'w'   of   initLife(['] gliderN)              endof
      'x'   of   initLife(['] gliderE)              endof
      'y'   of   initLife(['] gliderS)              endof
      'z'   of   initLife(['] gliderW)              endof
        
      '+'   of
        if(zoomFactor 16 <)
          zoomFactor++
          redisplay
        endif
      endof
  
      '-'   of
        if(zoomFactor 1 >)
          zoomFactor--
          redisplay
        endif
      endof
   
      '\e'  of
        keepGoing~
        "You can start life up again by executing \'Life\'\n" %s
      endof
        
      if(ch '0' >= ch '9' <= and)
        ch '0' - updatesToDo!
      endif
    endcase
  ;
  
  \ for some unknown reason, doing 'also sdl2' at top of file doesn't work,
  \   if we don't do it here SDL_Event will be undefined - WTF?
  also sdl2

  : runLife
    SDL_Event event
  
    true keepGoing!
  
    begin
    while(keepGoing)
      if(updatesToDo)
        t[ "==============================\n" %s ]t
        world.update
        updatesToDo--
      endif
      
      optionalDelay
  
      \ check for keyboard input when console window has focus
      handleInput byte ch!
      
      \ check for keyboard input when SDL window has focus
      if(ch 0=)
        if(SDL_PollEvent(event))
          if(event.common.type SDL_KEYDOWN =)
            event.key.keysym.sym ch!

            if(ch '=' =) andif(event.key.keysym.mod 1 and)
              '+' ch!       \ plus key in sdl event is equals+shiftModifier
            endif

            \ event.key.keysym.sym %x  %bl event.key.keysym.mod %x %nl
          elseif(event.common.type SDL_QUIT =)
            '\e' ch!        \ user tried to close SDL window, send an escape
          endif
        endif
      endif
      
      if(ch)
        handleCommand(ch)
      endif
  
    repeat
  ;
  
  m: run
    startSDLWithSize(windowWidth windowHeight)
  
    new LifeWorld world!
    world.init(64)
  
    initLife(['] manyPentominos)
  
    showHelp
    runLife
    
    world.shutdown
    world~
    
    endSDL
  ;m

;class


LifeApp gLifeApp

: Life
  if(objIsNull(gLifeApp))
    new LifeApp gLifeApp!
  endif
  
  gLifeApp.run
;

Life

loaddone

==========================================================================
How original game (life.fs) worked

updateNeighborCounts(-1|0|1 cellIndex)
  update neighbor counts in nextGeneration for one cell

updateCell(-1|1 cellIndex)
  update cell itself and neighbor counts in nextGeneration
  called when cell is born or dies (from Is-Born or Dies)

clearLastGeneration
  fills lastGeneration with blanks (not 0!)

setLiveCell(x y)
  sets cell @x,y in lastGeneration to living ($10)

copyNextToLast
  copies nextGeneration to lastGeneration
    
Innocence
  updates neighbor counts in nextGeneration based on life of nextGeneration cells
  assumes that nextGeneration low nibble (neighbor count) is already 0

Paradise
  iterates over lastGeneration
    non-blank cells are set to live in nextGeneration and shown on graphics display
  then Innocence is called to update nextGeneration neighbor counts
  then copyNextToLast is called, so lastGeneration now matches nextGeneration

updateNextGeneration
  iterates over cells in lastGeneration
  checks is-born or will-die rules based on lastGeneration neighbor counts
  and updates nextGeneration neighbor counts and alive settings
  ? what puts neighbor counts back to 0 in nextGeneration
  
main update loop is
  updateNextGeneration
  copyNextToLast

==========================================================================
idea: implement world grid as a set of linked subgrids
o subgrids are the same size
o subgrids are allocated when needed
o subgrids have up to 8 neighboring subgrids
o there are optimized update algorithms for the center area, and 8 edge areas

==========================================================================
seed initial cells
  grid holds alive markers only
  iterate over grid
    display alive cells
loop:
  iterate over grid
    at each alive cell, increment each of its 8 neighbors neighborCount
  iterate over grid
    apply birth/death rules 
      update display if alive state changes
    clear neighborCount


nw  n   ne      nw  n   ne      nw  n   ne  
w       e       w       e       w       e   
sw  s   se      sw  s   se      sw  s   se  
                -----------
nw  n   ne     |nw  n   ne |   nw  n   ne  
w       e      |w   c   e  |   w       e   
sw  s   se     |sw  s   se |   sw  s   se  
                -----------
nw  n   ne      nw  n   ne      nw  n   ne  
w       e       w       e       w       e   
sw  s   se      sw  s   se      sw  s   se  


7  | 0   1   2   3   4   5   6   7  | 0
----------------------------------------
63 | 56  57  58  59  60  61  62  63 | 56
55 | 48  49  50          53  54  55 | 48
47 | 40  41  42          45  46  47 | 40
   |    ..........................  |
23 | 16  17  18          21  22  23 | 16
15 | 8   9   10          13  14  15 | 8
7  | 0   1   2   3   4   5   6   7  | 0
----------------------------------------
63 | 56  57  58  59  60  61  62  63 | 56

NE  63      dim^2 - 1
NW  56      dim*(dim-1)
SW  0       0
SE  7       dim-1
N   57      NW+1
S   1       1
W   8       dim
E   15      2dim-1

NW  56
    NW:7        SE
    N:0,1       SW,SW+1
    W:63,55     NE,NE-dim
    C:48,49,57  NW-dim,NW-dim+1,NW+1
    
NE  63
    NE:0        SW
    N:6,7       SE-1,SE
    E:48,56     NW-dim,NW
    C:54,55,62  NW-2,NW-1,NE-1
    
SW  0
    SW:63       NE
    S:56,57     NW,NW+1
    W:7,15      SE,SE+dim
    C:1,8,9     S,dim,dim+1
    
SE  7
    SE:56       NW
    S:62,63     NE-1,NE
    E:0,8       SW,dim
    C:6,14,15   off-1,E-1,E

N   57...62     N..NE-1
    N:0,1,2     off-N,off-N+1,off-N+2
    C:48,49,50  off-dim-1,off-dim,off-dim+1
      56,58     off-1,off+1
      
S   1..6        S..SE-1
    S:56,57,58  off+NW-1,off+NW,off+NW+1
    C:0,2       off-1,off+1
      8,9,10    dim+off-1,dim+off,dim+1+2
      
E   15..55      E..NW-1  step by dim
    E:0,8,16    off-E,off-E+dim,off-E+2dim
    C:6,7,14    off-dim-1,off-dim,off-1
      22,23     off+dim-1,off+dim
      
W   8..48       W..NW-dim  step by dim
    W:7,15,23   off-1,off+dim-1,off+2dim-1
    C:0,1,9     off-dim,off-dim+1,off+1
      16,17     off+dim,off+dim+1

  .     .     |   .       .       .
  .     .     |   .       .       .
  .     .     |   .       .       .
------------------------------------            
  .     .     |   .       .       .
  .     .     |   .       .       .
  .     .     |   .       .       .


  .     .     |   .       .       .
  .     .     |   0,1     .       .
  .     .     |   .       1,0     .
----------------------------            
  .     -1,-1 |   0,-1    1,-1    .
  .     .     |   .       .       .

-1:0    0:0
-1:-1   0:-1

glider: 0,1   1,0   1,-1  0,-1   -1,-1

