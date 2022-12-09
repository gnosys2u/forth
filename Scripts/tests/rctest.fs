autoforget RCTEST
: RCTEST ;

// add ndrop
// fix bug where apps which contain app_autoload.txt only run it if there is an app_autoload.txt file also
// is there a bug where an embedded app_autoload.txt which has no loaddone gets an error at eof?
// see if builtin classes have a 'new' method
: ndrop 0 do drop loop ;

// N_STRINGS N String makeStr ... String object
: makeStr
  -> String str
  -> int nStrings
  do( 0 nStrings 1- )
    //i %d %nl
    str.append( i pick )
  +loop( -1 )
  ndrop( nStrings )
  str.release
;

: ninterpret
  new String -> String foo
  makeStr( foo )
  $evaluate( foo.get )
  //foo.get %s %nl
  foo.release
;

: obj  // obj TYPE_NAME INSTANCE_NAME  - defines obj var and creates instance
  256 string typeName
  blword -> typeName
  256 string instName
  blword -> instName
  r[ "new "  typeName " -> " typeName " " instName ]r ninterpret
;

: str  // STRING_PTR str INSTANCE_NAME  - defines String var and creates instance & initializes value to STRING_PTR
  new String -> String instName
  blword instName.set
  r[ "new String -> String " instName.get " " instName.get ".set" ]r ninterpret
  instName.release
;


: showContainer
  <Iterable>.headIter -> Iter iter
  begin
    iter.next
  while
    dup <String>.get %s %bl <Object>.show %nl
  repeat
  oclear iter
;

: showContainerR
  <Iterable>.tailIter -> Iter iter
  begin
    iter.prev
  while
    dup <String>.get %s %bl <Object>.show %nl
  repeat
  oclear iter
;

: showArray
  <Array>.headIter dup showContainer
  <Iter>.release
;

: showArrayR
  <Array>.tailIter dup showContainerR
  <Iter>.release
;

: showList
  <List>.headIter dup showContainer
  <Iter>.release
;

: showListR
  <List>.tailIter dup showContainerR
  <Iter>.release
;

: showMap
  <IntMap>.headIter -> IntMapIter iter
  begin
    iter.seekNext iter.currentPair
  while
    "key=" %s %d %bl dup <String>.get %s %bl <Object>.show %nl
  repeat
  iter.release
;

: showMapR
  <IntMap>.tailIter -> IntMapIter iter
  begin
    iter.seekPrev iter.currentPair
  while
    "key=" %s %d %bl dup <String>.get %s %bl <Object>.show %nl
  repeat
  iter.release
;

"alsatian" str dogA
"beagle" str dogB
"corgie" str dogC
obj Array adogs
obj List ldogs
obj IntMap mdogs


: test
  "using arrays:\n" %s

  "empty\n" %s
  adogs showContainer  adogs showContainerR
  dogA adogs.push
  "one element\n" %s
  adogs showContainer  adogs showContainerR
  dogB adogs.push
  dogC adogs.push
  "three elements\n" %s
  adogs showContainer  adogs showContainerR


  %nl %nl
  "using lists:\n" %s

  "empty\n" %s
  ldogs showContainer  ldogs showContainerR
  dogC ldogs.addHead
  "one element\n" %s
  ldogs showContainer  ldogs showContainerR
  dogA ldogs.addTail
  dogB ldogs.addHead
  "three elements\n" %s
  ldogs showContainer  ldogs showContainerR


  %nl %nl
  "using maps:\n" %s
  "empty\n" %s
  mdogs showContainer  mdogs showContainerR
  dogA 1 mdogs.set
  "one element\n" %s
  mdogs showContainer  mdogs showContainerR
  dogB 2 mdogs.set
  dogC 3 mdogs.set
  "three elements\n" %s
  mdogs showContainer  mdogs showContainerR
  %nl %nl
;

#if 0

class: animal extends object
;class


class: zoo extends object
  String name
  Array cages
  
  // ZOO_NAME ...
  m: init
    new String -> name
    new Array -> cages
    name.set
  ;m
  
  m: delete
    super.delete
  ;m
  
  // CAGE_NAME ...
  m: addCage
    new OPair -> OPair newCage
    newCage.setA
    newCage cages.push
  ;m
  
  m: show
  ;m
  
  
;class

#endif


: cleanup
  oclear dogA   oclear dogB   oclear dogC
  oclear adogs  oclear ldogs  oclear mdogs
;

loaddone
