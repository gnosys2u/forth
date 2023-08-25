setTrace(0)

"forth_autoload" $forget drop

only forth definitions decimal

: forth_autoload "This is the forth_autoload.txt tools module" %s ;

\ create the system singleton
makeObject System system

#if FORTH64
: cells 8* ;
: cell+ 8+ ;
: cell- 8- ;
: cell/ 8/ ;
8 iconstant cellsize
: p@ l@ ;
1 63 llshift lconstant CELL_HIGHEST_BIT
#else
: cells 4* ;
: cell+ 4+ ;
: cell- 4- ;
: cell/ 4/ ;
4 iconstant cellsize
: p@ i@ ;
1 31 ilshift iconstant CELL_HIGHEST_BIT
#endif

struct: dcell
  cell hi
  cell lo
;struct

\ initialize the system object
: _initSystemObject
  new StringMap -> system.namedObjects

  makeObject StringMap env

  int varNum
  ptrTo byte envName
  ptrTo byte envValue
  begin
    getEnvironmentPair(varNum)
    varNum++
    envValue!
    envName!
  while(envName)
    makeObject String varValue
    varValue.set(envValue)
    env.set(varValue envName)
    varValue~
  repeat
  
  env system.env!
  env~
  
  makeObject Array args
  argc int numArgs!
  ?do(numArgs 0)
    makeObject String arg
    arg.set(argv(i))
    args.push(arg)
    \ "adding argument " %s arg.get %s %nl
    oclear arg
  loop
  args system.args!
  args~
;

_initSystemObject
$forget("_initSystemObject") drop


$20 iconstant bl

: $word false $getToken ;
: getToken $getToken dup strlen over 1- b! 1- ;

\  gets next token from input stream, returns:
\  DELIM ... false                 if token is empty or end of line
\  DELIM ... TOKEN_PTR true        if token found
: word?
  if($word dup strlen)
    true
  else
    drop false
  endif
;
: blword? word?(bl) ;

\ the builtin $word op leaves an empty byte below parsed string for us to stuff length into
: word false getToken ;
: parse word count ;

: require bl word count required ;

: $alias
  ptrTo byte oldSymbol!
  ptrTo byte newSymbol!
  
  system.getDefinitionsVocab  Vocabulary dstVocab!
  system.getSearchStack  SearchStack sstack!
  sstack.find(oldSymbol)  Vocabulary foundVocab!   ptrTo int oldEntry!
  if(oldEntry) andif(foundVocab)
    \ need to copy value, since addSymbol below could cause vocabulary memory to be realloced at a different address
    makeObject ByteArray valueBuffer
    valueBuffer.fromMemory(oldEntry foundVocab.getValueLength 0)
    valueBuffer.base -> oldEntry
    
    dstVocab.addSymbol(newSymbol 0 0 0)
    dstVocab.getNewestEntry -> ptrTo int newEntry
    move(valueBuffer.base  newEntry  foundVocab.getValueLength)
    \ newSymbol %s %nl dump(valueBuffer.base 32)
    \ newSymbol %s %nl dump(newEntry 32)
    valueBuffer~
    foundVocab~
  else
    "Symbol " %s oldSymbol %s " not found!\n" %s
  endif
  dstVocab~
;

: alias
  blword 250 string aa!
  aa blword $alias
;

alias ->o ->+    \ ->o stores to an object variable without changing refcounts
alias mko makeObject
alias bool byte
alias objIsNull 0=
alias objNotNull 0<>
alias ds dstack
alias .s dstack
alias sf@ ui@       \ 32-bit sfloat fetch is same as unsigned 32-bit int
alias builds create

$7FFFFFFF int MAXINT!
$80000000 int MININT!
$FFFFFFFF int MAXUINT!
$7FFFFFFFFFFFFFFFL long MAXLONG!
$8000000000000000L long MINLONG!
$FFFFFFFFFFFFFFFFL long MAXULONG!

: ialigned 3+ -4 and ;
: laligned 7+ -8 and ;

#if FORTH64
alias constant lconstant
alias Cell Long
alias CellArray LongArray
MAXULONG cell MAXCELL!
MINLONG  cell MINCELL!
MAXULONG cell MAXUCELL!
alias rshift lrshift
alias lshift llshift
alias rotate lrotate

: depth s0 sp - 3 rshift 1- ;

alias , l,
: 2@ dup cell+ @ swap @ ;
: 2! dup >r ! r> cell+ ! ;

alias @@++ l@@++
alias @!++ l@!++

#else
alias constant iconstant
alias Cell Int
alias CellArray IntArray
MAXINT  cell MAXCELL!
MININT  cell MINCELL!
MAXUINT cell MAXUCELL!
alias rshift irshift
alias lshift ilshift
alias rotate irotate

: depth s0 sp - 2 rshift 1- ;

alias , i,
alias 2@ l@
alias 2! l!

alias @@++ i@@++
alias @!++ i@!++

#endif

\ even on 64-bit platforms we only do 32-bit alignment
alias align ialign
alias aligned ialigned

\ ######################################################################
\  optypes vocabulary ops

system.setDefinitionsVocab(system.getVocabByName("optypes"))

\ OPTYPE8 OPVALUE24 --- OPCODE32
: makeOpcode
  $FFFFFF and
  swap 24 lshift
  or
;

\ OPCODE32 --- OPTYPE
: getOptype
  24 rshift $FF and
;

\ OPTYPE --- TRUE/FALSE
: isImmediate
  cell result
  case
    optypes:NativeImmediate of
    optypes:UserDefImmediate of
    optypes:CCodeImmediate of
    optypes:RelativeDefImmediate of
      result--
    endof
  endcase
  
  result
;

only forth definitions
\ ######################################################################

: ' blword $' ;

\ "using: STRUCT_NAME" will allow you to reference structure member offsets with MEMBER_NAME#
\ you can stack multiple structures if needed
: using:
  blword system.addUsingVocabulary
;

\ remove all structures added with "using:" from search order
: ;using
  system.clearUsingVocabularies
;

: lf
  makeObject String srcName
  srcName.set(blword)
  if($load?(srcName.get) not)
    makeObject String altName
    altName.set(srcName.get)
    altName.append(".fs")
    if($load?(altName.get) not)
      altName.set(srcName.get)
      altName.append(".txt")
      if($load?(altName.get) not)
        addErrorText("Can't find ")
        error(srcName.get)
      endif
    endif
  endif
  srcName~
;
    

: forget
  blword dup
  if( not( $forget ) )
    "forget failed for " %s %s %nl
  else
    drop
  endif
;
: autoforget blword $forget drop ;

: see verbose describe ;

: %nc      \ NUM CHAR ...      prints CHAR NUM times
  over 0> if
    swap 0 do dup %c loop drop
  else
    drop drop
  endif
;

: spaces bl %nc ;

: autoload      \ autoload OPNAME SCRIPTFILE        load SCRIPTFILE if OPNAME undefined
  if( blword $find  0= )
    $load( blword )
  else
    blword drop
  endif
;

: sizeOf blword $sizeOf processConstant ; precedence sizeOf
: offsetOf blword $offsetOf processConstant ; precedence offsetOf
: $: pushToken : ;

: $,
  \ TOS is ptr to nul terminated string
  here over strlen 1+ allot
  swap strcpy
;

: $constant create $, ialign ;

: _buildConstantArray
  -> int numStrings
  \ compile number of strings
  numStrings ,
  \ create array of pointers to strings
  here ptrTo int string0!
  numStrings cells allot
  \ compile strings and stuff ptrs to them into array
  ?do(numStrings 0) 
    here swap $, 
    \ TODO - 64bit
    numStrings 1- i - string0 int[] !
  loop
;

\ N_STRING_PTRS N $constantArray ...
: $constantArray
  builds
    _buildConstantArray
  does
    \ TODO - 64bit
    int[] cell+ @
;

alias c@ ub@
alias c! b!
alias c, b,

: 2, , , ;
: 2constant create 2, does 2@ ;

#if(FORTH64)
alias land and
alias lor or
alias lxor xor
alias ucell ulong
alias l* *
alias l= =
alias 2rotate rotate
alias l+ +
alias l0<> 0<>
alias l= =
alias l<> <>
alias l> >
alias l>= >=
alias l< <
alias l<= <=
alias l0= 0=
alias l0< 0<
alias l0<= 0<=
alias lwithin within
alias lmin min
alias lmax max
alias cmp lcmp
alias ucmp ulcmp
alias lit llit
#else
: land rot and >r and r> ;
: lor rot or >r and r> ;
: lxor rot xor >r and r> ;
alias ucell uint
alias cmp icmp
alias ucmp uicmp
alias lit ilit
#endif

: time&date splitTime(time) ;

\ ######################################################################
\ features support

\ values to be used with features variable
enum: eFeatures
  $0001  kFFMultiCharacterLiterals
  $0002  kFFCStringLiterals
  $0004  kFFIgnoreCase
  $0008  kFFParenIsExpression
  $0010  kFFAllowContinuations
  $0020  kFFAllowVaropSuffix
  $0040  kFFAnsiControlOps
  $0080  kFFFloatingPointStack
  
  \ kFFAnsi and kFFRegular are the most common feature combinations
  kFFIgnoreCase kFFAnsiControlOps + kFFFloatingPointStack + kFFAnsi
  
  kFFMultiCharacterLiterals kFFCStringLiterals + kFFParenIsExpression +
    kFFAllowContinuations + kFFAllowVaropSuffix +           kFFRegular
    
;enum

: setAnsiMode  kFFAnsi setFeatures ;
: setNonAnsiMode  kFFRegular setFeatures ;

: showFeatures
  getFeatures
  "$" %s dup %x
  dup kFFMultiCharacterLiterals and if " kFFMultiCharacterLiterals" %s endif
  dup kFFCStringLiterals and if " kFFCStringLiterals" %s endif
  dup kFFIgnoreCase and if " kFFIgnoreCase" %s endif
  dup kFFParenIsExpression and if " kFFParenIsExpression" %s endif
  dup kFFAllowContinuations and if " kFFAllowContinuations" %s endif
  kFFAllowVaropSuffix and if " kFFAllowVaropSuffix" %s endif
;

\ ######################################################################
\ help support

255 string _TempStringA
255 string _TempStringB

false int _helpfileLoaded!
mko StringMap _helpMap

: addHelp
  blword ptrTo byte name!
  mko String helpContents
  helpContents.set($word(0))
  _helpMap.set(helpContents name)
  helpContents~
;


: $help
  ptrTo byte opName!
  
  if( strlen( opName ) 0= )
    \ line was empty, just list all help
    _helpMap.headIter StringMapIter iter!
    begin
    while(iter.seekNext iter.currentPair)
      %s %bl <String>.get %s %nl
    repeat
    iter~
  else
    if(_helpMap.grab(opName))
      String opDescription!
      opDescription.get %s %nl
      opDescription~
    else
      opName %s " not found!\n" %s
    endif
  endif
;

: help
  blword _TempStringA!
  if( _helpfileLoaded not )
    $load( "help.fs"  )
    true _helpfileLoaded!
    \ there is a $help at end of help.txt that will complete the lookup
    _TempStringA
  else
    $help( _TempStringA )
  endif
;

addHelp immediate    makes most recently defined word have precedence
: immediate
  system.getDefinitionsVocab Vocabulary vocab!
  vocab.getNewestEntry
  vocab.getValueLength + >r \ top of rstack points to length byte of counted string
  move( r@ 1+ _TempStringA r@ c@ )
  r> c@ dup _TempStringA 4- i!
  0 swap _TempStringA + c!
  precedence(pushToken(_TempStringA))
  vocab~
;

addHelp	dump		ADDRESS LEN dump	dump memory
addHelp	ldump		ADDRESS LEN ldump	dump memory as longs
addHelp fdump	FILENAME OFFSET LEN fdump	dump file contents
addHelp fldump	FILENAME OFFSET LEN fldump	dump file contents as longs

int dumpWidth
16 -> dumpWidth

\ ADDR LEN OFFSET _dump
: _dump
  cell offset!		\ offset is subtracted from the actual data address before display
  cell len!
  ptrTo byte addr!
  int columns
  ptrTo byte endAddr
  int ch

  addr len + endAddr!
  len columns!
  begin
  while( addr endAddr < )
#if FORTH64
    addr offset - "%016llx" format %s "   " %s
#else
    addr offset - "%08x" format %s "   " %s
#endif
    if( len dumpWidth > )
      dumpWidth columns!
    endif
    do( columns 0 )
      addr i + c@ "%02x " format %s
      if( and( i 3 ) 3 = )
        %bl
      endif
    loop
    "    " %s
    \ why do we have to add 1 to dumpWidth here?
    dumpWidth 1+ columns - 0 do %bl %bl %bl loop
    do( columns 0 )
      addr i + c@ ch!
      if( ch bl >  ch 127 < and )
        ch
      else
        '.'
      endif
      %c
    loop
    columns addr!+
    columns len!-
    %nl
  repeat
;

\ ADDR LEN OFFSET _ldump
: _ldump
  int offset!		\ offset is subtracted from the actual data address before display
  and( 3+ invert( 3 ) ) int len!
  and( invert( 3 ) )    cell addr!
  int columns
  int endAddr
  int ch

  addr len + endAddr!
  len columns!
  begin
  while( addr endAddr < )
    addr offset - "%08x" format %s "   " %s
    if( len dumpWidth > )
      dumpWidth columns!
    endif
    do( columns 0 )
      addr i + @ "%08x " format %s
    +loop( 4 )
    "    " %s
    \ why do we have to add 1 to dumpWidth here?
    dumpWidth 1+ columns - 0 do %bl %bl %bl loop
    do( columns 0 )
      addr i + c@ ch!
      if( ch bl >  ch 127 < and )
        ch
      else
        '.'
      endif
      %c
    loop
    columns addr!+
    columns len!-
    %nl
  repeat
;

: dump 0 _dump ;
: ldump 0 _ldump ;


: _fdump
  op dumpOp!
  int len!
  int offset!
  "rb" fopen cell infile!
  cell buff

  if( infile 0= )
    "open failure" %s %nl
    exit
  endif
  if( len 0= )
    \ read entire file
    infile flen len!
  endif
  len malloc buff!
  infile offset 0 fseek
  if( 0 <> )
    "fdump fseek failure" %nl exit
  endif
  buff len 1 infile fread
  if( 0= )
    "fdump read failure" %s %nl
  else
    buff len buff dumpOp
  endif
  infile fclose drop
  buff free
;

: fdump ['] _dump _fdump ;
: fldump ['] _ldump _fdump ;

\ USER_DEFINED_OPCODE COUNT ...     display COUNT ops of user definition (good with defs made by :noname or f:)
: dumpOps
  uint numToDump!
  uint op!
  128 string buff
  op _opAddress
  ptrTo uint codeAddress
  
  ?dup if
    1 = if
      codeAddress!
      op& buff describe@ buff %s %nl %nl
      numToDump 0 do
        codeAddress i@ op!
        codeAddress %x %bl %bl
        4 codeAddress!+
        op& buff describe@ buff %s %nl
      loop
    else
      drop
    endif
  endif
;
addHelp dumpOps USER_DEFINED_OPCODE COUNT dumpOps     display COUNT ops of user definition (good with defs made by :noname or f:)
addHelp addHelp	addHelp SYMBOL REST_OF_LINE		add help entry
addHelp $,	STRING_ADDR $,		compiles string (can leave DP unaligned)
addHelp help	help BLAH		show help for BLAH
addHelp	ds	ds			dump stack

addHelp $hash   STRING_ADDR $hash     return hash of string
: $hash
  strlen( dup ) phHash
;

: iliteral ['] ilit i, i, ;
: lliteral ['] llit i, l, ;
alias sfliteral iliteral
alias fliteral lliteral
: literal ['] lit i, , ;
: 2literal
#if FORTH64
  ['] i128lit i, swap
#else
  ['] llit i,
#endif
  , ,
;
precedence literal   precedence fliteral   precedence 2literal
precedence iliteral  precedence sfliteral  precedence lliteral

: chars ;
: char blword c@ ;

\ given ptr to cstring, compile code which will push ptr to cstring
: compileStringLiteral
  ptrTo byte src!
  src strlen cell len!
  len 4+ $FFFFFFFC and 2 rshift cell lenInts!
  optypes:makeOpcode( optypes:ConstantString lenInts ) i,		\ compile literal string opcode
  \ or($15000000 lenInts) i,		\ compile literal string opcode
  strcpy( here src )
  allot( lenInts 4* )
;

\ given ptr to char block and count, compile code that will push that
: compileBlockLiteral
  cell len!
  ptrTo byte src!
  len 3+ $FFFFFFFC and 2 rshift cell lenInts!
  optypes:makeOpcode( optypes:ConstantString lenInts ) i,		\ compile literal string opcode
  \ or($0f000000 lenInts) i,		\ compile push branch opcode
  cmove( src here len)
  allot( lenInts 4* )
  or($14000000 len) i,		    \ compile lit(len) opcode
;

: ."
  '"' $word
  if( state @ )
    compileStringLiteral
    postpone %s
  else
    %s
  endif
;
precedence ."

: s"
  '"' false $getToken
  if( state @ )
    compileStringLiteral( dup )
    strlen postpone iliteral
  else
    dup strlen
  endif
;
precedence s"

: quit r0 -> rp 0 -> fp done ;
: erase 0 fill ;
: blank bl fill ;
alias emit %c
alias does> does
alias then endif
: char+ 1+ ;
alias cr %nl
alias space %bl
alias synonym alias
\ alias words vlist

: accept    \ buffer bufferLen ... bytesRead     read up to bufferLen bytes from console into buffer
  cell bufferLen!
  ptrTo byte buffer!
  cell bytesRead

  if(fgets(buffer bufferLen stdin))
    strlen(buffer) bytesRead!
    if( bytesRead )
      if( buffer bytesRead@+ 1- c@ '\n' = )
        \ trim trailing linefeed from count
        bytesRead--
      endif
    endif
  endif
  bytesRead
;

variable _#tib
addHelp tib         ... ptrToBuffer     return text input buffer address
: tib source drop ;
addHelp #tib        ... ptrToTibCount    return address of variable with tib valid char count
: #tib source _#tib ! drop _#tib ;

addHelp expect  addr n ...    grab up to n chars from input, storing in buffer at addr
addHelp span    ... spanAddr    return address of variable holding char count of last expect
variable span
: expect accept span ! ;

addHelp emptyInputBuffer
: emptyInputBuffer
  \ change the input state to have readOffset equal to writeOffset
  save-input
  dup if(4 =)
    >r >r >r
    drop dup
    r> r> r> restore-input
  else
    0 do drop loop
    error("emptyInputBuffer - save-input has wrong count")
  endif
;

\ get a line from current input and append to specified string
\ works with any input source (console, file, block)
addHelp consumeInputBuffer stringObj ...    get a line from current input and append to specified string
: consumeInputBuffer
  String lineout!
  \ get line of text from current input, append to lineout
  null fillInputBuffer lineout.append
  lineout~

  \ empty buffer, otherwise the outer interpreter will try to interpret the line we just got
  emptyInputBuffer
;


addHelp $enum       enumInfo enumValue ... str      convert an enum value into its string
: $enum
  cell enumInfo!
  cell enumValue!
  findEnumSymbol(enumValue enumInfo)
  if(0=)
    mko String ss
    ss.resize(256)
    ss.format("unknown_%s_%d" enumInfo 16+ enumValue 2)
    addTempString(ss.get ss.length)
    ss~
  endif
;

addHelp cd	cd BLAH			change working directory to BLAH
: cd
  if( chdir( blword ) )
    "cd failed!\n" %s
  endif
;

\ creates a temporary filename that does not currently exist in current directory
\ the name is very easily predictable, if you need a name that is secure against hacks
\ replace this op with one of your own
: makeTempfileName
  getEnvironmentVar("FORTH_TEMP") -> ptrTo byte tempDir
  if(tempDir 0=)
    "" tempDir!
  endif
  mko String timeStr
  timeStr.resize(256)
  strftime(timeStr.get timeStr.size "forth_%Y%m%d_%H%M%S" time)
  timeStr.fixup
  mko String resultStr
  begin
    resultStr.set(tempDir)
    resultStr.append(timeStr.get)
    resultStr.append(format(rand "_%d"))
    resultStr.append(".tmp")
  until(not(fexists(resultStr.get)))

  addTempString(resultStr.get resultStr.length)

  resultStr~
  timeStr~
;

: $shellRun
  ptrTo byte commandString!
  true int okay!
  null ptrTo byte outName!
  null ptrTo byte errName!
  null cell outStream!
  null cell errStream!
  -1   cell oldStdOut!
  -1   cell oldStdErr!
  0    int bytesRead!
  2048 int readSize!
  malloc( readSize 1+ ) ptrTo byte buffer!
  -1   int result!
  
  \ create temp filenames for stdout and stderr streams
  makeTempfileName outName!
  if( outName 0= )
    error( "$shellRun: failure creating standard out tempfile name" )
    false okay!
  endif
  
  makeTempfileName -> errName
  if( errName 0= )
    error( "$shellRun: failure creating standard error tempfile name" )
    false okay!
  endif
  
  \ open temp file which will take standard output
  if( outName )
    fopen( outName "w" ) outStream!
    if( outStream 0= )
      addErrorText( "$shellRun: failure opening standard out file " )
      error( outName )
      false okay!
    endif
  endif
  
  \ open temp file which will take standard error
  if( errName )
    fopen( errName "w" ) errStream!
    if( errStream 0= )
      addErrorText( "$shellRun: failure opening standard error file " )
      error( errName )
      false okay!
    endif
  endif
  
  \ dup standard output file descriptor so we can restore it on exit
  if( okay )
    _dup( 1 ) oldStdOut!
    if( oldStdOut -1 = )
      error( "$shellRun: failure dup-ing stdout" )
      false okay!
    endif
  endif
    
  \ dup standard error file descriptor so we can restore it on exit
  if( okay )
    _dup( 2 ) oldStdErr!
    if( oldStdErr -1 = )
      error( "$shellRun: failure dup-ing stderr" )
      false okay!
    endif
  endif

  \ 
  \ redirect standard out to temp output file
  \ 
  if( okay )
    if( _dup2( _fileno( outStream ) 1 ) -1 = )
      error( "$shellRun: failure redirecting stdout" )
      false okay!
    else
      \ redirect standard error to temp error file
      if( _dup2( _fileno( errStream ) 2 ) -1 = )
        error( "$shellRun: failure redirecting stderr" )
        false okay!
      else
      
        \ have DOS shell execute command line string pointed to by TOS
        _shellRun( commandString ) result!
        
        fflush( stdout ) drop
        fflush( stderr ) drop
        \ close standard error stream, then reopen it on oldStdError (console output)
        _dup2( oldStdErr 2 ) drop
      endif
      \ close standard output stream, then reopen it on oldStdOut (console output)
      _dup2( oldStdOut 1 ) drop
    endif
    fclose( outStream ) drop
    null outStream!
    fclose( errStream ) drop
    null errStream!
  endif

  \  
  \ dump contents of output and error files using forth console IO routines
  \ 
  if( okay )
    fopen( outName "r" ) outStream!
    if( outStream )
      begin
        fread( buffer 1 readSize outStream ) -> bytesRead
        0 bytesRead buffer byte[] c!
        buffer %s
      until( feof( outStream ) )
      fclose( outStream ) drop
      null -> outStream
    else
      addErrorText( "$shellRun: failure reopening standard output file " )
      error( outName )
      false okay!
    endif
    
    fopen( errName "r" ) -> errStream
    if( errStream )
      begin
        fread( buffer 1 readSize errStream ) bytesRead!
        0 bytesRead buffer byte[] c!
        buffer %s
      until( feof( errStream ) )
      fclose( errStream ) drop
      null errStream!
    else
      addErrorText( "$shellRun: failure reopening standard error file " )
      error( errName )
      false okay!
    endif
  endif

  \ cleanup
  if( outStream )
    fclose( outStream ) drop
  endif
  if( errStream )
    fclose( errStream ) drop
  endif
  if( outName )
	remove( outName ) drop
  endif
  if( errName )
	remove( errName ) drop
  endif
  free( buffer )
  
  result
;	


addHelp pwd	pwd			display working directory
: pwd
#if WINDOWS
  "chdir"
#else
  "pwd"
#endif
  $shellRun drop
;

addHelp ls	ls BLAH		display directory (BLAH is optional filespec)
: ls
  '\n' $word _TempStringA!
#if WINDOWS
  "dir" _TempStringB!
#else
  "ls" _TempStringB!
#endif
  if( strcmp( _TempStringA "" ) )
    \ user specified a directory
    strcat( _TempStringB " " )
    strcat( _TempStringB _TempStringA )
  endif
  $shellRun( _TempStringB ) drop
;

addHelp mv	mv OLDNAME NEWNAME		rename a file
: mv
  blword _TempStringA!
  blword _TempStringB!
  renameFile( _TempStringA _TempStringB )
  if( 0<> )
    addErrorText( "mv: failure renaming file " )
    error( _TempStringA )
  endif
;

addHelp rm	rm FILENAME		remove a file
: rm
  blword _TempStringA!
  remove( _TempStringA )
  if( 0<> )
    addErrorText( "rm: failure removing file " )
    error( _TempStringA )
  endif
;

addHelp more more FILENAME	display FILENAME on screen
: more
  255 string linebuff
  fopen( blword "r" ) cell infile!
  if( infile null = )
    exit
  endif
  false int done!
  int lineCounter
  begin
    fgets( linebuff 250 infile ) int result!
    
    if( result null = )
      true done!
    else
      result %s
      \ pause every 50 lines
      if( lineCounter++@ 50 > )
        0 lineCounter!
        "Hit q to quit, any other key to continue\n" %s
        fgetc( stdin ) dup
        if( 'q' = swap 'Q' = or )
          true done!
        endif
      endif
    endif
    
    if( feof( infile ) )
      true done!
    endif
  done until
  %nl
  infile fclose drop
;

addHelp sys	sys REST_OF_LINE	run rest of line in a DOS shell
: sys $shellRun( 0 $word ) drop ;

addHelp fileExists	"FILEPATH" fileExists ... true/false		tell if a file exists
: fileExists "r" fopen dup if fclose drop true else drop false endif ;

addHelp numLocals	returns number of longwords of local variables in current stack frame
: numlocals fp if fp rp - 4 / 1- else 0 endif ;

: dlocals
  fp if
    rp cell+
    fp cell-
    do
      i @ %x %nl
    cellsize negate +loop
  else
    "no locals defined\n" %s
  endif
;
addHelp dlocals		display the local variables in current stack frame

addHelp demo	 demo is used to both display and evaluate a line of text
: demo source drop %s %nl ;
precedence demo

addHelp list     BLOCK_NUM ...     display specified block
variable scr  0 scr !
: list dup scr ! block 16 0 do dup 64 type %nl 64+ loop drop ;
: lb dup scr ! block 16 0 do i %d '\t' %c '|' %c dup 64 type '|' %c %nl 64+ loop drop ;
: load dup thru ;

enum: threadRunStates
  kFTRSStopped		\ initial state, or after executing stop, needs another thread to Start it
  kFTRSReady		\ ready to continue running
  kFTRSSleeping		\ sleeping until wakeup time is reached
  kFTRSBlocked		\ blocked on a soft lock
  kFTRSExited		\ done running - executed exitThread
;enum

#if WINDOWS

enum:	WIN32_FILE_ATTRIB
  $00001	FATTRIB_READONLY
  $00002	FATTRIB_HIDDEN
  $00004	FATTRIB_SYSTEM
  $00010	FATTRIB_DIRECTORY
  $00020	FATTRIB_ARCHIVE
  $00040	FATTRIB_DEVICE
  $00080	FATTRIB_NORMAL
  $00100	FATTRIB_TEMPORARY
  $00200	FATTRIB_SPARSE
  $00400	FATTRIB_LINK
  $00800	FATTRIB_COMPRESSED
  $01000	FATTRIB_OFFLINE
  $02000	FATTRIB_NOT_INDEXED
  $04000	FATTRIB_ENCRYPTED
;enum

struct: FILETIME
  int	lowDateTime
  int	highDateTime
;struct

decimal

enum:	WIN32_CONSTANTS
  windowsConstants 4+  @ TCHAR_SIZE
  windowsConstants 8+  @ MAX_PATH
  windowsConstants 12+ @ WIN32_FIND_DATA_SIZE
  windowsConstants 16+ @ CRITICAL_SECTION_SIZE
;enum

struct: WIN32_FIND_DATA
  int		attributes
  FILETIME	creationTime
  FILETIME	lastAccessTime
  FILETIME	lastWriteTime
  int		fileSizeHigh
  int		fileSizeLow
  int		reserved0
  int		reserved1
  MAX_PATH arrayOf byte fileName
  14 arrayOf byte shortFileName
;struct

struct: dirent
  int	d_ino
  short	d_reclen
#if FORTH64
  short	d_dummy1
  long	d_namlen
#else
  int	d_namlen
#endif
  int	d_type
  \ for some reason bytes beyond end of dirent struct get overwritten,
  \   add 8 bytes of padding for now
  MAX_PATH 8+ arrayOf byte d_name
;struct

#else

struct: dirent
  int	d_ino
  short	d_reclen
  cell	d_namlen
  int	d_type
  256 arrayOf byte d_name
;struct

#endif

\ these may be OS specific
enum: DIRENT_TYPE_BITS
  $4000 DIRENT_IS_DIR
  $8000 DIRENT_IS_REGULAR
;enum

\ "DIRECTORY_PATH" ... Array of String
: getFilesInDirectory
  cell dirHandle
  dirent entry
  ptrTo byte pName
  new Array Array of String filenames!
  ref dirHandle %x %bl entry %x %nl
  
  opendir dirHandle!
  if( dirHandle )
    begin
    while( readdir(dirHandle entry) )
      entry.d_name(0 ref) pName!
      if(pName c@ '.' <>)
        new String String filename!
        filename.set(pName)
        filenames.push(filename)
        filename~
      endif
    repeat
    closedir(dirHandle) drop
  endif
  unref filenames
;

\ ######################################################################
\ debug and trace statements
\ stuff in d[ ... ]d is only compiled if compileDebug is true
\ stuff in t[ ... ]t is only compiled if compileTrace is true, and is only executed if runTrace is true
\ stuff in assert[ ... ] is only compiled if compileAsserts is true

false int compileDebug!

false int compileTrace!
false int runTrace!

false int compileAsserts!

: d[
  \ remember start of debug section in case we need to uncompile it
  here
;
precedence d[

: ]d
  \ TOS is dp when d[ was executed - start of debug section
  if( compileDebug )
    \ leave debug section as is
    drop
  else
    \ uncompile debug section
    dp !
  endif
;
precedence ]d

: t[
  \ remember start of debug section in case we need to uncompile it
  here
  postpone runTrace postpone if
;
precedence t[

: ]t
  postpone endif
  if( compileTrace )
    \ leave trace section as is
    drop
  else
    \ uncompile trace section
    dp !
  endif
;
precedence ]t


\ usage:   assert[ boolean ]
: _assert
  if( not )
    error( "assertion failure" )
  endif
;

: assert[
  $word( ']' )
  if( compileAsserts )
    $evaluate postpone _assert
  else
    drop
  endif
;
precedence assert[

: oshow
  if( dup 0= )
    "NULLObj" %s drop
  else
    <Object>.show
  endif
;
addHelp oshow   OBJECT oshow ...    like OBJECT.show, but doesn't crash if OBJECT is NULL

: ss system.stats ;

\ ######################################################################
\ floating point constants

$400921fb54442d18L lconstant dpi
$400a934f0979a371L lconstant dlog2ten
$3ff71547652b82feL lconstant dlog2e
$3fd34413509f79ffL lconstant dlog10two
$3fe62e42fefa39efL lconstant dlntwo
$7fffffffffffffffL lconstant dnan

$40490fdb iconstant fpi
$40549a78 iconstant flog2ten
$3fb8aa3b iconstant flog2e
$3e9a209b iconstant flog10two
$3f317218 iconstant flntwo
$7fffffff iconstant fnan

\ ######################################################################
\ automated test support

\ PARAM_VALUE extParam PARAM_TYPE PARAM_NAME
: extParam
  blword ptrTo byte paramType!
  blword ptrTo byte paramName!
  
  if($find(paramName))
    \ already defined, just drop value on TOS
    drop
  else
    mko String paramString
    
    if(strcmp(paramType "string"))
      paramString.appendFormatted("%s %s!" r[ paramType paramName ]r)
    else
      \ param is string, so build: "STRING_LEN string PARAM_NAME!"
      ptrTo byte stringVal!
      stringVal
      paramString.appendFormatted("%d string %s!" r[ strlen(stringVal) paramName ]r)
    endif
    $evaluate(paramString.get)
    
    oclear paramString
  endif
;

\ scripts that may be used in automated testing should check 'testsRunning' and skip printing anything
\  which could have non-predictable output - like running time, clock time or random elements
bool testsRunning

mko Array consoleOutStreamStack

addHelp pushConsoleOut   OUTPUT_STREAM pushConsoleOut ...   save current console output stream and set new output stream
: pushConsoleOut
  consoleOutStreamStack.push(getConsoleOut)
  setConsoleOut
;

addHelp pushFileOut   OUTPUT_FILENAME pushFileOut ...   redirect output stream to named file
: pushFileOut
  ptrTo byte outFileName!
  mko FileOutStream outFile
  
  if(outFile.open(outFileName "w"))
    pushConsoleOut(outFile)
  else
    addErrorText(outFileName)
    error("pushOutputFile: failed to open ")
  endif
  outFile~
;

addHelp popConsoleOut   popConsoleOut ... OLD_CONSOLE_OUTSTREAM      restore previous console output stream
: popConsoleOut
  if(consoleOutStreamStack.count)
    getConsoleOut
    setConsoleOut(consoleOutStreamStack.pop)
  else
    error("popConsoleOutStream - stack is empty")
  endif
;

\ use tr{ ... }tr to quickly redirect all print output to the trace stream (usually an external logger program)
mko TraceOutStream traceOut
: tr{ pushConsoleOut(traceOut) ;
: }tr popConsoleOut drop ;

addHelp outToScreen   restore output to the normal console
: outToScreen getDefaultConsoleOut setConsoleOut ;

\ use err[ ... ]err to quickly redirect all print output to the error stream
\ in particular, use this around any output you want to see in test summary
: err[ pushConsoleOut(getErrorOut) ;
: ]err popConsoleOut drop ;

\ use p[ ... ]p around console output commands to prevent multiple threads from 
\  having garbled output - don't put anything inside p[...]p that could block
\  your thread, or you will get a deadlock
AsyncLock printLock
system.createAsyncLock printLock!
: p[ printLock.grab ;
: ]p printLock.ungrab ;

\ use this version of printlocks if you want to be able to turn it off dynamically:
\ true bool usePrintLocks!
\ : p[ if(usePrintLocks) printLock.grab endif ;
\ : ]p if(usePrintLocks) printLock.ungrab endif ;

\ ######################################################################
addHelp pushContext save the current base, search vocabularies and definitions vocabulary on stack
: pushContext
  system.getSearchStack SearchStack searchStack!o
  do(searchStack.depth 0)
    searchStack.getAt(i)
  loop
  searchStack.depth
  system.getDefinitionsVocab
  base @
  'fctx'
;

addHelp popContext restore the current base, search vocabularies and definitions vocabulary from stack
: popContext
  int numSearchVocabs
  system.getSearchStack SearchStack searchStack!o
  if('fctx' <>)
    error("popContext - wrong stuff on stack")
  else
    base !
    system.setDefinitionsVocab
    numSearchVocabs!
    if(numSearchVocabs 0>)
      searchStack.clear
      do(numSearchVocabs 1)
        searchStack.push
      loop
    endif
  endif
;

: marker
  create
    system.getSearchStack SearchStack searchStack!o
    \ save this op, definitions vocabs, and all search vocabs
    system.getDefinitionsVocab.getNewestEntry ui@ ,
    system.getDefinitionsVocab.getWid ,
    searchStack.depth ,
    do(searchStack.depth 0)
      searchStack.getAt(i).getWid ,
    loop
  does>
    ptrTo cell pData!
    Vocabulary vocab
    system.getSearchStack SearchStack searchStack!o
    searchStack.clear
    
    pData& @@++ forgetOp
    pData& @@++ system.getVocabByWid vocab!o

    if(vocab)
      system.setDefinitionsVocab(vocab)
    else
      error("bad definitions wordlist in marker")
    endif

    pData& @@++ 0 ?do
      pData& @@++ system.getVocabByWid vocab!o
      if(vocab)
        searchStack.push(vocab)
      else
        error("bad search wordlist in marker")
      endif
    loop
;


\ ######################################################################
\ flags to use with setTrace command

enum: eLogFlags
  1     kLogStack
  2     kLogOuterInterpreter
  4     kLogInnerInterpreter
  8     kLogShell
  16    kLogStructs
  32    kLogVocabulary
  64    kLogIO
  128   kLogEngine
  256   kLogToFile
  512   kLogToConsole
  1024  kLogCompilation
  2048  kLogProfiler
;enum

\ ######################################################################
\  defs used with Socket

enum: networkEnums
  0     INADDR_ANY
  
  1     SOCK_STREAM     \ stream socket
  2     SOCK_DGRAM      \ datagram socket
  3     SOCK_RAW        \ raw-protocol interface
  
  0     AF_UNSPEC       \ unspecified
  1     AF_UNIX         \ local to host (pipes, portals)
  2     AF_INET         \ internetwork: UDP, TCP, etc.
  23    AF_INET6        \ Internetwork Version 6
  
  4     IPPROTO_IPV4
  6     IPPROTO_TCP
  17    IPPROTO_UDP
  41    IPPROTO_IPV6
;enum

struct: sockaddr_in
    ushort family
    ushort port
    uint  ipv4addr
    long  dummy
;struct

struct: sockaddr
  ushort family
  14 arrayOf ubyte dummy
;struct


\ ######################################################################

: goclient $6701a8c0 client ;
: goserver server ;

: f. %f %bl ;
: g. %g %bl ;
: sf. %sf %bl ;
: sg. %sg %bl ;

\ ######################################################################
\  DLL support

: _addDLLEntry blword addDLLEntry ;

: _dllEntryType
  builds
    ,
  does
    @ _addDLLEntry
;

\ entry points can be defined with:
\ ARGUMENT_COUNT _addDLLEntry ENTRY_POINT_NAME
\ or use the convenience words dll_0 ... dll_15

0 _dllEntryType dll_0
1 _dllEntryType dll_1
2 _dllEntryType dll_2
3 _dllEntryType dll_3
4 _dllEntryType dll_4
5 _dllEntryType dll_5
6 _dllEntryType dll_6
7 _dllEntryType dll_7
8 _dllEntryType dll_8
9 _dllEntryType dll_9
10 _dllEntryType dll_10
11 _dllEntryType dll_11
12 _dllEntryType dll_12
13 _dllEntryType dll_13
14 _dllEntryType dll_14
15 _dllEntryType dll_15


loaddone
