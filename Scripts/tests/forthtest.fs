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

test[ '\0' 0= ] test[ '\a' 7 = ] test[ '\b' 8 = ] test[ '\t' 9 = ] test[ '\n' $0a = ] test[ '\v' $0b = ]
test[ '\f' $0c = ] test[ '\r' $0d = ] test[ '"' $22 = ] test[ ''' $27 = ] test[ '\\' $5c = ]

startTest
'\a' %c '\b' %c '\t' %c '\n' %c '\v' %c '\f' %c '\r' %c '"' %c ''' %c  '\' %c outToScreen
test[ checkResult( "\a\b\t\n\v\f\r\"'\\" ) ]

\ verify that character constants work
: testCharacterConstants startTest 'a' %c '\s' %c 'b' %c '\t' %c 'c' %c '\n' %c 'z' %c %nl ;
outToTestBuffer( testBuff2 )
'a' %c '\s' %c 'b' %c '\t' %c 'c' %c '\n' %c 'z' %c %nl
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
long gvl   ulong gvul   float gvf   double gvd

: testGlobalVars1
  startTest
  0 -> gvi 7 0 do gvi %d %bl 5 ->+ gvi loop
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
  0 -> aa 7 0 do aa %d %bl 5 ->+ aa loop
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
  22.5 -> gvf gvf sg. -0.125 ->+ gvf gvf sg. 0.75 ->- gvf gvf sg.
  22.5l -> gvd gvd g. -0.125l ->+ gvd gvd g. 0.75l ->- gvd gvd g.
;
test[ testGlobalVars4 checkResult( "22.5 22.375 21.625 22.5 22.375 21.625 " ) ]

: testLocalVars4
  startTest
  float lvf   double lvd
  22.5 -> lvf lvf sg. -0.125 ->+ lvf lvf sg. 0.75 ->- lvf lvf sg.
  22.5l -> lvd lvd g. -0.125l ->+ lvd lvd g. 0.75l ->- lvd lvd g.
;
test[ testLocalVars4 checkResult( "22.5 22.375 21.625 22.5 22.375 21.625 " ) ]

\ ==================================

\ Test fetch & store
: buffer builds 4* allot does ;
20 buffer tb

$DEADBEEF tb !
test[ startTest tb b@ %d %bl tb s@ $ffff and %x checkResult( "-17 beef" ) ]
test[ startTest tb s@ %d %bl tb b@ %d checkResult( "-16657 -17" ) ]

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
test[ -5 -7 * 35 = ] test[ 22 7 / 3 = ] test[ 99 11 / 9 = ] test[ -100 7 / -14 = ] test[ -65535 256 / -255 = ]
test[ true false or ] test[ 33 7 or 39 = ] test[ $f0f $0f0 or $fff = ] test[ $1234 $350461 or $351675 = ]
test[ true false and not ] test[ 33 7 and 1 = ] test[ $f0f $0f0 and 0= ] test[ true false xor   33 7 xor 38 = ]
test[ $f0f $0f0 xor $fff = ] test[ $505 $141 xor $444 = ]

\ 2* 4* 8* 2/ 4/ 8/ /mod mod negate
\ test[ -5 -7 u* -12 35 2= ]
test[ 243 2* 486 = ] test[ 243 4* 972 = ] test[ 243 8* 1944 = ] test[ 744 2/ 372 = ]
test[ 744 4/ 186 = ] test[ 744 8/ 93 = ] test[ 4183 23 /mod 20 181 2= ] test[ 193747 39 mod 34 = ] test[ -34 negate 34 = ]

\ sf+ sf- sf* sf/ sf= sf<> sf> sf>= sf< sf<= 
test[ 3.5f 4.25f sf+ 7.75f sf= ] test[ 8.5f 3.25f sf- 5.25f sf= ] test[ 3.5f 4.5f sf* 63.0f 4.0f sf/ sf= ] test[ 5.0f 4.0f sf<> ]
test[ 27.3f 22.2f sf> ] test[ 27.3f 27.3f sf> not ] test[ 27.3f 27.3f sf>= ] test[ 7.2f 121.9f sf< ] test[ 676.0f 676.0f sf< not ]
test[ 676.0f 676.0f sf<= ]

\ sf0= sf0<> sf0> sf0>= sf0< sf0<= sfwithin sfmin sfmax
test[ 0.0f sf0= ] test[ 0.7f sf0= not ] test[ 0.0f sf0> not ] test[ 1.2f sf0> ] test[  0.0f sf0>= ] test[ -3.3f sf0>= not ]
test[ 5.1f sf0>= ] test[ 9.4f sf0< not ] test[ 0.0f sf0<= ] test[  -2.3f sf0<= ] test[ -5.0f -1.0f 1.0f sfwithin not ]
test[ 0.5f -1.0f 1.0f sfwithin ] test[ 7.0f -1.0f 1.0f sfwithin not ] test[ 5.0f 2.3f sfmin 2.3f sf= ] test[ -10.0f 4.3f sfmax 4.3f sf= ]
   
\ f+ f- f* f/ f= f<> f> f>= f< f<= 
test[ 3.5d 4.25d f+ 7.75d f= ] test[ 8.5d 3.25d f- 5.25d f= ] test[ 3.5d 4.5d f* 63.0d 4.0d f/ f= ] test[ 5.0d 4.0d f<> ]
test[ 27.3d 22.2d f> ] test[ 27.3d 27.3d f> not ] test[ 27.3d 27.3d f>= ] test[ 7.2d 121.9d f< ] test[ 676.0d 676.0d f< not ]
test[ 676.0d 676.0d f<= ]

\ f0= f0<> f0> f0>= f0< f0<= fwithin fmin fmax
test[ 0.0d f0= ] test[ 0.7d f0= not ] test[ 0.0d f0> not ] test[ 1.2d f0> ] test[ 0.0d f0>= ] test[ -3.3d f0>= not ]
test[ 5.1d f0>= ] test[ 9.4d f0< not ] test[ 0.0d f0<= ] test[  -2.3d f0<= ] test[ -5.0d -1.0d 1.0d fwithin not ]
test[ 0.5d -1.0d 1.0d fwithin ] test[ 7.0d -1.0d 1.0d fwithin not ] test[ 5.0d 2.3d fmin 2.3d f= ]
test[ -10.0d 4.3d fmax 4.3d f= ]
   
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

test[ sfabs(-56.75) 56.75 sf= sfabs(56.75) 56.75 sf= ]
test[ fabs(-56.75l) 56.75L f= fabs(56.75l) 56.75L f= ]
test[ sfldexp(0.875 8) 224.0f sf= fldexp(-0.875l 8) -224.0L f= ]
test[ sffrexp(-224.0) 8 = swap -0.875 sf= ffrexp(224.0l) 8 = >r 0.875L f= r> ]
test[ sfmodf(12345.625) 0.625 sf= swap 12345.0f sf= fmodf(-12345.625l) -0.625L f= >r -12345.0L f= r> ]
test[ sffmod(-27.375 4.0) -3.375 sf= ffmod(27.375l 4.0l) 3.375L f= ]

\ 2dup 2swap 2drop 2over 2rot r[ ]r
\ b! b@ sb@ s! s@ ss@ ! @ 2! 2@ 
\ move fill varAction! varAction@
\ l+ l- l* l/ lmod l/mod lnegate i2l i2f i2d f2l f2i f2d d2l d2i d2f l2f l2d
\ l= l<> l> l>= l< l<= l0= l0> l0>= l0< l0<= lwithin lmin lmax
\ . %d %x %2d %2x %s %c type %bl %nl %f %2f format 2format
\ printDecimalSigned printAllSigned printAllUnsigned octal decimal hex

\ ==================================

\ Test block floating point ops
create srcA
  35.0f i, 17.0f i, 1.125f i, 100.0f i,
create srcB
  5.0f i, 2.0f i, 8.0f i, 4.0f i,
create dst
  0.0f i, 0.0f i, 0.0f i, 0.0f i, 17.0f i,

: sfcheck4
  dst 16+ -> ptrTo float pf
  false -> int itWorked
  if(pf sf@ 17.0f sf=)
    true -> itWorked
    do(4 0)
      -> float fv
      4 ->- pf
      if(pf sf@ fv sf<>)
        pf sf@ %sg %bl fv %sg %nl
        false -> itWorked
      endif
    loop
  endif
  itWorked
;

test[ sfAddBlock(srcA srcB dst 4)   40.0f 19.0f 9.125f 104.0f sfcheck4 ]
test[ sfSubBlock(srcA srcB dst 4)   30.0f 15.0f -6.875f 96.0f sfcheck4 ]
test[ sfMulBlock(srcA srcB dst 4)   175.0f 34.0f 9.0f 400.0f sfcheck4 ]
test[ sfDivBlock(srcA srcB dst 4)   7.0f 8.5f 0.140625f 25.0f sfcheck4 ]
test[ sfScaleBlock(srcA dst 4.0f 4)  140.0f 68.0f 4.5f 400.0f sfcheck4 ]
test[ sfOffsetBlock(srcA dst 4.0f 4) 39.0f 21.0f 5.125f 104.0f sfcheck4 ]
test[ sfScaleBlock(srcA dst 0.0f 4)  0.0f 0.0f 0.0f 0.0f sfcheck4 ]
test[ sfMixBlock(srcA dst 1.0f 4)    35.0f 17.0f 1.125f 100.0f sfcheck4 ]
test[ sfMixBlock(srcB dst 0.5 4)    37.5 18.0f 5.125f 102.0f sfcheck4 ]

create dsrcA
  35.0d l, 17.0d l, 1.125d l, 100.0d l,
create dsrcB
  5.0d l, 2.0d l, 8.0d l, 4.0d l,
create ddst
  0.0d l, 0.0d l, 0.0d l, 0.0d l, 17.0d l,

: fcheck4
  ddst 32+ -> ptrTo double pd
  false -> int itWorked
  if(pd f@ 17.0L f=)
    true -> itWorked
    do(4 0)
      -> double dv
      8 ->- pd
      if(dv pd f@ f<>)
        pd l@ g. dv g.
        false -> itWorked
      endif
    loop
  endif
  itWorked
;

test[ fAddBlock(dsrcA dsrcB ddst 4)   40.0L 19.0L 9.125L 104.0L  fcheck4 ]
test[ fSubBlock(dsrcA dsrcB ddst 4)   30.0L 15.0L -6.875L 96.0L fcheck4 ]
test[ fMulBlock(dsrcA dsrcB ddst 4)   175.0L 34.0L 9.0L 400.0L fcheck4 ]
test[ fDivBlock(dsrcA dsrcB ddst 4)   7.0L 8.5L 0.140625L 25.0L fcheck4 ]
test[ fScaleBlock(dsrcA ddst 4.0d 4)  140.0L 68.0L 4.5L 400.0L fcheck4 ]
test[ fOffsetBlock(dsrcA ddst 4.0d 4) 39.0L 21.0L 5.125L 104.0L fcheck4 ]
test[ fScaleBlock(dsrcA ddst 0.0d 4)  0.0L 0.0L 0.0L 0.0L fcheck4 ]
test[ fMixBlock(dsrcA ddst 1.0d 4)    35.0L 17.0L 1.125L 100.0L fcheck4 ]
test[ fMixBlock(dsrcB ddst 0.5d 4)    37.5L 18.0L 5.125L 102.0L fcheck4 ]

\ ==================================

\ Test exception handling

int caughtVal   int exNumIn   int finallyVal

: mod7catcher
  int exNum
  try
    raise(exNumIn)
  except
    -> exNum
    if(mod(exNum 7) 0=)
      7 -> caughtVal
    else
      raise(exNum)
    endif
  finally
    finallyVal 1 or -> finallyVal
  endtry
;

: mod3catcher
  int exNum
  try
    mod7catcher
  except
    -> exNum
    if(mod(exNum 3) 0=)
      3 -> caughtVal
    else
      raise(exNum)
    endif
  finally
    finallyVal 2 or -> finallyVal
  endtry
;

: bystander2
  17 mod3catcher
;

: bystander1
  bystander2
;

: mod11catcher
  -> exNumIn
  -1 -> caughtVal
  0 -> finallyVal
  int exNum
  try
    bystander1
  except
    -> exNum
    if(mod(exNum 11) 0=)
      11 -> caughtVal
    else
      raise(exNum)
    endif
  endtry
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

