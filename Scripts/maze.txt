// a simple maze generator I made to test some ideas for mazes
//  for an abandoned Subatomic Studios game, CandyRunners.

autoforget MAZE
: MAZE ;


1 [if]
ms@ -> int seed
//: random ( -- u ) seed 31241 * 6927 + dup -> seed ;
//: random ( -- u ) seed 0x107465 * 0x234567 + dup -> seed ;
0xff800000 constant ROL9MASK
: rol9 // u1 -- u2 | rotate u1 left by 9 bits
    dup ROL9MASK and 23 rshift swap 9 lshift or ;     
: random // -- u
seed 107465 * 234567 + rol9 dup -> seed ;

[else]
ms@ -> int seed
srand( ms@ )
: random rand ;
[endif]
: choose random swap mod ;

struct: mazeDirs
  int dx
  int dy
  byte ch
;struct


enum: eMazeDirs
  kDirRight
  kDirDown
  kDirLeft
  kDirUp
  kNumDirs
;enum

: char2dir
  case
    `R` of kDirRight endof
    `L` of kDirLeft  endof
    `U` of kDirUp    endof
    `D` of kDirDown  endof
  endcase
;

kNumDirs arrayOf mazeDirs dirs
: initDir
  -> int ix
  ix -> dirs.dy
  ix -> dirs.dx
  ix -> dirs.ch
;

initDir( `R`  1  0 kDirRight )
initDir( `D`  0  1 kDirDown )
initDir( `L`  -1  0 kDirLeft )
initDir( `U`  0  -1 kDirUp )

: reverseDir
  2+ 3 and
;

enum: eMazeTileTypes
  `#` kMTTWall
  `R` kMTTRight
  `L` kMTTLeft
  `U` kMTTUp
  `D` kMTTDown
  `[` kMTTEntrance
  `]` kMTTExit
  `.` kMTTUnused
  `?` kMTTDeadEnd
  ` ` kMTTOpen
;enum

: ?pick 0= if swap endif drop ;
  

// maze sizes do not include the boundary around the maze, so a 10 by 20 maze
// will have valid x indices from 0 to 11 and y indices from 0 to 21

class: mazeArray
  
  ByteArray maze
  int width
  int rowBytes
  int height
  int startX
  int startY
  int exitX
  int exitY
  
  m: delete
    oclear maze
  ;m
  
  m: size
    maze.count
  ;m
  
  m: get
    maze.get( rowBytes * + )
  ;m
  
  m: set
    maze.set( rowBytes * + )
  ;m

  m: ref
    maze.ref( rowBytes * + )
  ;m
   
  // width height startX startY exitX exitY ...
  m: init
    -> exitY
    -> exitX
    -> startY
    -> startX
    -> height
    -> width
    new ByteArray -> maze
    width 2+ -> rowBytes
    maze.resize( rowBytes height 2+ * )
    
    fill( maze.base maze.count kMTTUnused )
    do( rowBytes 0 )
      set( kMTTWall i 0 )
      set( kMTTWall i height 1+ )
    loop
    do( height 1+ 1 )
      set( kMTTWall 0 i )
      set( kMTTWall width 1+ i )
    loop
    set( kMTTEntrance startX 1- startY )
    set( kMTTExit exitX 1+ exitY )
  ;m
  
  m: draw
    maze.base
    do( height 2+ 0 )
      dup width 2+ type %nl
      rowBytes +
    loop
    drop
  ;m
  
;class


class: mazeDisplay

  
  mazeArray maze
  ByteArray mazeText
  int rowBytes
  int numRows

    
  m: delete
    oclear maze
    oclear mazeText
  ;m
  
  : dir2Offset
    case
      kDirRight of 1                endof
      kDirLeft  of -1               endof
      kDirUp    of rowBytes negate  endof
      kDirDown  of rowBytes         endof
    endcase
  ;
  
  : size
    mazeText.count
  ;
  
  : ref
    mazeText.ref( 2* 1- rowBytes * swap 2* + 1- )
  ;
  
  : get
    ref c@
  ;
  
  : set
    ref c!
  ;

  // mazeArray endX endY reachedExit  ...
  m: init
    
    -> int reachedExit
    -> int endY
    -> int endX
    -> maze
    maze.startX -> int x
    maze.startY -> int y
    
    new ByteArray -> mazeText
    maze.width 2* 1+ -> rowBytes
    maze.height 2* 1+ -> numRows
    mazeText.resize( numRows rowBytes * )
    
    fill( mazeText.base mazeText.count kMTTWall )

    kDirRight -> int dir
    begin
    while( or( x endX <> y endY <> ) )
      ref( x y ) -> ptrTo byte pPos
      kMTTOpen pPos c!
      char2dir( maze.get( x y ) ) -> dir
      //x %d %bl y %d %bl maze.get( x y ) %c %bl dir %d %nl
      kMTTOpen pPos dir2Offset( dir ) + c!
      //draw %nl
      x dir dirs.dx + -> x
      y dir dirs.dy + -> y
    repeat
    set( ?pick( kMTTOpen kMTTDeadEnd reachedExit ) endX endY )
    kMTTEntrance ref( maze.startX maze.startY ) 1- c!
    kMTTExit ref( maze.exitX maze.exitY ) 1+ c!
    
  ;m
  
  m: draw
    mazeText.base
    do( numRows 0 )
      dup rowBytes type %nl
      rowBytes +
    loop
    drop
  ;m
  
;class


11 -> int defaultMazeWidth
7 -> int defaultMazeHeight

class: mazeGenerator

  int mazeWidth
  int mazeHeight
  
  int startX
  int startY
  int exitX
  int exitY
  int posX
  int posY
  int direction

  int numMoves
  int numStuck
  int numBacksteps
  
  mazeArray maze
  mazeDisplay disp
  
  m: delete
    oclear maze
    oclear disp
  ;m
  
  m: set
    maze.set
  ;m
  
  m: init
  
    if( mazeWidth 0= mazeHeight 0= or )
      defaultMazeWidth -> mazeWidth
      defaultMazeHeight -> mazeHeight
    endif
    
    1 -> startX
    mazeHeight 2/ -> startY
    
    mazeWidth -> exitX
    mazeHeight 2/ 1+ -> exitY
    
    new mazeArray -> maze
    maze.init( mazeWidth mazeHeight startX startY exitX exitY )

    new mazeDisplay -> disp
  ;m

  : numPossibleMoves
    0
    if( maze.get( posX 1+ posY ) kMTTUnused = )
      1+
    endif
    if( maze.get( posX 1- posY ) kMTTUnused = )
      1+
    endif
    if( maze.get( posX posY 1+ ) kMTTUnused = )
      1+
    endif
    if( maze.get( posX posY 1- ) kMTTUnused = )
      1+
    endif
    //dup %d " possible moves\n" %s
  ;

  : randomNewDirection
    direction 4 choose 1- + 3 and
    //dup %d %bl dup dirs.ch %c " is new dir\n" %s
  ;
  
  m: draw
    maze.draw
  ;m

  : reachedExit
    and( posX exitX = posY exitY = )
  ;

  // posX posY dir ... newX newY
  : move
    tuck dirs.dy + >r
    dirs.dx + r>
  ;

  m: drawBig
    disp.init( maze posX posY reachedExit )
    disp.draw
  ;m

  : findPreviousTile
    if( maze.get( posX posY 1+ ) kMTTUp = )
      posX posY 1+
    else
      if( maze.get( posX posY 1- ) kMTTDown = )
        posX posY 1-
      else
        if( maze.get( posX 1+ posY ) kMTTLeft = )
          posX 1+ posY
        else
          if( maze.get( posX 1- posY ) kMTTRight = )
            posX 1- posY
          else
            "How did I get here?" %s %nl
          endif
        endif
      endif
    endif
  ;
  
  m: gen
    "Using seed " %s seed %d %nl
    init
    0 -> numMoves
    0 -> numStuck
    0 -> numBacksteps
  
    kDirRight -> direction
    startX -> posX
    startY -> posY
    int newDir
    int newX
    int newY
    
    begin
    while( and( numMoves 100 < not( reachedExit)))
      if( numPossibleMoves )
        begin
          randomNewDirection -> newDir
          posX posY newDir move -> newY -> newX
        until( maze.get( newX newY ) kMTTUnused = )
        maze.set( newDir dirs.ch posX posY )
        newDir -> direction
        newX -> posX
        newY -> posY
        //"Moving " %s newDir dirs.ch %c " to " %s newX %d %bl newY %d %nl
      else
        1 if
        //"stuck at " %s posX %d %bl posY %d %nl
        1 ->+ numStuck
        begin
          findPreviousTile -> newY -> newX
          maze.set( kMTTWall posX posY )
          newX -> posX
          newY -> posY
          1 ->+ numBacksteps
          //"back to " %s posX %d %bl posY %d %bl numPossibleMoves %d " moves" %s %nl
        until( numPossibleMoves )
        endif
      endif
      1 ->+ numMoves
    repeat
    
    if( reachedExit )
      maze.set( kMTTRight posX posY )
      numMoves %d " moves  " %s numStuck %d " times stuck   " %s numBacksteps %d " backsteps\n" %s
    else
      "Ran out of moves!\n" %s
    endif

  ;m
  
  m: regen
    -> mazeHeight
    -> mazeWidth
    gen
    drawBig
  ;m
  
;class

new mazeGenerator -> mazeGenerator m
m.gen m.drawBig

requires randoms

// recursive backtracking maze generator
// algorithm from http://weblog.jamisbuck.org/2010/12/27/maze-generation-recursive-backtracking

// may have a bug where it accesses one beyond end of maze storage

class: MazeDirection
  cell wall
  cell oppositeWall
  cell name
  cell dx
  cell dy

  m: init
    -> dy
    -> dx
    -> oppositeWall
    -> wall
    -> name
  ;m
  
;class


class: MazeGen1

  Array of MazeDirection dirs
  ByteArray grid
  int width
  int height
  RandomIntGenerator random
  MazeDirection north
  MazeDirection south
  MazeDirection east
  MazeDirection west
  
  m: delete
    oclear dirs
    oclear grid
    oclear random
    oclear north oclear south oclear east oclear west
  ;m
 
  : addDirection
    mko MazeDirection dir
    dir.init
    dirs.push(dir)
    dir
    oclear dir
  ;
  
  m: init
    -> height
    -> width
    new RandomIntGenerator -> random
    new ByteArray -> grid
    grid.resize(width height *)
    
    
    new Array -> dirs
    `N` 1 2   0 -1 addDirection -> north
    `S` 2 1   0  1 addDirection -> south
    `E` 4 8   1  0 addDirection -> east
    `W` 8 4  -1  0 addDirection -> west
  ;m

  : gridRef grid.ref(width * +) ;

  m: carvePassages recursive
    -> cell cy
    -> cell cx
    gridRef(cx cy) -> ptrTo byte posRef
    
    random.shuffle(dirs)
    dirs.headIter -> ArrayIter iter
    
    begin
    while(iter.next)
      -> MazeDirection dir
      cx dir.dx + -> cell nx
      cy dir.dy + -> cell ny

      if(within(ny 0 height)) andif(within(nx 0 width))
        gridRef(nx ny) -> ptrTo byte nextPosRef
        if(nextPosRef b@ 0=)
          posRef b@ dir.wall or posRef b!
          nextPosRef b@ dir.oppositeWall or nextPosRef b!
          //"carve @" %s cx %d %bl cy %d %bl dir.name %c %nl 
          carvePassages(nx ny)
        endif
      endif
    repeat
    
    oclear iter
  ;m
  
  m: draw
    ptrTo byte posRef
    ` ` %c
    do(width 2* 1- 0)
      `_` %c
    loop
    %nl
    do(height 0)
      `|` %c
      do(width 0)
        gridRef(i j) -> posRef
        if(posRef b@ south.wall and) ` ` else `_` endif %c
        if(posRef b@ east.wall and)
          // isn't this possibly looking one beyond end of maze?
          if(posRef b@ posRef 1+ b@ or south.wall and) ` ` else `_` endif %c
        else
          `|` %c
        endif
      loop
      %nl
    loop
  ;m
  
;class

mko MazeGen1 maze
maze.init(40 30)
maze.carvePassages(0 0)
maze.draw

loaddone

