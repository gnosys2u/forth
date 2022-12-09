autoforget MISCTEST
: MISCTEST ;

// these are less rigorous tests than those in forthtest, they are mainly looking for compile failures or runtime exceptions
: section ds " ======================== " dup %s 0 $word %s %s %nl ;

//===========================================================================
section filetest

: filetest
  mko ByteArray buffer
  ptrTo int tfile
  buffer.resize(2000)
  "testOutput" -> ptrTo byte dirName
  if(not(fexists(dirName)))
    mkdir(dirName 0x1ff) drop      // 0x1ff is the same as 777 octal (rwx permissions)
  endif

  mko String fname
  fname.set(dirName)
  fname.append("/_test.txt")
  
  if(fexists(fname.get))
    fname.get %s " exists!\n" %s
  else
    fname.get %s " does not exist!\n" %s
  endif

  fopen( fname.get "w" ) -> tfile
  fputs( "This is the first line.\n" tfile ) drop
  fputs( "This is the second line.\n" tfile ) drop
  "File position: " %s ftell( tfile ) %d %nl
  fclose( tfile ) drop

  fopen( fname.get "r" ) -> tfile
  flen( tfile ) -> int fileSize
  fname.get %s " has " %s fileSize %d " bytes.\n" %s
  fread( buffer.base 1 200 tfile ) -> int bytesActuallyRead
  "Read " %s bytesActuallyRead %d " bytes.\n" %s
  buffer.set(0 bytesActuallyRead)  // add a terminating null
  buffer.base %s
  fclose( tfile ) drop
  oclear buffer
  remove(fname.get) drop
  oclear fname
  rmdir(dirName) drop
;

filetest

//===========================================================================
section test op variables

: woohoo "woohoo!" %s ;

' woohoo -> op woo
' woohoo -> int loo

: t1 "aaa" %s woo "bbb" %s ;
: t2 loo -> op foo  "aaa" %s foo "bbb" %s ;

t1 t2

//===========================================================================
section test bsearch
requires forth_internals

variable bb

variable aa
8 allot


0x87553427 aa !
0xFAC2AA90 aa 4+ !

: findit
  bb c!
  // bsearch( KEY_ADDR ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
  bsearch(bb aa 8 1 kBTUByte 0 )
  bb c@ %x " is at index " %s %d %nl
;

0x55 findit
0x44 findit
0x12 findit

//===========================================================================
section test outstream redirection

class: DoubleOutStream extends OutStream
  OutStream mOutStream
  
  m: putChar
    dup mOutStream.putChar
    mOutStream.putChar
  ;m
  
  m: putBlock
    0 ?do
      dup c@ putChar 1+
    loop
    drop
  ;m
  
  m: putString
    dup strlen putBlock
  ;m
  
  m: delete
    oclear mOutStream
    super.delete
  ;m
  
  m: init
    -> mOutStream
  ;m
  
;class

mko DoubleOutStream dsObj
getConsoleOut dsObj.init
dsObj setConsoleOut
`a` %c `b` %c %nl
"jklmn" dup strlen type
"does this work?" %s
resetConsoleOut
%nl
oclear dsObj

//===========================================================================
section test stats

system.stats

system.getDefinitionsVocab.getName %s %nl
system.getSearchVocabTop.getName %s %nl
system.getSearchVocabDepth %d %bl system.getOpsTable %x %nl
dump(system.getDefinitionsVocab.getNewestEntry 32)

//===========================================================================
section test showStruct

struct: boo
  int a
  long b
  float c
  10 string d
;struct

boo bb

bb ref boo showStruct

55 -> bb.a
123456789l -> bb.b
123.675 -> bb.c
bb ref boo showStruct
bb 100 dump

"asdfsd" -> bb.d
bb ref boo showStruct

struct: moo
  10 string mn
  boo mboo
  5 arrayOf 20 string mb
;struct

moo mm

: testmoo
  "ksjd" -> mm.mn
  "one" -> 0 mm.mb
  "two" -> 1 mm.mb
  "three" -> 2 mm.mb
  "four" -> 3 mm.mb
  "five" -> 4 mm.mb
  "ddd" -> mm.mboo.d
  %nl mm ref moo showStruct
  mm 300 dump
;

testmoo
forget boo
//===========================================================================
section goto and labels

: testGotoLabels
  -> int m
  m 5 > gotoIfNot isBad
  m 10 <= gotoIf isGood
  m 100 <= gotoIf isBad
  
label isGood
    m . "is good!\n" %s
    goto exit
label isBad
    m . "SUCKS!!\n" %s
label exit
;

: testAndifOrif
  -> int m
  if(m 5 >) andif(m 10 <=) orif(m 100 >)
    m . "is good!\n" %s
  else
    m . "SUCKS!!\n" %s
  endif
;

: testOfIf
  -> int n
  case(n)
    ofif(n 1 =) "one" %s endof
    ofif(n 10 <) "less than 10" %s endof
    ofif(n 10 =) "ten" %s endof
    ofif(n 20 <) "11 to 19" %s endof
    ofif(n 30 <) "20 to 29" %s endof
    dup %d
  endcase
;

: testAll dup testGotoLabels dup testAndifOrif testOfIf ;
4 testAll  5 testAll 7 testAll 10 testAll 11 testAll 20 testAll 100 testAll 101 testAll %nl
forget testGotoLabels

//===========================================================================
section more funcs

: moo
  int a int b 5 -> a 7 -> b
  f: 3 -> int a ;f drop
  a . b .
;
moo %nl
f: "wahooey!" %s ;f -> op wahoo
wahoo %nl
: boo
  f: "bsjkdfjsk" %s ;f
  dup %x %bl -> wahoo
;
boo wahoo %nl
forget moo

//===========================================================================
section test custom show method

class: customShowTest
  int id
  int wut
  
  new:
    _allocObject ->o customShowTest newObj
    4 -> newObj.id
    7 -> newObj.wut
    newObj
  ;
  
  m: show
    if(scHandleAlreadyShown)
      scBeginIndent
      scShowHeader
      scBeginFirstElement("id")
      id %d
      scBeginNextElement("wutzit")
      wut %d
      scEndElement(null)
      scEndIndent
      scShowIndent("}")
    endif
  ;m
  
;class

mko customShowTest aa

mko customShowTest bb
7 -> bb.id

mko customShowTest cc
9 -> cc.id

mko Array boo
boo.push(aa)
boo.push(bb)
boo.push(cc)
boo.push(aa)
boo.push(bb)
boo.push(cc)

boo.show %nl

forget customShowTest
//===========================================================================
section test file out stream

mko FileOutStream outFile
mko FileInStream inFile

outFile.open("_testData.txt" "wb") drop
outFile.putChar(`a`)
outFile.putChar(0xD)
outFile.putChar(`b`)
outFile.putChar(0xD)
outFile.putChar(0xA)
outFile.putChar(`c`)
outFile.putChar(0xD)
outFile.close

inFile.open("_testData.txt" "r") drop
long bb

`12345678` -> bb
inFile.getLine(ref bb 7) drop
dump(ref bb 8)

`12345678` -> bb
inFile.getLine(ref bb 7) drop
dump(ref bb 8)

`12345678` -> bb
inFile.getLine(ref bb 7) drop
dump(ref bb 8)

inFile.close
oclear outFile
oclear inFile
remove("_testData.txt") drop
forget outFile

