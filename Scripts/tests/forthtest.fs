autoforget forthtest

requires testbase

: forthtest ;

decimal

\ empty stack markers - should still be here after all tests done
$DEADBEEF 123456789

0 -> numFailedTests

\ : showsection " ================== " dup %s swap %s %s %nl ;

testBuff.set( "aba" )
\ turbo 255 setTrace
\ vd checkResult
test[ checkResult( "aba" ) ]
  

\ ==================================

\ test special characters in strings and characters

test[ '\z' 0= ] test[ '\a' 7 = ] test[ '\b' 8 = ] test[ '\t' 9 = ] test[ '\n' $0a = ] test[ '\v' $0b = ]
test[ '\f' $0c = ] test[ '\r' $0d = ] test[ '"' $22 = ] test[ ''' $27 = ] test[ '\\' $5c = ]
test[ '\_' $20 = ] test[ '\q' $22 = ] test[ '\"' $22 = ]

startTest
'\a' %c '\b' %c '\t' %c '\n' %c '\v' %c '\f' %c '\r' %c '"' %c ''' %c  '\' %c outToScreen
test[ checkResult( "\a\b\t\n\v\f\r\"'\\" ) ]

\ verify that character constants work
: testCharacterConstants startTest 'a' %c '\_' %c 'b' %c '\t' %c 'c' %c '\n' %c 'z' %c %nl ;
outToTestBuffer( testBuff2 )
'a' %c '\_' %c 'b' %c '\t' %c 'c' %c '\n' %c 'z' %c %nl
test[ testCharacterConstants checkResult( testBuff2.get ) ]

\ ==================================

\ test control structures

: testIf if "t" addBuff endif ;
test[ startTest "A" addBuff 1 testIf "B" addBuff 0 testIf "C" addBuff checkResult( "AtBC" ) ]

: testIfElse if 't' else 'f' endif ;
test[ 1 testIfElse 't' = ]  test[ 0 testIfElse 'f' = ]

: testBeginUntil
  startTest
  5 begin dup . 1 - dup 0= until drop
;
test[ testBeginUntil checkResult( "5 4 3 2 1 " ) ]

: testBeginWhile
  startTest
  5 begin dup while dup . 1- repeat drop
;
test[ testBeginWhile checkResult( "5 4 3 2 1 " ) ]

: testDoLoop
  startTest
  5 0 do  i . loop
  -5 0 do  i . -1 +loop
  1000 0 do i . leave " burp " %s loop
  1000 0 do i . i 2 = if unloop exit endif loop
  " should never get here" %s
;
test[ testDoLoop checkResult( "0 1 2 3 4 0 -1 -2 -3 -4 -5 0 0 1 2 " ) ]

\ testBuff.get %s '|' %c %nl

: casetest
  case
    0 of "zero" endof
    1 of "one" endof
    2 of "two" endof
    "whatever" swap
  endcase
  addBuff
;

test[ startTest 1 casetest   3 casetest   0 casetest   2 casetest   5 casetest  checkResult( "onewhateverzerotwowhatever" ) ]


\ ==================================

byte gvb   ubyte gvub   short gvs   ushort gvus   int gvi   uint gvui
long gvl   ulong gvul   sfloat gvf   float gvd

: testGlobalVars1
  startTest
  0 -> gvi 7 0 do gvi . 5 ->+ gvi loop
;
test[ testGlobalVars1 checkResult( "0 5 10 15 20 25 30 " ) ]

: testGlobalVars2
  startTest
  -5 -> gvb gvb . 12 ->+ gvb gvb . 3 ->- gvb gvb .
  -5 -> gvub gvub . 12 ->+ gvub gvub . 3 ->- gvub gvub .
  -1100 -> gvs gvs . 1200 ->+ gvs gvs . 1100 ->- gvs gvs .
  -1100 -> gvus gvus . 1200 ->+ gvus gvus . 1100 ->- gvus gvus .
  -100000 -> gvi gvi . 230000 -> gvi gvi . 3000 ->- gvi gvi .
  -100000 -> gvui gvui . 230000 -> gvui gvui . 3000 ->- gvui gvui .
;
#if FORTH64
test[ testGlobalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 4294867296 230000 227000 " ) ]
#else
test[ testGlobalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 -100000 230000 227000 " ) ]
#endif

: testLocalVars1
  int aa
  startTest
  0 -> aa 7 0 do aa . 5 ->+ aa loop
;
test[ testLocalVars1 checkResult( "0 5 10 15 20 25 30 " ) ]

: testLocalVars2
  byte lvb   ubyte lvub   short lvs   ushort lvus   int lvi   uint lvui
  startTest
  -5 -> lvb lvb . 12 ->+ lvb lvb . 3 ->- lvb lvb .
  -5 -> lvub lvub . 12 ->+ lvub lvub . 3 ->- lvub lvub .
  -1100 -> lvs lvs . 1200 ->+ lvs lvs . 1100 ->- lvs lvs .
  -1100 -> lvus lvus . 1200 ->+ lvus lvus . 1100 ->- lvus lvus .
  -100000 -> lvi lvi . 230000 -> lvi lvi . 3000 ->- lvi lvi .
  -100000 -> lvui lvui . 230000 -> lvui lvui . 3000 ->- lvui lvui .
;
#if FORTH64
test[ testLocalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 4294867296 230000 227000 " ) ]
#else
test[ testLocalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 -100000 230000 227000 " ) ]
#endif

: testGlobalVars3
  startTest
  -12345678987654321l -> gvl gvl l. 54321l ->+ gvl gvl l. 22l ->+ gvl gvl l.
  12345678987654321l -> gvul gvul l. 54321l ->- gvul gvul l. 22l ->- gvul gvul l.
;
test[ testGlobalVars3 checkResult( "-12345678987654321 -12345678987600000 -12345678987599978 12345678987654321 12345678987600000 12345678987599978 " ) ]

: testLocalVars3
  startTest
  long lvl   ulong lvul
  -12345678987654321l -> lvl lvl l. 54321l ->+ lvl lvl l. 22l ->+ lvl lvl l.
  12345678987654321l -> lvul lvul l. 54321l ->- lvul lvul l. 22l ->- lvul lvul l.
;
test[ testLocalVars3 checkResult( "-12345678987654321 -12345678987600000 -12345678987599978 12345678987654321 12345678987600000 12345678987599978 " ) ]

: testGlobalVars4
  startTest
  22.5Ef -> gvf gvf sg. -0.125Ef ->+ gvf gvf sg. 0.75Ef ->- gvf gvf sg.
  22.5E -> gvd gvd g. -0.125E ->+ gvd gvd g. 0.75E ->- gvd gvd g.
;
test[ testGlobalVars4 checkResult( "22.5 22.375 21.625 22.5 22.375 21.625 " ) ]

: testLocalVars4
  startTest
  sfloat lvf   float lvd
  22.5Ef -> lvf lvf sg. -0.125Ef ->+ lvf lvf sg. 0.75Ef ->- lvf lvf sg.
  22.5E -> lvd lvd g. -0.125E ->+ lvd lvd g. 0.75E ->- lvd lvd g.
;
test[ testLocalVars4 checkResult( "22.5 22.375 21.625 22.5 22.375 21.625 " ) ]

\ ==================================

\ Test fetch & store
: buffer builds 4* allot does ;
20 buffer tb

$DEADBEEF tb !
test[ startTest tb b@ . tb s@ $ffff and %x checkResult( "-17 beef" ) ]
test[ startTest tb s@ . tb b@ %d checkResult( "-16657 -17" ) ]

\ ==================================

\ Test strings
20 string aaa 20 string bbb
"ajaja" -> aaa "bobobo" -> bbb
test[ strlen( aaa ) 5 = strlen( bbb ) 6 = ]

test[ strcpy( aaa bbb ) strcmp( aaa "bobobo" ) 0= ]
test[ "blahblah" -> aaa  strncpy( aaa bbb 2 ) strcmp( aaa "boahblah" ) 0= ]

test[ "head" -> aaa "tail" -> bbb strcat( aaa bbb ) strcmp( aaa "headtail" ) 0= ]
test[ "head" -> aaa "tail" -> bbb strncat( aaa bbb 3 ) strcmp( aaa "headtai" ) 0= ]

test[ "blahblah" -> aaa  strcmp( strchr( aaa 'l' ) "lahblah" ) 0= ]
test[ "blahblah" -> aaa  strcmp( strrchr( aaa 'l' ) "lah" ) 0= ]
test[ "blahblah" -> aaa  strchr( aaa 'q' ) 0= ]

test[ strcmp( aaa "blahblah" ) 0= ]
test[ strcmp( aaa "BlahblaH" ) 1 = ]
test[ stricmp( aaa "BlahblaH" ) 0= ]
test[ stricmp( aaa "Blahbla" ) 1 = ]

test[ strcmp( strstr( aaa "ahbl" ) "ahblah" ) 0= ]
test[ strstr( aaa "ahab" ) 0= ]
\ strcpy strncpy strlen strcat strncat strchr strrchr strcmp stricmp strstr strtok

\ ==================================

\ Test inline comments
: _fail_ " FAILED! " %s ;
: _ok_ %d ( _fail_ ) ;

\ ==================================

\ Test basic ops - only errors are displayed
decimal

\ loaddone

\ < <= = <> > >= u< u> 0< 0<= 0= 0<> 0> 0>=
test[ 1 5 < -1 -5 < not 7 7 < not ] test[ 22 11 <= not -134 77 <= 99 99 <= ] test[ 11 18 = not -11214 -11214 = ]
test[ 14 24 <> 83 83 <> not ] test[ 1243 987 > 3775 783475 > not 88 88 > not ] test[ 7837 11 >= 27384 273855 >= not 71 71 >= ]
test[ 11 22 u< 241 11 u< not -22 89743 u< not ] test[ -1 -2 u> -43 5009 u> -54345 -54345 u> not ]
test[ -20 0< 0 0< not 20 0< not ] test[ -97 0<= 0 0<= 834 0<= not ] test[ -12 0= not 0 0= 43 0= not ]
test[ -6324127 0<> 0 0<> not 5748 0<> ] test[ -500 0> not 0 0> not 345879 0> ] test[ -23580 0>= not 0 0>= 1235 0>= ]

\ + - * / or and xor not 
test[ 1 5 + 6 = ] test[ 1 -4 + -3 = ] test[ $80000000 -1 + $7fffffff = ] test[ 1 5 - -4 = ]
test[ 1 -4 - 5 = ] test[ $80000000 -1 - $80000001 = ] test[ 3 7 * 21 = ] test[ $101 $11 * $1111 = ]
test[ -5 -7 * 35 = ] test[ 22 7 / 3 = ] test[ 99 11 / 9 = ] test[ -100 7 / -15 = ] test[ -65535 256 / -256 = ]
test[ true false or ] test[ 33 7 or 39 = ] test[ $f0f $0f0 or $fff = ] test[ $1234 $350461 or $351675 = ]
test[ true false and not ] test[ 33 7 and 1 = ] test[ $f0f $0f0 and 0= ] test[ true false xor   33 7 xor 38 = ]
test[ $f0f $0f0 xor $fff = ] test[ $505 $141 xor $444 = ]

\ 2* 4* 8* 2/ 4/ 8/ /mod mod negate
\ test[ -5 -7 u* -12 35 2= ]
test[ 243 2* 486 = ] test[ 243 4* 972 = ] test[ 243 8* 1944 = ] test[ 744 2/ 372 = ]
test[ 744 4/ 186 = ] test[ 744 8/ 93 = ] test[ 4183 23 /mod 20 181 2= ] test[ 193747 39 mod 34 = ] test[ -34 negate 34 = ]

\ sf+ sf- sf* sf/ sf= sf<> sf> sf>= sf< sf<= 
test[ 3.5Ef 4.25Ef sf+ 7.75Ef sf= ] test[ 8.5Ef 3.25Ef sf- 5.25Ef sf= ] test[ 3.5Ef 4.5Ef sf* 63.0Ef 4.0Ef sf/ sf= ] test[ 5.0Ef 4.0Ef sf<> ]
test[ 27.3Ef 22.2Ef sf> ] test[ 27.3Ef 27.3Ef sf> not ] test[ 27.3Ef 27.3Ef sf>= ] test[ 7.2Ef 121.9Ef sf< ] test[ 676.0Ef 676.0Ef sf< not ]
test[ 676.0Ef 676.0Ef sf<= ]

\ sf0= sf0<> sf0> sf0>= sf0< sf0<= sfwithin sfmin sfmax
test[ 0.0Ef sf0= ] test[ 0.7Ef sf0= not ] test[ 0.0Ef sf0> not ] test[ 1.2Ef sf0> ] test[  0.0Ef sf0>= ] test[ -3.3Ef sf0>= not ]
test[ 5.1Ef sf0>= ] test[ 9.4Ef sf0< not ] test[ 0.0Ef sf0<= ] test[  -2.3Ef sf0<= ] test[ -5.0Ef -1.0Ef 1.0Ef sfwithin not ]
test[ 0.5Ef -1.0Ef 1.0Ef sfwithin ] test[ 7.0Ef -1.0Ef 1.0Ef sfwithin not ] test[ 5.0Ef 2.3Ef sfmin 2.3Ef sf= ] test[ -10.0Ef 4.3Ef sfmax 4.3Ef sf= ]
   
\ f+ f- f* f/ f= f<> f> f>= f< f<= 
test[ 3.5E 4.25E f+ 7.75E f= ] test[ 8.5E 3.25E f- 5.25E f= ] test[ 3.5E 4.5E f* 63.0E 4.0E f/ f= ] test[ 5.0E 4.0E f<> ]
test[ 27.3E 22.2E f> ] test[ 27.3E 27.3E f> not ] test[ 27.3E 27.3E f>= ] test[ 7.2E 121.9E f< ] test[ 676.0E 676.0E f< not ]
test[ 676.0E 676.0E f<= ]

\ f0= f0<> f0> f0>= f0< f0<= fwithin fmin fmax
test[ 0.0E f0= ] test[ 0.7E f0= not ] test[ 0.0E f0> not ] test[ 1.2E f0> ] test[ 0.0E f0>= ] test[ -3.3E f0>= not ]
test[ 5.1E f0>= ] test[ 9.4E f0< not ] test[ 0.0E f0<= ] test[  -2.3E f0<= ] test[ -5.0E -1.0E 1.0E fwithin not ]
test[ 0.5E -1.0E 1.0E fwithin ] test[ 7.0E -1.0E 1.0E fwithin not ] test[ 5.0E 2.3E fmin 2.3E f= ]
test[ -10.0E 4.3E fmax 4.3E f= ]
   
\ lshift arshift rshift rotate
test[ 1 8 lshift 256 = ] test[ 17 2 lshift 68 = ] test[ -80 3 arshift -10 = ] test[ 19 2 arshift 4 = ]
test[ $F1234567 4 rshift $F123456 = ]
#if(FORTH64)
test[ $FFFFFFFFF1234567 4 arshift $FFFFFFFFFF123456 = ]
test[ $12345678 8 rotate $1234567800 = ] test[ $12345678 -8 rotate $7800000000123456 = ]
#else
test[ $F1234567 4 arshift $FF123456 = ]
test[ $12345678 8 rotate $34567812 = ]
#endif
test[ $1234567876543210L 8 llshift $3456787654321000L l= ] test[ $1234567876543210L 8 lrshift $0012345678765432L l= ]
test[ $1234567876543210L 8 lrotate $3456787654321012L l= ] test[ $1234567876543210L -8 lrotate $1012345678765432L l= ]

\ dup ?dup swap over rot pick
test[ 17 5 dup 2 pick tuck 17 = swap 5 = and rot 5 = rot 17 = 0 drop and rot 17 = ]
test[ 87 5 ?dup 0 ?dup 0= rot 5 = rot 5 = and and over 87 = rot 87 = ]

\ -rot nip tuck 
test[ 43 27 88 -rot 27 = -rot 43 = -rot 88 = and and and 1 4 5 nip 5 = swap 1 = and 11 22 tuck 22 = swap 11 = and swap 22 = and ]

test[ sfabs(-56.75Ef) 56.75Ef sf= sfabs(56.75Ef) 56.75Ef sf= ]
test[ fabs(-56.75E) 56.75E f= fabs(56.75e) 56.75E f= ]
test[ sfldexp(0.875Ef 8) 224.0Ef sf= fldexp(-0.875E 8) -224.0E f= ]
test[ sffrexp(-224.0Ef) 8 = swap -0.875Ef sf= ffrexp(224.0E) 8 = >r 0.875E f= r> ]
test[ sfmodf(12345.625Ef) 0.625Ef sf= swap 12345.0Ef sf= fmodf(-12345.625E) -0.625E f= >r -12345.0E f= r> ]
test[ sffmod(-27.375Ef 4.0Ef) -3.375Ef sf= ffmod(27.375E 4.0E) 3.375E f= ]

\ 2dup 2swap 2drop 2over 2rot r[ ]r
\ b! b@ sb@ s! s@ ss@ ! @ 2! 2@ 
\ move fill varAction! varAction@
\ l+ l- l* l/ lmod l/mod lnegate i2l i2sf i2f sf2l sf2i sf2f f2l f2i f2sf l2sf l2f
\ l= l<> l> l>= l< l<= l0= l0> l0>= l0< l0<= lwithin lmin lmax
\ . %d %x %2d %2x %s %c type %bl %nl %f %2f format 2format
\ printDecimalSigned printAllSigned printAllUnsigned octal decimal hex

\ ==================================

\ Test block floating point ops
create srcA
  35.0Ef i, 17.0Ef i, 1.125Ef i, 100.0Ef i,
create srcB
  5.0Ef i, 2.0Ef i, 8.0Ef i, 4.0Ef i,
create dst
  0.0Ef i, 0.0Ef i, 0.0Ef i, 0.0Ef i, 17.0Ef i,

: sfcheck4
  dst 16+ -> ptrTo sfloat pf
  false -> int itWorked
  if(pf sf@ 17.0Ef sf=)
    true -> itWorked
    do(4 0)
      -> sfloat fv
      4 ->- pf
      if(pf sf@ fv sf<>)
        pf sf@ %sg %bl fv %sg %nl
        false -> itWorked
      endif
    loop
  endif
  itWorked
;

test[ sfAddBlock(srcA srcB dst 4)   40.0Ef 19.0Ef 9.125Ef 104.0Ef sfcheck4 ]
test[ sfSubBlock(srcA srcB dst 4)   30.0Ef 15.0Ef -6.875Ef 96.0Ef sfcheck4 ]
test[ sfMulBlock(srcA srcB dst 4)   175.0Ef 34.0Ef 9.0Ef 400.0Ef sfcheck4 ]
test[ sfDivBlock(srcA srcB dst 4)   7.0Ef 8.5Ef 0.140625Ef 25.0Ef sfcheck4 ]
test[ sfScaleBlock(srcA dst 4.0Ef 4)  140.0Ef 68.0Ef 4.5Ef 400.0Ef sfcheck4 ]
test[ sfOffsetBlock(srcA dst 4.0Ef 4) 39.0Ef 21.0Ef 5.125Ef 104.0Ef sfcheck4 ]
test[ sfScaleBlock(srcA dst 0.0Ef 4)  0.0Ef 0.0Ef 0.0Ef 0.0Ef sfcheck4 ]
test[ sfMixBlock(srcA dst 1.0Ef 4)    35.0Ef 17.0Ef 1.125Ef 100.0Ef sfcheck4 ]
test[ sfMixBlock(srcB dst 0.5Ef 4)    37.5Ef 18.0Ef 5.125Ef 102.0Ef sfcheck4 ]

create dsrcA
  35.0E l, 17.0E l, 1.125E l, 100.0E l,
create dsrcB
  5.0E l, 2.0E l, 8.0E l, 4.0E l,
create ddst
  0.0E l, 0.0E l, 0.0E l, 0.0E l, 17.0E l,

: fcheck4
  ddst 32+ -> ptrTo float pd
  false -> int itWorked
  if(pd f@ 17.0E f=)
    true -> itWorked
    do(4 0)
      -> float dv
      8 ->- pd
      if(dv pd f@ f<>)
        pd l@ g. dv g.
        false -> itWorked
      endif
    loop
  endif
  itWorked
;

test[ fAddBlock(dsrcA dsrcB ddst 4)   40.0E 19.0E 9.125E 104.0E  fcheck4 ]
test[ fSubBlock(dsrcA dsrcB ddst 4)   30.0E 15.0E -6.875E 96.0E fcheck4 ]
test[ fMulBlock(dsrcA dsrcB ddst 4)   175.0E 34.0E 9.0E 400.0E fcheck4 ]
test[ fDivBlock(dsrcA dsrcB ddst 4)   7.0E 8.5E 0.140625E 25.0E fcheck4 ]
test[ fScaleBlock(dsrcA ddst 4.0E 4)  140.0E 68.0E 4.5E 400.0E fcheck4 ]
test[ fOffsetBlock(dsrcA ddst 4.0E 4) 39.0E 21.0E 5.125E 104.0E fcheck4 ]
test[ fScaleBlock(dsrcA ddst 0.0E 4)  0.0E 0.0E 0.0E 0.0E fcheck4 ]
test[ fMixBlock(dsrcA ddst 1.0E 4)    35.0E 17.0E 1.125E 100.0E fcheck4 ]
test[ fMixBlock(dsrcB ddst 0.5E 4)    37.5E 18.0E 5.125E 102.0E fcheck4 ]

\ ==================================

\ Test exception handling

int caughtVal   int exNumIn   int finallyVal

: mod7catcher
  int exNum
  try[
    throw(exNumIn)
  ]catch[
    exNum!
    if(mod(exNum 7) 0=)
      7 caughtVal!
    else
      throw(exNum)
    endif
  ]finally[
    finallyVal 1 or finallyVal!
  ]try
;

: mod3catcher
  int exNum
  try[
    mod7catcher
  ]catch[
    exNum!
    if(mod(exNum 3) 0=)
      3 caughtVal!
    else
      throw(exNum)
    endif
  ]finally[
    finallyVal 2 or finallyVal!
   ]try
;

: bystander2
  17 mod3catcher
;

: bystander1
  bystander2
;

: mod11catcher
  exNumIn!
  -1 caughtVal!
  finallyVal~
  int exNum
  try[
    bystander1
  ]catch[
    exNum!
    if(mod(exNum 11) 0=)
      11 caughtVal!
    else
      throw(exNum)
    endif
   ]try
;

test[ mod11catcher(42) 17 = caughtVal 7 = finallyVal 3 = ]
test[ mod11catcher(22) caughtVal 11 = finallyVal 3 = ]
test[ mod11catcher(36) 17 = caughtVal 3 = finallyVal 3 = ]

\ ==================================

\ Test pointer move ops

\ 0123456789abcdef012345 
testBuff2.set( "This is a test string" )
testBuff2.get -> ptrTo byte pSrc
ref pSrc -> ptrTo int src
testBuff.get -> ptrTo cell pDst
ref pDst -> ptrTo cell dst
src l@@++ dst l@!++ 
src b@@++ dst b@!++ 
src b@@++ dst b@!++ 
src s@@++ dst s@!++ 
src i@@++ dst i@!++ 
src s@@++ dst s@!++ 
src b@@++ dst b@!++ 
src b@@++ dst b@!++ 
src s@@++ dst s@!++ 
test[ checkResult( testBuff2.get ) ]

test[ $DEADBEEF 123456789 2= ]	\ check for stack underflow or extra items


\ ==================================
\ test $evaluate

: ee $evaluate( "'e' %c 'y' %c" );
: cc $evaluate( "'c' %c" );
: bb $evaluate( "'b' %c" ) cc $evaluate( "'d' %c" ) ;
: aa $evaluate( "'a' %c" ) bb ee $evaluate( "'z' %c" ) ;

\ test[ strcmp( aa "abcdeyz" ) 0= ]

\ ==================================

\ test fprintf snprintf
64 arrayOf byte testBytes
test[ fprintf(stdout "This line %s %c%c printed %d times\n" "should" 98 101 3 4) 36 =]
test[ snprintf(0 ref testBytes 64 "This %s %s %c%c printed %d times\n"  "line" "should" 98 101 3 5) 36 =]
0 ref testBytes %s
"This line should be printed 3 times\n" %s

\ test sscanf
cell a   cell b   cell c
test[ sscanf("1 b 10 " "%d %c %x " ref a ref b ref c 3) 3 =  c 16 =  b 98 =  a 1 =]

\ test fscanf
ptrTo int tfile
test[ fopen("__test.txt" "w" ) -> tfile   tfile 0<>]
test[ fputs("1 b 10 " tfile ) 0>=]
test[ fclose(tfile) 0=]
test[ fopen("__test.txt" "r" ) -> tfile   tfile 0<>]
test[ fscanf(tfile "%d %c %x " ref a ref b ref c 3) 3 =  c 16 =  b 98 =  a 1 =]
test[ fclose(tfile) 0=]
test[ remove("__test.txt") 0=]


\ ==================================

\ %nl %nl "Hit ENTER to exit" %s
\ stdin fgetc

"forthtest" %s showPassFail

loaddone

