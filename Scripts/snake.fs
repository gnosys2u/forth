autoforget _snake
: _snake ;

// ported from snake program on http://skilldrick.github.io/easyforth/ by Nick Morgan   11/27/2015

40 constant width
40 constant height

width height * arrayOf int snakeXHead
width height * arrayOf int snakeYHead
width height * arrayOf byte graphics

int appleX
int appleY

enum: snakeEnums
  0     left
  1     up
  2     right
  3     down
  `#`   wallChar
  `.`   emptyChar
  `@`   appleChar
  `*`   snakeChar
;enum

int direction
int length

: random
  rand swap mod
;

: snakeX       // offset -- address
  ref snakeXHead
;
: snakeY       // offset -- address
  ref snakeYHead
;

: convertXY       // x y -- offset
  width * +
;

int penChar
: setPen -> penChar ;

: draw       // x y --
  -> int y   -> int x
  penChar convertXY(x y) -> graphics
  
  setConsoleCursor(x y)
  penChar %c
;

: drawWalls
  setPen(wallChar)
  do(width 0)
    draw(i 0) 
    draw(i height 1-)
  loop 
  do(height 0)
    draw(0 i)
    draw(width 1- i)
  loop
;

: initializeSnake 
  4 -> length 
  do(length 1+ 0) 
    12 i - i snakeX !
    12 i snakeY !
  loop
  right -> direction
;

: setApplePosition -> appleX -> appleY ;

: initializeApple  setApplePosition(4 4) ;

: initialize
  setPen(emptyChar)
  do(width 0)
    do(height 0)
      draw(j i) 
    loop 
  loop 
  drawWalls 
  initializeSnake 
  initializeApple
;

: moveUp  -1 ->+ snakeYHead(0) ;
: moveLeft  -1 ->+ snakeXHead(0) ;
: moveDown  1 ->+ snakeYHead(0) ;
: moveRight  1 ->+ snakeXHead(0) ;

: moveSnakeHead
  case(direction)
    of(left)    moveLeft   endof
    of(up)      moveUp     endof
    of(right)   moveRight  endof
    of(down)    moveDown   endof
  endcase
;

// Move each segment of the snake forward by one
: moveSnakeTail
  do(0 length)
    i snakeX @ i 1+ snakeX !
    i snakeY @ i 1+ snakeY !
  +loop(-1)
;

: isHorizontal 
  direction dup 
  left = swap 
  right = or
;

: isVertical
  direction dup 
  up = swap 
  down = or
;

: turnUp     isHorizontal if up -> direction endif ;
: turnLeft   isVertical if left -> direction endif ;
: turnDown   isHorizontal if down -> direction endif ;
: turnRight  isVertical if right -> direction endif ;

false -> int gameOver

: changeDirection       // key -- 
  case
    of(`a`) turnLeft   endof
    of(`w`) turnUp     endof
    of(`d`) turnRight  endof
    of(`s`) turnDown   endof
    of(0x1b) true -> gameOver endof
  endcase
;

: check-input 
  if(key?)
    key changeDirection 
  endif
;

// get random x or y position within playable area
: randomPosition       // -- pos 
  4- random 2+
;

: moveApple
  setPen(appleChar)
  appleX appleY draw
  width randomPosition height randomPosition 
  setApplePosition
;

: growSnake  1 ->+ length ;

: checkApple 
  if( and(snakeXHead(0) appleX =   snakeYHead(0) appleY =) )
    moveApple 
    growSnake 
  endif
;

: checkCollision       // -- flag 
  // get current x/y position 
  snakeXHead(0) snakeYHead(0) 
 
  // get color at current position 
  convertXY graphics
 
  // leave boolean flag on stack 
  or( dup wallChar = swap snakeChar =)
;

: drawSnake 
  setPen(snakeChar)
  do(length 0)
    draw(i snakeX @ i snakeY @)
  loop 
  setPen(emptyChar)
  draw(length snakeX @  length snakeY @)
;

: drawApple 
  setPen(appleChar)
  draw(appleX appleY)
;

: gameLoop       // -- 
  false -> gameOver
  begin 
    drawSnake 
    drawApple 
    ms(100)
    check-input 
    moveSnakeTail 
    moveSnakeHead 
    checkApple 
  until(or(checkCollision gameOver))
  setConsoleCursor(0 height 1+)
  "Game Over, Final Lenghth: " %s length %d %nl
;

: snake
  initialize
  gameLoop
;

"snake      begins a game of snake\n" %s
"wasd       movement controls\n" %s
"<ESCAPE>   quits game\n" %s

loaddone

