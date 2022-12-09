: advDefs ;

enum: WordClass
    WordClass_None WordClass_Motion WordClass_Object
    WordClass_Action WordClass_Message
;enum

0 constant NOTHING

enum: MotionWord
    100 MIN_MOTION
    MIN_MOTION ROAD
    
    ENTER UPSTREAM DOWNSTREAM FOREST FORWARD
    BACK VALLEY STAIRS OUT HOUSE GULLY STREAM ROCK
    BED CRAWL COBBLES IN SURFACE NOWHERE DARK PASSAGE
    LOW CANYON AWKWARD GIANT VIEW U D PIT OUTDOORS
    CRACK STEPS DOME LEFT RIGHT HALL JUMP BARREN
    OVER ACROSS E W N S NE SE SW NW DEBRIS HOLE
    WALL BROKEN Y2 CLIMB LOOK FLOOR ROOM SLIT
    SLAB XYZZY DEPRESSION ENTRANCE PLUGH SECRET
    CAVE CROSS BEDQUILT PLOVER ORIENTAL CAVERN
    SHELL RESERVOIR OFFICE FORK

    MOTION_LIMIT
;enum

enum: ObjectWord
    200 MIN_OBJ
    MIN_OBJ KEYS
    
    LAMP GRATE GRATE_ CAGE ROD ROD2 TREADS TREADS_
    BIRD RUSTY_DOOR PILLOW SNAKE FISSURE FISSURE_ TABLET CLAM OYSTER
    MAG DWARF KNIFE FOOD BOTTLE WATER OIL MIRROR MIRROR_ PLANT
    PLANT2 PLANT2_ STALACTITE SHADOW SHADOW_ AXE DRAWINGS PIRATE
    DRAGON DRAGON_ CHASM CHASM_ TROLL TROLL_ NO_TROLL NO_TROLL_
    BEAR MESSAGE GORGE MACHINE BATTERIES MOSS
    GOLD DIAMONDS SILVER JEWELS COINS CHEST EGGS
    TRIDENT VASE EMERALD PYRAMID PEARL RUG RUG_ SPICES CHAIN
    
    OBJ_LIMIT
    CHAIN MAX_OBJ 
;enum

enum: ActionWord
    300 MIN_ACTION
    MIN_ACTION TAKE
    
    DROP OPEN CLOSE ON OFF WAVE CALM GO
    RELAX POUR EAT DRINK RUB TOSS WAKE FEED FILL BREAK BLAST
    KILL SAY READ FEEFIE BRIEF FIND INVENTORY SCORE
    //SAVE RESTORE
    QUIT
    
    ACTION_LIMIT
;enum

enum: MessageWord
    400 MIN_MESSAGE
    MIN_MESSAGE ABRA
    
    HELP TREES DIG LOST MIST FUCK STOP INFO SWIM
    
    MESSAGE_LIMIT
;enum

: wordToName
  -> int wordNum
  case(wordNum)
    ofif(wordNum 400 >=)
      findEnumSymbol(wordNum  ref MessageWord)
    endof
    ofif(wordNum 300 >=)
      findEnumSymbol(wordNum  ref ActionWord)
    endof
    ofif(wordNum 200 >=)
      findEnumSymbol(wordNum  ref ObjectWord)
    endof
    ofif(wordNum 100 >=)
      findEnumSymbol(wordNum  ref MotionWord)
    endof
    if(wordNum 0=)
      "Nothing" true
    else
      false
    endif
  endcase
  if(0=)
    "UnknownWord"
    "WARNING: wordToName - unrecognized word with value " %s wordNum %d %nl
  endif
;

enum: Location
    -1 R_INHAND
    0 R_LIMBO
    
    // unique locations
    R_ROAD R_HILL R_HOUSE R_VALLEY R_FOREST R_FOREST2 R_SLIT R_OUTSIDE R_INSIDE
    R_COBBLES R_DEBRIS R_AWK R_BIRD R_SPIT R_EMIST
    R_NUGGET R_EFISS R_WFISS R_WMIST
    R_LIKE1 R_LIKE2 R_LIKE3 R_LIKE4 R_LIKE5 R_LIKE6 R_LIKE7
    R_LIKE8 R_LIKE9 R_LIKE10 R_LIKE11 R_LIKE12 R_LIKE13 R_LIKE14
    R_BRINK R_ELONG R_WLONG
    R_DIFF0 R_DIFF1 R_DIFF2 R_DIFF3 R_DIFF4 R_DIFF5
    R_DIFF6 R_DIFF7 R_DIFF8 R_DIFF9 R_DIFF10
    R_PONY R_CROSS R_HMK R_WEST R_SOUTH R_NS R_Y2 R_JUMBLE R_WINDOE
    R_DIRTY R_CLEAN R_WET R_DUSTY R_COMPLEX
    R_SHELL R_ARCHED R_RAGGED R_SAC R_ANTE R_WITT
    R_BEDQUILT R_SWISS R_SOFT
    R_E2PIT R_W2PIT R_EPIT R_WPIT
    R_NARROW R_GIANT R_BLOCK R_IMMENSE
    R_FALLS R_INCLINE R_ABOVEP R_SJUNC
    R_TITE R_LOW R_CRAWL R_WINDOW
    R_ORIENTAL R_MISTY R_ALCOVE R_PLOVER R_DARK
    R_SLAB R_ABOVER R_MIRROR R_RES
    R_SCAN1 R_SCAN2 R_SCAN3 R_SECRET
    R_WIDE R_TIGHT R_TALL R_BOULDERS
    R_SLOPING R_SWSIDE
    R_DEAD0 R_DEAD1 R_PIRATES_NEST R_DEAD3 R_DEAD4 R_DEAD5
    R_DEAD6 R_DEAD7 R_DEAD8 R_DEAD9 R_DEAD10 R_DEAD11
    R_NESIDE R_CORR R_FORK R_WARM R_VIEW R_CHAMBER
    R_LIME R_FBARR R_BARR
    R_NEEND R_SWEND
    R_NECK R_LOSE R_CLIMB R_CHECK
    R_THRU R_DUCK R_UPNOUT R_DIDIT
    
    R_PPASS R_PDROP
    R_TROLL
    FIRST_REMARK

    // meta
    R_INSIDE MIN_IN_CAVE
    R_EMIST MIN_LOWER_LOC
    R_DIDIT MAX_LOC
;enum

: locationToName
  -> int locationNum
  findEnumSymbol(locationNum  ref Location)
  if(0=)
    "UnknownLocation"
    "WARNING: locationToName - unrecognized location with value " %s locationNum %d %nl
  endif
;

enum: flagsBits
  0x008 F_CAVE_HINT
  0x010 F_BIRD_HINT
  0x020 F_SNAKE_HINT
  0x040 F_TWIST_HINT
  0x080 F_DARK_HINT
  0x100 F_WITT_HINT
;enum

struct: Instruction
  MotionWord mot    // command word
  int cond          // requirement for command to be obeyed
  Location dest     // where command takes you
;struct

: showInstruction
  -> ptrTo Instruction inst
  inst.mot wordToName %s " -> " %s inst.dest locationToName %s %bl
  inst.cond -> int cond
  mod(cond 100) MIN_OBJ + -> int obj
  if(cond 0=)
    "always"
  elseif(cond 100 <=)
    %d "% of the time\n" %s
  elseif(cond 200 <=)
    "if player is carrying " %s wordToName(obj) %s
  elseif(cond 300 <=)
    "if object " %s wordToName(obj) %s " is here" %s
  else
    cond 100 / 3- -> int propValue
    "if object " %s wordToName(obj) %s " does not have property value " %s propValue %d
  endif
  %nl
;
  
enum: eAdventureState
  kStateLoopTop     // 0
  kStateCommence
  kStateInnerLoop
  kStateCycle
  kStatePreParse    // 4
  kStateParse
  kStateIntransitive
  kStateTransitive
  kStateGetObject   // 8
  kStateCantSeeIt
  kStateTryMove
  kStatePitchDark
  kStateDeath       // 12
  kStateQuit
;enum
  

loaddone
