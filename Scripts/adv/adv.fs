requires randoms

\ true compileDebug!

autoforget collosalCaveAdventure
vocabulary collosalCaveAdventure
also collosalCaveAdventure definitions

: ADV ;

: sp!
  depth r> swap >r >r
;

ptrTo byte _spName
: sp@
  _spName!
  r> r> swap >r depth 1-
  if(2dup <>)
    "Stack mismatch in " %s _spName %s %bl %d %bl %d %bl ds
  else
    2drop
  endif
;
alias sp? sp@

\ spoo MESSAGE_UNTIL_END_OF_LINE - spit out message while debugging
\   good for tracking down when loading just dies quietly
: spoo
  0 $word ptrTo byte msg!
  d[ t{ msg %s %nl ds }t ]d
; immediate spoo

spoo before defs
requires defs
spoo after defs

\ The Z-machine's "@random 42" instruction returns a value in the range 1..42.
: ran random swap mod ;
: pct ran(100) swap < ;
: streq 
  \ 5 strncmp 0=
  true bool result!
  ptrTo byte pStr1!
  ptrTo byte pStr2!
  do(5 0)
    pStr1 i + c@ byte ch1!
    pStr2 i + c@ byte ch2!
    if(ch1 ch2 <>)
      false result!
      leave
    endif
    if(ch2 0=)
      leave
    endif
  loop
  result
;
: puts %s %nl ;

spoo before Place
\ =============================== Place ===============================
class: Place
  String long_desc
  String short_desc
  uint flags
  int locID
  int objects  \ ObjectWord - first object at this location, or NOTHING
  int visits
  
  \ locationID longDescription shortDescription flags ...
  m: init
    flags!
    new String short_desc!  short_desc.set
    new String long_desc!  long_desc.set
    locID!
    NOTHING objects!
    0 visits!
    \ flags if short_desc.get %s " has flags $" %s flags %x %nl endif
  ;m
  
  m: delete
    long_desc~
    short_desc~
  ;m
  
  m: setVisits
    visits!
  ;m
;class


\ =============================== IObjectData ===============================
class: ObjectData
    ObjectWord link  \ next object at this location, or NOTHING
    ObjectWord base  \ NOTHING for mobile objects
    int prop
    Location place
    String name
    Array of String desc  \ .prop ranges from 0 to 3
    
    m: delete
      name~
      desc~
    ;m
    
    m: init  \ NAME_STRING BASE? LOC_ID
      place!
      base!
      new String name!
      name.set
      new Array desc!
    ;m
    
    m: removeObject ;m
    
    m: setProp
      prop!
    ;m
    
    m: setPlace
      place!
    ;m
;class

\ =============================== Dwarf ===============================
class: Dwarf
  bool seen
  Location oldloc
  Location loc
  int index
  
  m: setLoc
    loc!
    d[ ">>> Moving dwarf " %s index %d " to " %s loc ref Location findEnumSymbol if(0=) "???" endif %s %nl ]d
  ;m
  
  m: setIndex index! ;m
  m: setOldloc oldloc! ;m
  m: setSeen seen! ;m

;class
  
spoo before Word
\ =============================== Word ===============================
class: Word
  String text
  int meaning
  WordClass class
  
  m: init
    class!
    meaning!
    new String text!
    text.set
  ;m
  
  m: delete
    text~
  ;m
  
;class

\ =============================== Hint ===============================
class: Hint
  int count
  bool given
  int thresh
  int cost
  String prompt
  String hint

  m: delete
    prompt~
    hint~
  ;m

;class


class: AdventureState
  String name
  op stateOp
  eAdventureState stateNum
    
  m: init
    stateNum!
    stateOp!
    new String name!
    name.set
  ;m
  
  m: delete
    name~
  ;m
  
;class
  
spoo before IGame
\ =============================== IGame ===============================
\ IGame is abstract interface for Game class
72 constant BUF_SIZE
350 constant MAX_SCORE
3 constant MAX_DEATHS
8 constant HIGHEST_CLASS

  
    
    

class: IGame
  StringIntMap wordMap
  Array of Word words

  733 arrayOf Instruction travels       \ Instruction travels[733];
  MAX_LOC 22+ arrayOf ptrTo Instruction start    \ Instruction *start[MAX_LOC + 2];
  Array of Place places

  int holding_count          \ how many objects have objs(t).place < 0?
  Location last_knife_loc
  int tally                 \ treasures awaiting you
  int lost_treasures  \ treasures that you won't find
  Array of ObjectData _objs
  
  \ BUF_SIZE arrayOf byte buffer \ your input goes here 
  \ BUF_SIZE arrayOf byte word1 \ and then we snarf it to here
  \ BUF_SIZE arrayOf byte word2 \ and here
  ByteArray inBuffer
  ByteArray inWord1
  ByteArray inWord2
  int dwarfAngerLevel  \ how angry are the dwarves?
  Array of Dwarf dwarves
  Dwarf pirate
  
  bool gave_up
  int death_count

  int lamp_limit  \ countdown till darkness
  int clock1   int clock2  \ clocks that govern closing time
  bool closed  \ set only when you're in the repository
  int bonus  \ extra points awarded for exceptional adventuring skills

  bool warnedOfClosing
  bool closingPanic
  
  int turns  \ how many times we've read your commands
  int verbose_interval  \ command BRIEF sets this to 10000
  int foobar  \ progress in the FEE FIE FOE FOO incantation

  Array of String classMessages
  IntArray classScores  
  Array of String deathWishes

  Array of Hint hints
  
  bool firstDwarfKill
  bool haveTriedToGetKnife
  int __west_count
  Array of AdventureState adventureStates
  
  ConsoleInStream consoleInStream
  FileInStream restoreStream
  String saveFileName
  String restoreFileName
  FileOutStream saveStream
  
  \ start adventure state machine variables
  Location mOldOldLoc
  Location mOldLoc
  Location mLoc
  Location mNewLoc
  MotionWord mMotion   \ currently specified motion
  ActionWord mVerb  \ currently specified action
  ActionWord mOldVerb  \ mVerb before it was changed
  ObjectWord mObj  \ currently specified object, if any
  ObjectWord mOldObj  \ former value of mObj
  bool mWasDark
  bool mQuit
  int mLookCount
  int mWordTemp
  eAdventureState mCurStateNum
  \ end adventure state machine variables

  m: delete
    words~
    wordMap~
    places~
    _objs~
    dwarves~
    pirate~
    classMessages~
    classScores~
    deathWishes~
    hints~
    inBuffer~
    inWord1~
    inWord2~
    adventureStates~
    consoleInStream~
    restoreFileName~
    restoreStream~
    saveFileName~
    saveStream~
  ;m
  
  m: init
    new Array words!
    new StringIntMap wordMap!
    R_LIMBO last_knife_loc!
    15 tally!
    0 lost_treasures!
    new Array _objs!
    _objs.resize(MAX_OBJ 1+ MIN_OBJ -)
    new Array dwarves!
    15 clock1!
    30 clock2!
    false closed!
    0 bonus!
    false warnedOfClosing!
    false closingPanic!
    0 turns!
    5 verbose_interval!
    0 foobar!
    new Array classMessages!
    new IntArray classScores!
    new Array deathWishes!
    new Array hints!
    true firstDwarfKill!
    false haveTriedToGetKnife!
    new Array places!
    0 __west_count!
    new ByteArray inBuffer   inBuffer.resize(BUF_SIZE)   inBuffer.set(0 0)!
    new ByteArray inWord1    inWord1.resize(BUF_SIZE)    inWord1.set(0 0)!
    new ByteArray inWord2    inWord2.resize(BUF_SIZE)    inWord2.set(0 0)!
    new Array adventureStates!
    
    new ConsoleInStream consoleInStream!
    new String saveFileName!
    saveFileName.set("tmp.sav")
    new String restoreFileName!
    restoreFileName.set("adventure.sav")
  ;m
  
  m: getWordByName returns Word
  ;m
    
  m: dwarves_upset ;m
  m: give_up ;m
  m: quit ;m
  m: removeObjectFrom ;m
  m: build_object_table ;m
  m: dispatchState ;m

  m: addHint
    hints.push
  ;m
  
  m: objs returns ObjectData    \ OBJID ... OBJDATA
    MIN_OBJ - _objs.get
  ;m
  

  m: addObjectTo
    Location loc!
    ObjectWord t!
    Place place
    ObjectData obj
    places.get(loc) place!o
    ref place.objects ptrTo ObjectWord p!
    d[ "adding obj " %s t %d " to loc " %s loc %d  %bl place %x %nl ]d
    begin
    while(p i@ NOTHING <>)
      objs(p i@) obj!o 
      ref obj.link p!
    repeat
    t p i!
  ;m

;class
spoo after IGame

"OK." $constant okMsg   \ Woods' Fortran version didn't include the period, by the way.
"It is now pitch dark.  If you proceed you will most likely fall into a pit." $constant pitchDarkMsg

: is_forced
  Location loc!
  case(loc)
    of(R_NECK)    of(R_LOSE) of(R_CLIMB)
    of(R_CHECK)   of(R_THRU) of(R_DUCK)
    of(R_UPNOUT)  of(R_DIDIT)
      true
    endof
    
    false swap
  endcase
;

: has_light
  Location loc!
  case(loc)
    of(R_ROAD) of(R_HILL) of(R_HOUSE) of(R_VALLEY)
    of(R_FOREST) of(R_FOREST2) of(R_SLIT) of(R_OUTSIDE)
    of(R_INSIDE) of(R_COBBLES)
    of(R_PLOVER) of(R_VIEW)
    of(R_NEEND) of(R_SWEND)
      true
    endof
    
    false swap
  endcase
;

: has_water
  Location loc!
  case(loc)
    of(R_ROAD) of(R_HOUSE) of(R_VALLEY) of(R_SLIT)
    of(R_WET) of(R_FALLS) of(R_RES)
      true
    endof
    
    false swap
  endcase
;

: has_oil R_EPIT = ;

\ ===============================  ===============================

spoo before game
requires game

Game game

spoo before hints
requires hints
spoo before locations
requires locations
spoo before words
requires words
spoo after words

: goBody
  d[ ." before build_vocabulary" cr   ds ]d
  build_vocabulary
  d[ ." before build_travel_table" cr  ds ]d
  build_travel_table
  d[ ." before build_hints" cr  ds ]d
  build_hints
  d[ ." before build_object_table" cr  ds ]d
  game.build_object_table
  d[ ." before initialSetup" cr  ds ]d
  game.initialSetup
  d[ ." before simulate_an_adventure" cr  ds ]d
  game.simulate_an_adventure
;

: go
  "Using random seed " %s getRandomSeed %d %nl
  new Game game!
  game.init
  goBody
;

: restore
  new Game game!
  game.init
  new FileInStream game.restoreStream!
  blword ptrTo byte restoreName!
  if(strlen(restoreName) 0>)
    game.restoreFileName.set(restoreName)
    game.restoreFileName.append(".sav")
  else
    game.restoreFileName.set("adventure.sav")
  endif
  game.restoreStream.open(game.restoreFileName.get "r") drop
  goBody
;

previous definitions

: go collosalCaveAdventure:go ;
: restore collosalCaveAdventure:restore ;

"Type:\n\+
  go            to start a new adventure\n\+
  restore       to restore the default save (adventure.sav)\n\+
  restore BLAH  to restore from BLAH.sav\n\+
In the game enter:\n\+
  save          to save to the default save\n\+
  save BLAH     to save to BLAH.sav\n" %s
  
loaddone

e
get lamp
xyzzy
on lamp     We intentionally leave the rod there for now.
e
get cage
pit
e
get bird
w
d
s
get gold
n
n              This should bring you to the Hall of the Mt. King.
free bird
drop cage
s
get jewel
n
w
get coins
e    had to hit e again - maybe because of dwarves in room?
n
get silver          This should bring you to Y2.
n
plover
ne
get pyramid
s
plover          If any dwarves have shown up already, restart!
plugh
drop pyramid
drop coins
drop jewel
drop silver
drop gold
get bottle
get food
get keys
plugh               Now get axe, when you see it.
s
d
bedquilt
slab
s
d
pour water
u
w
u
reservoir
get water
s
s
d
s
d                   You should be at the bottom of West pit now.
pour water
u
e
d
get oil
u
w
d
climb
w
get eggs
n
oil door
  need to say "pour oil"
n
get trident
w
d
drop bottle
sw                      You should be at the South-West side of the chasm now.
u
toss eggs
  didn't work, neither did "give eggs"
cross
ne
barren
e
feed bear
open chain
get chain
get bear
w
fork
ne
e
get spice
fork
w
w                   You should be at the North-East side of the chasm now.
cross
free bear
cross
sw
d
drop keys
bedquilt
e
n
open clam
d
d
get pearl
u
u
s
u
e
u                   This should bring you back to Y2.
n
plugh
drop chain
drop spice
drop trident
drop pearl
plugh
s
d
bedquilt
w
oriental
n                   This should put you in the alcove.
w
drop lamp
drop axe
e
get emerald
w
get axe
get lamp
nw
s
get vase
se
e
get pillow
w
w
w
d
climb               You should now be in the Giant Room.
w
fee
fie
foe
foo
get eggs
s
d
u
w
u
s
kill dragon
yes
get rug
e
e
n                   This should bring you once more to Y2.
n
plugh
drop rug
drop pillow
drop vase
drop emerald
drop eggs           You now need to get a treasure stolen by the pirate if you haven't already.
xyzzy
get rod
pit
d
w
wave rod
w                   Let the pirate steal this (we saved it for this occasion).
get diamond
w
s                   Hope the pirate comes while you're on the way to his lair
e
s
s
s
n
e
n
e
nw                     Now into Dead end (if he hasn't shown up by now, see note 1).
   repeat se,n,w,w,w,e,e,w,s,n,s,s,s,n,e,n,e,nw until he appears
get chest           Also get any other treasures the pirate stole.
get diamond
se
n
d
debris
xyzzy
drop rod
drop chest
drop diamond
plugh               You may need to save the magazines until now, but you may not.
s
d
bedquilt
e
e
get magazine
e
drop magazine       
n                   Go North until you get out (see note 2).
   Wait until the cave closes.
sw
get rod
ne
drop rod
sw
blast