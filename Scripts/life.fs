// Conway's Game of Life, or Occam's Razor Dulled

// The original ANS Forth version by Leo Wong (see bottom) 
//   has been modified slightly to allow it to run under 
//   kForth. Also, delays have been changed from 1000 ms to 
//   100 ms for faster update --- K. Myneni, 12-26-2001

// modified this to work in my forth without requiring ansi compatability mode - Pat McElhatton February 28 2017

requires sdlscreen

// char c-addr -- 
: c+!  dup >r  c@ +  r> c! ;

1024 constant windowHeight
1024 constant windowWidth

windowHeight 2/ 5- constant cellRows
windowWidth 2/ 5- constant cellColumns
//200 constant cellRows
//200 constant cellColumns

cellColumns cellRows *  constant totalCells

// world wrap
: World
  builds
    totalCells  allot
  does
    // u -- c-addr 
    swap totalCells +  totalCells mod   + 
;

World lastGeneration
World nextGeneration

// biostatistics

// begin hexadecimal numbering
hex  // hex xy : x holds life , y holds neighbors count

0x10 constant Alive  // 0y = not alive

// Conway's rules:
// a life depends on the number of its next-door neighbors
// rules take a byte (Alive + numNeighbors) and return a bool

// it dies if it has fewer than 2 neighbors
: lonelyRule    0x12 < ;

// it dies if it has more than 3 neighbors
: crowdedRule    0x13 > ;

: willDieRule  
    dup lonelyRule  swap crowdedRule  or ;

// it is born if it has exactly 3 neighbors
: willGiveBirthRule  
    0x03 = ;

// back to decimal
decimal

// compass points
: N    cellColumns - ;
: S    cellColumns + ;
: E    1+ ;
: W    1- ;

// census
: setCellLife  // -1|1 i -- 
  >r  Alive *  r> nextGeneration c+! ;

: updateNeighborCounts  // -1|0|1 i -- 
  over if
    2dup N W nextGeneration c+!  2dup N nextGeneration c+!  2dup N E nextGeneration c+!
    2dup   W nextGeneration c+!                             2dup   E nextGeneration c+!
    2dup S W nextGeneration c+!  2dup S nextGeneration c+!       S E nextGeneration c+!
//    cellColumns - 1- nextGeneration
//    2dup c+!         1+ 2dup c+!    1+ 2dup c+!  cellColumns +
//    2- 2dup c+!                     2+ 2dup c+!  cellColumns +
//    2- 2dup c+!      1+ 2dup c+!    1+ c+!
 else
    2drop
  endif
;

: updateCell // -1|1 i -- 
   2dup setCellLife  updateNeighborCounts ;

// mortal coils
`?` constant liveCell
bl constant emptyCell

cellRows 2/ constant cellRowsOffset
cellColumns 2/ constant cellColumnsOffset
//: displayLiveCell  cellColumns /mod setConsoleCursor  liveCell %c ;0
//: displayEmptyCell  cellColumns /mod setConsoleCursor  emptyCell %c ;
//: displayLiveCell  cellColumns /mod   sc(0xFFFFFF) dp ;
//: displayEmptyCell  cellColumns /mod sc(0) dp ;
int gpx int gpy
: displayCell
  sc
  cellColumns /mod
  cellRowsOffset - 2* -> int py
  cellColumnsOffset - 2* -> int px
  px -> gpx
  py -> gpy
  mt(px py) fsq(2)
  //px . py .
  dp(px py)
;
: displayLiveCell
  0xFFFFFF
  displayCell
;
: displayEmptyCell
  0
  displayCell
;

// changes, changes
: Is-Born  // i -- 
   dup displayLiveCell
   1 swap updateCell ;
: Dies  // i -- 
   dup displayEmptyCell
   -1 swap updateCell ;

// the one and the many
: cellAddr>cellIndex  // c-addr -- i 
   0 lastGeneration -  ;
   
: updateNextGeneration  
  beginFrame
  0 lastGeneration -> int pNextCell
  totalCells -> int cellsLeft
  begin
  while(cellsLeft)
    pNextCell c@  dup Alive and
    if
      willDieRule
      if
        pNextCell cellAddr>cellIndex Dies
      endif
    else
      willGiveBirthRule
      if
        pNextCell cellAddr>cellIndex Is-Born
      endif
    endif
    1 ->- cellsLeft
    1 ->+ pNextCell
  repeat
  cellColumns 1- cellRows 1- setConsoleCursor
  endFrame
;

// in the beginning
: clearLastGeneration    
   0 lastGeneration  totalCells bl fill ;

: clearDisplay
  sc(0)
  cl
   //clearConsole
;

: setLiveCell   // x y ...   make cell at x, y live
  cellRows 2/ + swap cellColumns 2/ + swap
  cellColumns * + Alive swap lastGeneration c!
;

: setLiveCells // {xyPairs} numXYPairs xOffset yOffset xFlip yFlip ...
  -> int yFlip
  -> int xFlip
  -> int yOffset
  -> int xOffset
  0 do
    if(yFlip) negate endif yOffset + swap
    if(xFlip) negate endif xOffset + swap
    setLiveCell
  loop
;

: rPentomino
  0 0   1 0   -1 1  0 1  0 2   5
;
: setInitialLiveCells  
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

// subtlety
: Serpent  
   //0 2 setConsoleCursor
   "Press a key for knowledge."  %s key drop
   //0 2 setConsoleCursor
   "Press space to re-start, Esc to escape life." %s
;

// the primal state
: Innocence  
   totalCells 0
   do  i nextGeneration c@  Alive /  i updateNeighborCounts  loop ;

// children become parents
: copyNextToLast    0 nextGeneration  0 lastGeneration  totalCells  move ;

// a garden
: Paradise  
  // non-blank elements in lastGeneration become living in nextGeneration
  0 lastGeneration
  beginFrame
  do(totalCells 0)
    dup 1+ swap c@ bl <>
    dup if
      i displayLiveCell
    endif
    Alive and  i nextGeneration c!
  loop  drop
  endFrame
  Serpent
  Innocence copyNextToLast ;

: initLife
  clearLastGeneration
  setInitialLiveCells
  Paradise
;

// the human element

1 constant Ideas
: Dreams    Ideas ms ;

1 constant Images
: Meditation    Images ms ;

// free will
: handleInput  // -- char 
   key? dup 
   if  drop key
       dup bl = if  initLife  endif
   endif ;

// environmental dependence
27 constant Escape

// history
: runLife  
  begin
    updateNextGeneration
    copyNextToLast
    Dreams
    handleInput
    Meditation
  until(Escape =)
;

: Life
  startSDLWithSize(windowWidth windowHeight)
  
  initLife
  runLife
  
  endSDL
;

Life
