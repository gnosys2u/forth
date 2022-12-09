// ansi forth compatability words

requires forth_internals
requires forth_optype

autoforget compatability
: compatability ;

: countedStringToString
  // src dst
  -> ptrTo byte dst
  count
  -> int numBytes
  -> ptrTo byte src
  move( src dst numBytes )
  0 dst numBytes + c!
;

: stringToCountedString
  // src dst
  over strlen over c!
  // src len 
  1+ swap strcpy
;

: blockToString
  // src count dst
  -> ptrTo byte dst
  -> int numBytes
  -> ptrTo byte src
  move( src dst numBytes )
  0 dst numBytes + c!
;

: stringToBlock
  dup strlen
;

: lowerCaseIt
  dup strlen 0 do
    dup c@
    if( and( dup `A` >= over `Z` <= ) )
      0x20 + over c!
    else
      drop
    endif
    1+
  loop
  drop
;

: find
  257 string symbol
  countedStringToString( dup symbol )
  $find( symbol )
  // countedStr ptrToSymbolEntry
  if( dup )
    nip			// discard original counted string ptr
    @			// fetch opcode from first word of symbol entry value field
    if( opType:isImmediate( opType:getOptype( dup ) ) )
      1   // immediate op
    else
      -1
    endif
  endif
;

: .(
  `)` $word %s
;
precedence .(

: [char]  opType:makeOpcode( opType:litInt blword c@ ) , ; precedence [char]

: _abortQuote
  swap
  if
    error
  else
    drop
  endif
;
: abort" 
  `"` $word state @
  if
    compileStringLiteral
    lit _abortQuote ,
  else
    _abortQuote
  endif
; precedence abort"

: value -> postpone int ;
alias to ->

int __sp
: !csp sp -> __sp ;
: ?csp
  sp __sp <> if
    dstack
    1 error
    "stack mismatch" addErrorText
  endif
;

// the builtin $word op leaves an empty byte below parsed string for us to stuff length into
: word $word dup strlen over 1- c! 1- ;
: parse word count ;

: sliteral
  257 string str
  blockToString( str )
  compileStringLiteral( str )
  postpone stringToBlock
; precedence sliteral

32 constant #locals
true constant locals
true constant locals-ext
: locals|
  257 string varExpression
  ptrTo byte varName
  begin
    blword -> varName
  while( and( varName null <>   strcmp( varName "|" ) ) )
    "-> int " -> varExpression
    varName ->+ varExpression
    $evaluate( varExpression )
  repeat
; precedence locals|

: (local)
  250 string varName
  257 string varExpression
  blockToString( varName )
  "int " -> varExpression
  ->+ varExpression
  $evaluate( varExpression )
; precedence (local)

alias defer op
alias is ->

: off false swap ! ;
: on true swap ! ;

: s>d i2l ;

int _handler

: catch  // ... xt -- ... 0
  _handler  >r
  sp >r
  rp -> _handler
  execute 0
  r> drop
  r> -> _handler
;

: throw  // error -- error
  dup 0= if
    drop exit
  endif
  _handler -> rp
  // RTOS: oldSP oldHandlerRP
  r> swap >r -> sp
  // RTOS: errorCode oldHandlerRP
  r> r> -> _handler
;

: /string
  rot over + -rot
  -
;

alias include lf

: marker
  blword dup $:
  compileStringLiteral
  postpone $forget
  postpone drop
  postpone ;
;

: recurse
  system.getDefinitionsVocab -> Vocabulary vocab
  vocab.getNewestEntry @ ,
  oclear vocab
; precedence recurse

alias at-xy setConsoleCursor
: page clearConsole 0 0 setConsoleCursor ;
// addr count ... addr+count addr
: bounds over + swap ;

: ,"
  `"` $word
  here over strlen 1+ allot
  swap strcpy align
;
 
: refill
  "refill" fillInBuffer null <>
;

: -trailing
  -> int numChars
  -> ptrTo byte pStr
  
  begin
  while( and( numChars 0>   pStr numChars + 1- c@ ` ` <> ) )
    1 ->- numChars
  repeat
    
  pStr numChars
;

alias d>s drop

: [defined] bl word find nip ;
: [undefined] [defined] not ;
precedence [defined]  precedence [undefined]

alias fconstant constant
alias s>f i2f
alias s>d i2l
alias f>d f2l
alias d>f l2f
: fround
  dup f0>= if
    0.5 f+ floor
  else
    0.5 f- fceil
  endif
;

: >float
  float fval
  _TempStringA blockToString
  _TempStringA "%f" ref fval 1 sscanf
  fval swap
;

loaddone

