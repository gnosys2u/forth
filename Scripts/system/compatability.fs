\ ansi forth compatability words

requires forth_internals

getFeatures

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
  \ countedStr
  257 string symbol
  countedStringToString( dup symbol )
  $find( symbol )
  \ countedStr ptrToSymbolEntry
  if( dup )
    nip			\ discard original counted string ptr
    i@			\ fetch opcode from first  of symbol entry value field
    if( optypes:isImmediate( optypes:getOptype( dup ) ) )
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

: [char]  optypes:makeOpcode( optypes:Constant blword c@ ) i, ; precedence [char]

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

: c"
  '"' false getToken ptrTo byte src!
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
  '\q' false $wordEscaped dup 1- ub@
  processEscapeChars
  state @ if
    compileBlockLiteral
  endif
; precedence s\"

: compare
  \ aStr aLen bStr bLen ... -1/0/1
  ucell bLen!    ptrTo byte bStr!
  ucell aLen!    ptrTo byte aStr!
  cell result

  if(aLen bLen =) andif(memcmp(aStr bStr aLen) 0=)
    \ If the two strings are identical, n is zero. 
    result~
  else
    \ If the two strings are identical up to the length of the shorter string,
    \ n is minus-one (-1) if u1 is less than u2 and one (1) otherwise. 
    aLen bLen min ucell minLen!
    if(memcmp(aStr bStr minLen) 0=)
      if(aLen bLen <)
        result--
      else
        result++
      endif
    else
      \ If the two strings are not identical up to the length of the shorter string,
      \ n is minus-one (-1) if the first non-matching character in the string specified
      \ by c-addr1 u1 has a lesser numeric value than the corresponding character in
      \ the string specified by c-addr2 u2 and one (1) otherwise.
      ucell ix
      begin
      while(ix minLen <)
        aStr ix@+ ub@ bStr ix@+ ub@ -
        ?dup if
          0> break
        endif
        ix++
      repeat
      
      if
        result++
      else
        result--
      endif

    endif
  endif
  \ dump(aStr aLen)
  \ dump(bStr bLen)
  \ "compare: " %s result %d cr
  result
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
    getFeatures
    setNonAnsiMode
    "cell " varExpression!
    varName varExpression!+
    "!" varExpression!+
    $evaluate( varExpression )
    setFeatures
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
  getFeatures
  setNonAnsiMode
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
  
  setFeatures
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
 
: unescape
  \ srcStr srcLen dstStr --- dstStr dstLen
  ptrTo byte dstStr!
  ucell srcLen!    ptrTo byte srcStr!
  dstStr    \ leave for return
  byte bval

  if(srcLen)
    do(srcLen 0)
      srcStr& b@@++ dup dstStr& b@!++
      if('%' =)
        '%' dstStr& b@!++
        srcLen++
      endif
    loop
  endif
  
  srcLen
;

: -trailing
  int numChars!
  ptrTo byte pStr!
  
  begin
  while( and( numChars 0>   pStr numChars + 1- c@ bl = ) )
    numChars--
  repeat
    
  pStr numChars
;

: search
  \ caddr1 u1 caddr2 u2 ... 0 | caddr3 u3 true
  mko String s
  mko String q

  q.setBytes
  ucell srcLen! ptrTo byte srcStr!
  s.setBytes(srcStr srcLen)
  
  strstr(s.get q.get) ptrTo byte foundStr!

  if(foundStr)
    srcStr foundStr s.get - +
    s.get s.length + foundStr -
    true
  else
    srcStr srcLen
    false
  endif
  s~ q~
;

mko StringMap __replaceMap
: replaces
  \ valueAddr valueLen keyAddr keyLen
  mko String key
  key.setBytes
  if(getFeatures kFFIgnoreCase and)
    key.toLower
  endif
  mko String value
  value.setBytes
  
  value key.get __replaceMap.set
  \ "mapping {" %s key.get %s "} to {" %s value.get %s "}\n" %s
  value~
  key~
;

: substitute
  \ srcAddr srcLen dstAddr dstLen --- dstAddr endLen nSubstitutions
  ucell dstLen!
  ptrTo byte dstAddr!
  \ "template\n" %s  2dup dump
  mko String template
  template.setBytes
  
  mko String result
  result.substitute(template __replaceMap) cell nSubs!
  
  dstAddr result.length
  if(result.length dstLen >)
    -1
  else
    cmove(result.get dstAddr result.length)
    nSubs
  endif
  \ >r "result\n" %s 2dup dump r>
;

alias d>s drop

: buffer: create allot ;

: [defined] bl true getToken find nip 0<> ;
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
  elseif(ss.equals("#LOCALS"))
    256 true
\  elseif(ss.equals("FLOATING-STACK"))
\    16 true
  else
    false
  endif

  ss~
;

\ optional memory-allocation word set
: allocate   \ usize ... addr
  malloc dup 0=
;

: resize    \ oldAddr newSize ... newAddr flag
  over >r realloc ?dup if
    r> drop 0
  else
    r> -1
  endif
;

: free      \ addr ... flag
  free 0
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
  
: _getAndClearFileError
  >r 
  r@ ferror
  dup if
    r@ clearerr
  endif
  r> drop
;

\ caddr usize fileid   read-file   uread ior
: read-file
  cell fileId!
  ucell usize!
  ptrTo byte pDst!
  fread(pDst 1 usize fileId) _getAndClearFileError(fileId)
;

\ caddr maxSize fileid   read-line   uread flag ior        (can read maxSize + 2 terminators to caddr)
: read-line
  cell fileId!
  ucell maxSize!
  ptrTo byte pDst!

  ucell numRead
  bool success
  
  \ add a terminating null at very end of buffer 
  0 maxSize 1+ pDst + c!
  maxSize if
    pDst maxSize 1+ fileId fgets
    if
      pDst strlen numRead!
      numRead if
        numRead 1- pDst + c@
        '\n' = if
          numRead--
        endif
      endif
      success--   \ makes it TRUE/-1
    else
      \ fgets returned null
      numRead~
      success~
    endif
  
    numRead success _getAndClearFileError(fileId)

  else
    \ read 0 bytes always returns success
    0 true 0
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

\ ud fileid   resize-file   ior
: resize-file
#if FORTH64
  \ resizeFile takes a 64-bit long, ignore hiword of desired size
  swap drop resizeFile not
#else
  >r
  swap  \ change double-cell to long
  r> resizeFile not
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
  fwrite(pSrc 1 usize fileId) drop _getAndClearFileError(fileId)
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

vocabulary editor

: name>string
  \ standard vocabulary entries have 8 value bytes, then symbol length in a byte, then name
  8+ dup 1+ swap c@
;

: name>interpret
  i@
;

: name>compile
  i@ ucell ntOp!
  optypes:getOptype(ntOp) cell ntOptype!
  
  if(optypes:isImmediate(ntOptype))
      ntOp ['] execute
  else
  
    case(ntOptype)
      optypes:Native of
      optypes:UserDef of
      optypes:CCode of
      optypes:RelativeDef of
        ntOp ['] i,
      endof

      error("name>compile: bad optype")
    endcase

  endif
;

: traverse-wordlist
  ucell wid!
  op traverseOp!
  system.getVocabByWid(wid) Vocabulary vocab!
  if(vocab)
    vocab.headIter VocabularyIter iter!
    
    begin
    while(iter.next)
      traverseOp breakIfNot
    repeat
    
    iter~
    vocab~
  else
    
  endif
;

\ floating point wordset stuff
: nan DFloat:nan ;
: +inf DFloat:+inf ;
: -inf DFloat:-inf ;


\ ( - allow to span multiple lines when file is input

setFeatures

loaddone

