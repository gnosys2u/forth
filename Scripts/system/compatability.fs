\ ansi forth compatability words

requires forth_internals
requires forth_optype

features

setNonAnsiMode

autoforget compatability
: compatability ;

: countedStringToString
  \ src dst
  ptrTo byte dst!
  count
  int numBytes!
  ptrTo byte src!
  move( src dst numBytes )
  0 dst numBytes + c!
;

: stringToCountedString
  \ src dst
  over strlen over c!
  \ src dst 
  1+ swap strcpy
;

: blockToString
  \ src count dst
  ptrTo byte dst!
  int numBytes!
  ptrTo byte src!
  move( src dst numBytes )
  0 dst numBytes + c!
;

: stringToBlock
  dup strlen
;

: lowerCaseIt
  dup strlen 0 do
    dup c@
    if( and( dup 'A' >= over 'Z' <= ) )
      $20 + over c!
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
  \ countedStr ptrToSymbolEntry
  if( dup )
    nip			\ discard original counted string ptr
    i@			\ fetch opcode from first word of symbol entry value field
    if( opType:isImmediate( opType:getOptype( dup ) ) )
      1   \ immediate op
    else
      -1
    endif
  endif
;

: .(
  ')' $word %s
;
precedence .(

: [char]  opType:makeOpcode( opType:litInt blword c@ ) i, ; precedence [char]

\ stringAddr flag           if flag isn't zero, display the string and abort
: _abortQuote
  \ TOS: cstringAddr flag
  swap
  if
    addErrorText
    -2 throw
  else
    drop
  endif
;

: abort" 
  '"' $word state @
  if
    compileStringLiteral
    ['] _abortQuote i,
  else
    _abortQuote
  endif
; precedence abort"

: value -> postpone cell ;
alias to ->

cell __sp
: !csp sp -> __sp ;
: ?csp
  sp __sp <> if
    dstack
    1 error
    "stack mismatch" addErrorText
  endif
;

\ the builtin $word op leaves an empty byte below parsed string for us to stuff length into
: word $word dup strlen over 1- c! 1- ;
: parse word count ;

: c"
  '"' word ptrTo byte src!
  src ub@ 4+ $FFFFFFFC and 2 rshift cell lenInts!
  or($15000000 lenInts) i,		\ compile literal string opcode
  src here src ub@ 1+ move
  allot( lenInts 4* )
; precedence c"

: sliteral
  257 string str
  blockToString( str )
  compileStringLiteral( str )
  postpone stringToBlock
; precedence sliteral

: s\"
  '\q' $wordEscaped dup 1- ub@
  processEscapeChars
  state @ if
    compileBlockLiteral
  endif
; precedence s\"

: compare
  \ caddrA uLenA caddrB uLenB ... -1/0/1
  rot over cmp ?dup if
    memcmp
  else
    drop drop drop 0
  endif
;

\ optional locals word set

\ I don't think these are actually used anymore
32 constant #locals
true constant locals
true constant locals-ext

\ define a bunch of local vars, like:
\    locals| varA  varB varC |
: locals|
  257 string varExpression
  ptrTo byte varName
  begin
    blword varName!
  while( and( varName null <>   strcmp( varName "|" ) ) )
    "-> cell " varExpression!
    varName varExpression!+
    $evaluate( varExpression )
  repeat
; precedence locals|

\ caddr usize ...
\   adds a local variable, if usize is 0, signals end of local variable definitions
: (local)
  250 string varName
  257 string varExpression
  ?dup if
    blockToString( varName )
    features setNonAnsiMode
    "cell " varExpression!
    varName varExpression!+
    "!" varExpression!+
    $evaluate( varExpression )
    features!
  else
    \ length 0 signals end of locals definitions
    drop
  endif
  
; \ precedence (local)

\ {: initializedA initializedB ... | uninitializedA ... -- outA ... :}
: {:
  128 string token
  257 string varExpression
  0 cell section!
  features setNonAnsiMode
  mko String initializedVars
  
  \ section 0 - each token is an initialized local
  \ section 1 - each token is an uninitialized local
  \ section 2 - each token is an output variable - ignored, just a comment
  bool done
  begin
    blword token!
    \ "token:[" %s token %s "] section " %s section %d cr
    if(token "|" strcmp 0=)
      if(section)
        \ TODO: throw a syntax error
      else
        \ we are entering the uninitialized locals section
        if(initializedVars.length)
          $evaluate(initializedVars.get)
          initializedVars.clear
        endif
        1 section!
      endif
    elseif(token "--" strcmp 0=)
      if(section 1 >)
        \ TODO: throw a syntax error
      else
        \ we are entering the ignored outputs section
        if(initializedVars.length)
          $evaluate(initializedVars.get)
          initializedVars.clear
        endif
        2 section!
      endif
    elseif(token ":}" strcmp 0=)
      if(initializedVars.length)
        $evaluate(initializedVars.get)
      endif
      done++
    else
      case(section)
        of(0)
          \ "add initialized local " %s token %s cr
          \ add initialized local
          "cell " varExpression!
          token varExpression!+
          "! " varExpression!+
          initializedVars.prepend(varExpression)
        endof

        of(1)
          \ "add local " %s token %s cr
          \ add uninitialized local
          "cell " varExpression!
          token varExpression!+
          $evaluate(varExpression)
        endof
        
      endcase

    endif
  until(done)
  
  features!
; precedence {:

: defer@   >body i@   ;

: defer!   >body i!   ;

: defer ( "name" -- )
  create ['] abort ,
  does> i@ execute
;

: is
  state @ if
    postpone ['] postpone defer!
  else
    ' defer!
  endif
; precedence is

: action-of
  state @ if
    postpone ['] postpone defer@
  else
    ' defer@
  endif
; precedence action-of

alias compile, i,

: off false swap ! ;
: on true swap ! ;

: s>d dup 0< ;

: catch
  cell exceptionNum
  op thingToDo!
  try[
    thingToDo
  ]catch[
    exceptionNum!
  ]try
  exceptionNum
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

alias at-xy setConsoleCursor
: page clearConsole 0 0 setConsoleCursor ;
\ addr count ... addr+count addr
: bounds over + swap ;

: ,"
  '"' $word
  here over strlen 1+ allot
  swap strcpy align
;
 
: -trailing
  int numChars!
  ptrTo byte pStr!
  
  begin
  while( and( numChars 0>   pStr numChars + 1- c@ bl <> ) )
    numChars--
  repeat
    
  pStr numChars
;

alias d>s drop

: buffer: create allot ;

: [defined] bl word find nip ;
: [undefined] [defined] not ;
precedence [defined]  precedence [undefined]

: float+ 8+ ;
: floats 8 * ;
alias dfloats floats
alias dfloat+ float+
: sfloat+ 4+ ;
: sfloats 4 * ;

: dfaligned     \ addr ... doubleFloatAlignedAddr
  7 + 7 not and
;

: sfaligned     \ addr ... singleFloatAlignedAddr
  3 + 3 not and
;

alias faligned dfaligned

: dfalign       \ ...       force DP to be double-float aligned
  here dfaligned dp !
;

: sfalign       \ ...       force DP to be single-float aligned
  here sfaligned dp !
;

alias falign dfalign

: begin-structure
  create    here 0 0 ,
  does>     @
;

: +field
  create over , +
  does> @ +
;

: field: aligned 1 cells +field ;
: cfield: 1 chars +field ;
: sffield: sfaligned 1 sfloats +field ;
: dffield: dfaligned 1 dfloats +field ;
alias ffield: dffield:

: end-structure
  swap !
;

: environment?
  mko String ss
  ss.setBytes
  if(ss.equals("CORE"))
  orif(ss.equals("BLOCK"))
  orif(ss.equals("FLOATING"))
  orif(ss.equals("DOUBLE"))
    true true
  else
    false
  endif

  ss~
;

\ optional memory-allocation word set
: allocate   \ usize ... addr
  malloc dup 0<>
;

: resize    \ oldAddr newSize ... newAddr flag
  realloc dup 0<>
;

: free      \ addr ... flag
  free true
;

\ optional file-access word set
1 constant r/o
2 constant w/o
3 constant r/w
: bin 4 or ;

: _access2str
  ucell access!
  if(access bin and)
    access 3 and case
      r/o of  "rb" endof
      w/o of  "wb" endof
      r/w of  "r+b" endof
      null
    endcase
  else
    access case
      r/o of  "r" endof
      w/o of  "w" endof
      r/w of  "r+" endof
      null
    endcase
  endif
;

\ caddr usize access ... fileId ior
: open-file
  cell access!
  mko String filename
  filename.setBytes
  ptrTo byte accessStr
  
  if(access 4 and)
    access 3 and case
      r/o of  "rb" endof
      w/o of  "wb" endof
      r/w of  "r+b" endof
      null
    endcase
  else
    access case
      r/o of  "r" endof
      w/o of  "w" endof
      r/w of  "r+" endof
      null
    endcase
  endif
  accessStr!

  if(accessStr)
    fopen(filename.get accessStr)
    ?dup if
      0     \ success
    else
      0 -1  \ open failure
    endif
  else
    0 -1    \ unrecognized access mode
  endif  
  filename~
;

\ caddr usize access ... fileId ior
: create-file
  cell access!
  mko String filename
  filename.setBytes
  ptrTo byte accessStr
  
  if(access 4 and)
    access 3 and case
      w/o of  "wb" endof
      r/w of  "w+b" endof
      null
    endcase
  else
    access case
      w/o of  "w" endof
      r/w of  "w+" endof
      null
    endcase
  endif
  accessStr!

  if(accessStr)
    fopen(filename.get accessStr)
    ?dup if
      0     \ success
    else
      0 -1  \ open failure
    endif
  else
    0 -1    \ unrecognized access mode
  endif  
  filename~
;

\ fileid   ...   ior
: close-file
  fclose
;

\ caddr usize   ...   ior
: delete-file
  mko String filename
  filename.setBytes
  filename.get
  remove
  filename~
;

\ fileid   ...   udpos ior
: file-position
  cell result
  ftell result!
  result 0>= if
    result s>d 0
  else
    0. -1
  endif
;

\ fileid   ...   udsize
: file-size
  cell result
  flen result!
  result 0>= if
    result s>d 0
  else
    0. -1
  endif
;

\ caddr usize   file-status   status ior       ?is this just for checking existence?
: file-status
  mko String filename
  filename.setBytes
  filename.get fexists if
    0 0
  else
    0 1
  endif
  filename~
;

\ fileid   ...   ior
: flush-file
  fflush
;

\ caddr usize   included
: included
  addTempString $runFile
;
  
\ caddr usize fileid   read-file   uread ior
: read-file
  cell fileId!
  ucell usize!
  ptrTo byte pDst!
  fread(pDst 1 usize fileId) dup usize =
;

\ caddr maxSize fileid   read-line   uread flag ior        (can read maxSize + 2 terminators to caddr)
: read-line
  cell fileId!
  ucell maxSize!
  ucell numRead
  ptrTo byte pDst!
  bool success
  
  \ add a terminating null at very end of buffer 
  0 maxSize 1+ pDst + c!
  \ pDst usize pFile
  pDst maxSize fileId fgets
  if
    pDst strlen numRead!
    numRead if
      numRead 1- pDst + c@
      '\n' = if
        numRead--
      endif
    endif
    success++
  else
    \ got back a null,
    numRead~
    success~
  endif
  
  numRead success
  fileId ferror dup if
    fileId clearerr
  endif
;

\ caddr1 usize1 caddr2 usize2   rename-file   ior
: rename-file
  addTempString >r addTempString r> renameFile
;

\ ud fileid   reposition-file   ior
: reposition-file
  cell fileId!
  drop cell offset!
  
  \ fp offset ctrl
  fseek(fileId offset 0)
;

\ require FILENAME
: require
  blword $runFile
;

\ caddr usize   ...
: required
  mko String filename
  filename.setBytes
  filename.get $runFile
  filename~
;

\ ud fileid   resize-file   ior
: resize-file
#if FORTH64
  \ resizeFile takes a 64-bit long, ignore hiword of desired size
  swap drop resizeFile
#else
  >r
  swap  \ change double-cell to long
  r> resizeFile
#endif
;

\ source-id - set to fileid when reading from file
\ s\" - looks unchanged
\ s" - looks unchanged
\ caddr usize fileid   write-file   ior
: write-file
  cell fileId!
  ucell usize!
  ptrTo byte pSrc!
  fwrite(pSrc 1 usize fileId)
;

\ caddr usize fileid   write-line   ior        (can read usize + 2 terminators to caddr)
: write-line
  cell fileId!
  ucell usize!
  ptrTo byte pSrc!
  '\n' byte newLine!
  fwrite(pSrc 1 usize fileId)
  if(usize =)
    \ write the terminating newline
    fwrite(newLine& 1 1 fileId) 1 <>
  else
    \ initial write failed
    -1
  endif
;

\ ( - allow to span multiple lines when file is input

-> features

loaddone

