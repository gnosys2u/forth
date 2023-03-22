autoforget vartest

requires testbase

: vartest ;

decimal

\ empty stack markers - should still be here after all tests done
$DEADBEEF 123456789

\ true beNoisy!
0 numFailedTests!

testBuff.set( "aba" )
test[ checkResult( "aba" ) ]

\ =========================================================

-11 byte gvb!   4 ubyte gvub!   short gvs   55 ushort gvus!   12 int gvi!   uint gvui
55L long gvl!   12L ulong gvul!   7.5Ef sfloat gvf!   -100.25E float gvd!

test[ -11 gvb = 4 gvub = 0 gvs = 55 gvus = 12 gvi = 0 gvui = ]
test[ 55L gvl l= 12l gvul l= 7.5Ef gvf sf= -100.25E gvd f= ]

test[ $1ffffffffl gvl! gvl++ gvl $200000000l l= ]
test[ gvl-- gvl $1ffffffffl l= ]
test[ 1l gvl@+   $200000000l l= ]
test[ $200000000l gvl@- 1l l= ]
test[ 1l gvl!+ gvl $200000000l l= ]
test[ $200000000l gvl!- gvl l0= ]

: longtest1
  long vv

  startTest
  vv %ld %bl
  vv++ vv %ld %bl
  vv-- vv-- vv %ld %bl
  vv~ vv %ld %bl
  $1ffffffffl vv! vv++ vv %lx %bl
  5l vv@+ %lx %bl
  $3fffffffal vv@- %lx %bl
;
test[ longtest1 checkResult( "0 1 -1 0 200000000 200000005 1fffffffa " ) ]

: longtest2
  $200000000l long vv!

  startTest
  vv@-- %lx %bl vv %lx %bl
  vv@++ %lx %bl vv %lx %bl
  vv--@ %lx %bl vv %lx %bl
  vv++@ %lx %bl vv %lx %bl
;
test[ longtest2 checkResult( "200000000 1ffffffff 1ffffffff 200000000 1ffffffff 1ffffffff 200000000 200000000 " ) ]

: testGlobalVars1
  startTest
  gvi %d %bl gvi~
  7 0 do gvi %d %bl 5 gvi!+ loop
  3 0 do gvi--@ %d %bl loop
;
test[ testGlobalVars1 checkResult( "12 0 5 10 15 20 25 30 34 33 32 " ) ]

: testGlobalVars2
  startTest
  -5 gvb! gvb . 12 gvb!+  gvb . 3 gvb!-  gvb .
  -5 gvub! gvub . 12 gvub!+  gvub . 3 gvub!-  gvub .
  -1100 gvs! gvs . 1200 gvs!+  gvs . 1100 gvs!-  gvs .
  -1100 gvus! gvus . 1200 gvus!+  gvus . 1100 gvus!-  gvus .
  -100000 gvi! gvi . 230000 gvi! gvi . 3000 gvi!-  gvi .
  -100000 gvui! gvui . 230000 gvui! gvui . 3000 gvui!-  gvui .
;
#if FORTH64
test[ testGlobalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 4294867296 230000 227000 " ) ]
#else
test[ testGlobalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 -100000 230000 227000 " ) ]
#endif

: testLocalVars1
  int aa
  startTest
  0 aa! 7 0 do aa %d %bl 5 aa!+  loop
;
test[ testLocalVars1 checkResult( "0 5 10 15 20 25 30 " ) ]

: testLocalVars2
  byte lvb   ubyte lvub   short lvs   ushort lvus   int lvi   uint lvui
  startTest
  -5 lvb! lvb . 12 lvb!+  lvb . 3 lvb!-  lvb .
  -5 lvub! lvub . 12 lvub!+  lvub . 3 lvub!-  lvub .
  -1100 lvs! lvs . 1200 lvs!+  lvs . 1100 lvs!-  lvs .
  -1100 lvus! lvus . 1200 lvus!+  lvus . 1100 lvus!-  lvus .
  -100000 lvi! lvi . 230000 lvi! lvi . 3000 lvi!-  lvi .
  -100000 lvui! lvui . 230000 lvui! lvui . 3000 lvui!-  lvui .
;
#if FORTH64
test[ testLocalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 4294867296 230000 227000 " ) ]
#else
test[ testLocalVars2 checkResult( "-5 7 4 251 7 4 -1100 100 -1000 64436 100 64536 -100000 230000 227000 -100000 230000 227000 " ) ]
#endif

: testGlobalVars3
  startTest
  -12345678987654321l gvl! gvl l. 54321l gvl!+  gvl l. 22l gvl!+  gvl l.
  12345678987654321l gvul! gvul l. 54321l gvul!-  gvul l. 22l gvul!-  gvul l.
;
test[ testGlobalVars3 checkResult( "-12345678987654321 -12345678987600000 -12345678987599978 12345678987654321 12345678987600000 12345678987599978 " ) ]

: testLocalVars3
  startTest
  long lvl   ulong lvul
  -12345678987654321l lvl! lvl l. 54321l lvl!+  lvl l. 22l lvl!+  lvl l.
  12345678987654321l lvul! lvul l. 54321l lvul!-  lvul l. 22l lvul!-  lvul l.
;
test[ testLocalVars3 checkResult( "-12345678987654321 -12345678987600000 -12345678987599978 12345678987654321 12345678987600000 12345678987599978 " ) ]

: testGlobalVars4
  startTest
  22.5Ef gvf! gvf sg. -0.125Ef gvf!+  gvf sg. 0.75Ef gvf!-  gvf sg.
  22.5E gvd! gvd g. -0.125E gvd!+  gvd g. 0.75E gvd!-  gvd g.
;
test[ testGlobalVars4 checkResult( "22.5 22.375 21.625 22.5 22.375 21.625 " ) ]

: testLocalVars4
  startTest
  sfloat lvf   float lvd
  22.5Ef lvf! lvf sg. -0.125Ef lvf!+  lvf sg. 0.75Ef lvf!-  lvf sg.
  22.5E lvd! lvd g. -0.125E lvd!+  lvd g. 0.75E lvd!-  lvd g.
;
test[ testLocalVars4 checkResult( "22.5 22.375 21.625 22.5 22.375 21.625 " ) ]

: testStringVars
  startTest
  "woohoo" 100 string lvs!
  100 string lvs  "woohoo" lvs!
  lvs %s %bl
  "ey!" lvs!+ lvs %s %bl
  lvs~ "{" %s lvs %s "}" %s %bl
  "done" lvs! lvs %s
;
test[ testStringVars checkResult( "woohoo woohooey! {} done" ) ]


test[ $DEADBEEF 123456789 2= ]	\ check for stack underflow or extra items

"vartest" %s showPassFail

loaddone


