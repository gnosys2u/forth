autoforget BUILTIN_CLASS_TEST

requires testbase

: BUILTIN_CLASS_TEST ;

\ Object: delete show getClass compare(OBJ) keep release refCount
mko Object ollie
ollie.show
ollie.getClass.show
ollie.getClass -> Class ollieClass
ollie.compare(ollieClass) "ollie cmp ollieClass = " %s %d %nl
ollie.keep ollie.show
ollie.release ollie.show

\ Class: delete create getParent getName getVocabulary getInterface("INTERFACE_NAME") setNew(OP)
ollieClass.create -> Object ollie2
ollieClass.getName "ollie has class " %s %s %nl
ollieClass.getVocabulary <Vocabulary>.show
oclear ollie

\ Iterable: headIter tailIter find clone count clear
\ Iter: seekNext seekPrev seekHead seekTail next prev current remove unref findNext(OBJ) clone

\ Array: resize(N) ref(N) unref(N) get(N) set(OBJ N) findIndex(OBJ) push(OBJ) popUnref load(N_OBJECTS N)
\ ArrayIter:
\ List: head tail addHead(OBJ) addTail(OBJ) removeHead removeTail unrefHead unrefTail load(N_OBJECTS N)
\ ListIter: swapNext swapPrev split
\ IntMap: get(KEY) set(OBJ KEY) findKey(OBJ) remove(KEY)
\ IntMapIter: nextPair prevPair

\ String: size length get set(STR) append(STR) resize(N) startsWith(STR) endsWith(STR) contains(STR) clear hash appendChar(CHAR)

\ Pair: setA(OBJ) getA setB(OBJ) getB   (no find, clone, count, clear)
\ PairIter: (no remove, findNext, clone)
\ Triple: setA(OBJ) getA setB(OBJ) getB setC(OBJ) getC   (no find, clone, count, clear)
\ TripleIter: (no remove, findNext, clone)

\ ByteArray: resize ref(N) get(N) set(OBJ N) findIndex(OBJ) push(BYTE) pop base
\ ByteArrayIter: 
\ ShortArray: resize ref(N) get(N) set(OBJ N) findIndex(OBJ) push(BYTE) pop base
\ ShortArrayIter: 
\ IntArray: resize ref(N) get(N) set(OBJ N) findIndex(OBJ) push(BYTE) pop base
\ IntArrayIter: 
\ LongArray: resize ref(N) get(N) set(OBJ N) findIndex(OBJ) push(BYTE) pop base
\ LongArrayIter: 

\ Int Long SFloat Float: get set(N) show compare(N)

\ Thread: init(PSTACK_SIZE RSTACK_SIZE OP) start push(N) pop rpush(N) rpop getState step reset

\ Int SFloat DFloat Long
mko Int mostRandomNumber   mostRandomNumber.set(17)
mko Int theAnswer   theAnswer.set(42)
test[ mostRandomNumber.value 2* 8+ theAnswer.get = ]
test[ mostRandomNumber.compare(theAnswer) 0< ]
test[ theAnswer.set($abcfe1a3)   theAnswer.getByte -$5d =   theAnswer.getUByte $a3 = ]
test[ theAnswer.getShort -$1e5d =   theAnswer.getUShort $e1a3 = ]
test[ theAnswer.set($abcf7123)   theAnswer.getByte $23 =   theAnswer.getUByte $23 = ]
test[ theAnswer.getShort $7123 =   theAnswer.getUShort $7123 = ]

mko SFloat fluffy   fluffy.set(5.625Ef)
mko SFloat froofy   froofy.set(i>sf(44))
test[ fluffy.value 8.0Ef sf* 45.0Ef sf= ]
test[ fluffy.get 1.0Ef sf+ 6.625Ef sf= ]
test[ fluffy.set(fluffy.value 8.0Ef sf*)  fluffy.compare(froofy) 0> ]

mko DFloat dribble   dribble.set(11.0625E)
mko DFloat drabble  drabble.set(i>df(mostRandomNumber.get))
test[ dribble.get 0.9375E df+ 12 i>df df= ]
test[ dribble.set(dribble.value 5.9375E df+) drabble.compare(dribble) 0= ]

mko Long howie    howie.set(1000000000L)
mko Long face     face.set(howie.get howie.value l*)
test[ face.get 1000000000000000000L l= ]
test[ face.compare(howie) 0> howie.compare(face) 0< ]


\ String

mko Array arr
mko String donner   donner.set("Donner")
mko String blitzen   blitzen.set("Blitzen")
mko String rudy
test[ blitzen.compare(donner) 0< ]
test[ donner.length 6 = blitzen.length 7 = ]
test[ donner.size donner.length >=  blitzen.size blitzen.length >= ]
test[ strcmp(blitzen.get "Blitzen") 0=   strcmp(blitzen.get "Blitzend") 0<  not(blitzen.equals("Blitze")) ]
test[ rudy.clear rudy.length 0= ]
test[ rudy.copy(blitzen)   rudy.keepLeft(5)  rudy.appendBytes("krieg" 5)   rudy.equals("Blitzkrieg") ]
test[ rudy.copy(blitzen)   rudy.keepLeft(100) rudy.compare(blitzen) 0= ]
test[ rudy.copy(blitzen)   rudy.keepRight(5)  strcmp(rudy.get "itzen") 0= ]
test[ rudy.copy(blitzen)   rudy.keepRight(55)  rudy.compare(blitzen) 0= ]
test[ rudy.copy(blitzen)   rudy.keepMiddle(2 3)  rudy.equals("itz") ]
test[ rudy.copy(blitzen)   rudy.keepMiddle(2 13)  rudy.equals("itzen") ]
test[ rudy.copy(blitzen)   rudy.keepMiddle(12 13)  rudy.equals("") ]
test[ rudy.copy(blitzen)   rudy.leftBytes(5)   strncmp("Blitz" -rot) 0= ]
test[ rudy.copy(blitzen)   rudy.leftBytes(100)   strncmp("Blitzen" -rot) 0= ]
test[ rudy.copy(blitzen)   rudy.rightBytes(5)   strncmp("itzen" -rot) 0= ]
test[ rudy.copy(blitzen)   rudy.rightBytes(55)   strncmp("Blitzen" -rot) 0= ]
test[ rudy.copy(blitzen)   rudy.middleBytes(2 3)   strncmp("itz" -rot) 0= ]
test[ rudy.copy(blitzen)   rudy.middleBytes(2 13)   strncmp("itzen" -rot) 0= ]
test[ rudy.copy(blitzen)   rudy.middleBytes(12 13)  0= swap strcmp("") 0= ]
test[ rudy.copy(donner) rudy.append(" Part") rudy.appendChar('y') rudy.prepend("The ") rudy.equals("The Donner Party") ]
test[ donner.toLower strcmp(donner.get "donner") 0=  donner.toUpper donner.equals("DONNER") ]
test[ donner.replaceChar('N' 'l') strcmp(donner.get "DOllER") 0= ]
test[ rudy.split(arr bl) rudy.join(arr "_") rudy.equals("The_Donner_Party") not(rudy.equals("foo")) ]
test[ rudy.startsWith("Th") not(rudy.startsWith("bl")) rudy.endsWith("rty") not(rudy.endsWith("vw")) ]
test[ rudy.contains("onne") not(rudy.contains("cheesecake")) ]
test[ rudy.copy(donner) rudy.resize(3) rudy.length 3 = rudy.equals("DOl") ]
test[ rudy.getBytes 3 = swap strcmp("Don") ]
test[ rudy.setBytes("spoofity" 5) rudy.equals("spoof") ]
test[ rudy.appendBytes("ishness" 3) rudy.equals("spoofish") ]
test[ rudy.prependBytes("unti" 2) rudy.equals("unspoofish") ]
test[ rudy.hash 0<> ]
test[ rudy.format("all %d %s" 9 "yards" 2) rudy.equals("all 9 yards") ]
test[ rudy.appendFormatted(" of %s" "ale" 1) rudy.equals("all 9 yards of ale") ]
test[ 0 rudy.get 5+ c! rudy.fixup rudy.length 5 = ]


int nextObjId
-1 -> int lastTobjDeleted

class: tobj    extends Object
  int objId

  m: setObjId
    -> objId
  ;m
    
  new:
    _allocObject
    nextObjId over -> <tobj>.objId
    "|+" %s nextObjId %d
    1 ->+ nextObjId
  ;

  \ m: show    objId %d %bl  ;m
  
  m: delete
    objId -> lastTobjDeleted
    "|-" %s objId %d
    super.delete
  ;m
  
;class


mko tobj valA
mko tobj valB
mko tobj valC
mko tobj valD
mko tobj valE

\ Array:   clear   resize(N)   count   get(I)   set(O I)   findIndex(O)   push(O)   pop   headIter   tailIter   clone
\ List:   head   tail   addHead(O)   addTail(O)   removeHead   removeTail   headIter   tailIter   count   find(O)   clone
\ IntMap:   clear   count   get(KEY)   set(O KEY)   findIndex(O)   remove(KEY)   headIter   tailIter   

\ "(vb=" %s tb <tobj>.refCount %d " before adding to map)" %s
mko IntMap imapA
imapA.set( valA 'a' )  imapA.set( valB 'b' )  imapA.set( valC 'c' )  imapA.set( valD 'd' )  imapA.set( valE 'e' )
%nl imapA.show %nl

mko LongMap lmapA
lmapA.set( valA 1l )  lmapA.set( valB '22'l )  lmapA.set( valC 1000000000l )  lmapA.set( valD -15l )  lmapA.set( valE 66l )
%nl lmapA.show %nl

mko SFloatMap fmapA
fmapA.set( valA 1.7Ef )  fmapA.set( valB 2.3Ef )  fmapA.set( valC 33.0Ef )  fmapA.set( valD 1.234Ef )  fmapA.set( valE 5.789Ef )
%nl fmapA.show %nl

mko FloatMap dmapA
dmapA.set( valA 1.7E )  dmapA.set( valB 2.3E )  dmapA.set( valC 33.0E )  dmapA.set( valD 1.234E )  dmapA.set( valE 5.789E )
%nl dmapA.show %nl

"===================================================\n"%s
mko StringMap smapA
smapA.set( valA "aa" )  smapA.set( valB "bb" )  smapA.set( valC "cc" )  smapA.set( valD "dd" )  smapA.set( valE "ee" )
smapA.show %nl
"===================================================\n"%s
: testsmapAGet
  -> ptrTo byte pName
  if(smapA.grab(pName))
    <Object>.show
  else
    pName %s " not found in smapA\n" %s
  endif
;
testsmapAGet("aa")
testsmapAGet("bb")
testsmapAGet("cc")
testsmapAGet("cc")
testsmapAGet("ee")
testsmapAGet("foo")
%nl
%nl smapA.show %nl

mko List listA
: tlistA
  if(imapA.grab)
    listA.addHead
  endif
;
tlistA('b') tlistA('c') tlistA('d')
%nl listA.show %nl

mko Array arrayA
arrayA.push(imapA.grab( 'a' ) drop)  arrayA.push(valB)  arrayA.push(imapA.grab( 'c' ) drop)
%nl arrayA.show %nl

: showArray
  -> Array a
  a.headIter -> ArrayIter iter
  a.show %nl
  begin
  while( iter.next )
    <Object>.show
  repeat
  begin
  while( iter.prev )
    <Object>.show
  repeat
  oclear a
  oclear iter
  %nl
;
arrayA showArray

"toList\n" %s
arrayA.toList -> List listFromArrayA
"toList 2\n" %s
listFromArrayA.show
"toList 3\n" %s
listFromArrayA.toArray -> Array arrayFromListA
arrayFromListA.show

oclear listFromArrayA  oclear arrayFromListA

\ "{clearing map}" %s
oclear imapA  oclear lmapA  oclear fmapA  oclear dmapA  oclear smapA

\ "{clearing list}" %s
oclear listA

\ "{clearing array}" %s
oclear arrayA

\ test odrop
: test_odrop
  "testing odrop\n" %s
  
  12345
  nextObjId -> int testId
  -1 -> lastTobjDeleted
  
  new tobj odrop
  if(testId lastTobjDeleted <>)
    "odrop test1 FAILED!!!!\n" %s
  endif
  
  if(dup 12345 <>)
    "odrop test1 left junk on stack!!!!\n" %s
  endif
  
  mko tobj fred
  -1 -> lastTobjDeleted
  fred odrop
  if(-1 lastTobjDeleted <>)
    "odrop test2 FAILED!!!!\n" %s
  endif
  
  if(12345 <>)
    "odrop test2 left junk on stack!!!!\n" %s
  endif

  oclear fred  
;
test_odrop
  
oclear valA  oclear valB  oclear valC  oclear valD  oclear valE

mko List zz
mko String za
mko String zb
mko String zc
mko String zd
mko String zr
za.set( "a" )
zb.set( "b" )
zc.set( "c" )
zd.set( "d" )
zr.set( "r" )
zz.addTail( za ) zz.addTail( zb ) zz.addTail( zr ) zz.addTail( za ) 
zz.addTail( zc ) zz.addTail( za ) zz.addTail( zd ) 
zz.addTail( za ) zz.addTail( zb ) zz.addTail( zr ) zz.addTail( za ) 

zz.clone -> List zz2

: so
  drop dup %x %bl @ %d %bl
;

\ abra cad abra
: ff
  -> Iterable oi
  "list " %s oi so %nl
  oi.headIter -> Iter iter
  begin
  while( iter.next )
    -> String ss
    ss.show
    oclear ss
  repeat
  %nl
  oclear oi
;

\ Iterable: headIter tailIter find clone count clear
\ Iter: seekNext seekPrev seekHead seekTail next prev current remove findNext(OBJ) clone

\ Array: resize(N) ref(N) get(N) set(OBJ N) findIndex(OBJ) push(OBJ) pop
\ ArrayIter:

: testArray
  mko Array aa
  r[ 1 17 42 53 89 ]r
  -> int nItems
  do( nItems 0 )
  loop
  oclear aa
;

: test
  mko Array aa
  mko List la
  do( 15 0 )
    mko Int bob
    bob.set( i )
    aa.push( bob )
    la.addTail( bob )
    oclear bob
  loop
  oclear aa
  oclear la
;

test
"run cleanup to free objects" %s %nl

: cleanup
  oclear ollie
  oclear zz  oclear zz2
  oclear za  oclear zb  oclear zc  oclear zd  oclear zr
;

oclear arr

loaddone

+	Object
-	Class
+	Array
+	ArrayIter
+	List
-	ListIter
-	Map
-	MapIter
-	IntMap
-	IntMapIter
-	LongMap
-	LongMapIter
-	SFloatMap
?	SFloatMapIter
-	FloatMap
?	FloatMapIter
-	StringMap
-	StringMapIter
+	String
+	Pair
-	PairIter
+	Triple
-	TripleIter
+	ByteArray
+	ByteArrayIter
+	ShortArray
+	ShortArrayIter
+	IntArray
+	IntArrayIter
+	LongArray
+	LongArrayIter
+	SFloatArray
?	SFloatArrayIter
+	FloatArray
?	FloatArrayIter
+	Int
+	Long
+	SFloat
+	Float
-	Thread
-	InStream
-	FileInStream
-	ConsoleInStream
-	OutStream
-	FileOutStream
-	StringOutStream
-	ConsoleOutStream
-	FunctionOutStream
