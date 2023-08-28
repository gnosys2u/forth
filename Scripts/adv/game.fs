
\ static variables for class Game
mko Array of AdventureState Game_adventureStates

class: Game extends IGame

  : setState
    mCurStateNum!
  ;
  
  \ remove an object from a locations linked list of objects  
  : removeObjectFrom
    Location loc!
    ObjectWord t!
    Place place
    ObjectData obj
    ObjectData nextObj
    places.get(loc) place!o
    objs(t) obj!o
    if(obj.place loc =)
      if(place.objects t =)
        \ object is head of list
        obj.link place.objects!
      else
        objs(place.objects) nextObj!o
        begin
        while(nextObj.link t <>)
          objs(nextObj.link) nextObj!o
        repeat
        obj.link nextObj.link!
      endif
      NOTHING obj.link!
    else
      \ TODO: report error, object was not in loc to begin with
    endif
  ;
  
  : lookup
    ptrTo byte w!
    \ TODO: lookup Word in words, return word.meaning (an int)
    words.headIter ArrayIter iter!
    NOTHING ObjectWord result!
    begin
    while( iter.next )
      Word word
      word!o
      if(objNotNull(word))
        \ word.text.get %s %bl w %s %nl
        if(streq(word.text.get w))
          word.meaning
          iter~
          exit
        endif
      endif
    repeat
    iter~
    NOTHING
  ;

  : word_class
    int word!
    case(word)
      ofif(word NOTHING =)
        WordClass_None
      endof
  
      ofif(within(word MIN_MOTION MOTION_LIMIT))
        WordClass_Motion
      endof
  
      ofif(within(word MIN_OBJ OBJ_LIMIT))
        WordClass_Object
      endof
  
      ofif(within(word MIN_ACTION ACTION_LIMIT))
        WordClass_Action
      endof
  
      ofif(within(word MIN_MESSAGE MESSAGE_LIMIT))
        WordClass_Message
      endof
  
      "word_class: unrecognized word " %s dup %d %nl
      WordClass_None swap
      
    endcase
  ; 

  \ ========== Data structures for objects. =================================
  \ This section corresponds to sections 63--70 in Knuth.
  
  : toting objs.place 0< ;                  \ OBJID ... TRUE/FALSE
  : is_immobile objs.base NOTHING <> ;      \ OBJID ... TRUE/FALSE
  : there                                   \ OBJID LOCID ... TRUE/FALSE
    swap objs.place =
  ;
  
  : showObjectsAtLocation
    Location loc!
    locationToName(loc) %s " has " %s
    Place place
    ObjectData obj
    places.get(loc) place!o
    place.objects ObjectWord objWord!
    begin
    while(objWord NOTHING <>)
      wordToName(objWord) %s %bl
      objs(objWord) obj!o
      obj.link objWord!
    repeat
    %nl
  ;

  \ Return true if t is a treasure. Notice that RUG_ (the other half
  \ of the schizoid rug) does not need to be a treasure. You can
  \ never be carrying it; the pirate can't steal it; and its prop
  \ value is irrelevant (we use the prop value of the base object RUG).
  
  : is_treasure
    case
      of(GOLD) of(DIAMONDS) of(SILVER) of(JEWELS)
      of(COINS) of(CHEST) of(EGGS) of(TRIDENT)
      of(VASE) of(EMERALD) of(PYRAMID) of(PEARL)
      of(RUG) of(SPICES) of(CHAIN)
        true
      endof
      false swap
    endcase
  ;
  
  : bottle_contents
    case(objs(BOTTLE).prop)
      of(0) WATER endof
      of(2) OIL endof
      \ other valid values: 1, -2 (post-closing)
      NOTHING swap
    endcase
  ;
  
  \ Return true if t is at loc, or being carried.
  : here
    Location loc!
    ObjectWord t!
    or(toting(t) there(t loc))
  ;
  
  : dropObject
    Location loc!
    ObjectWord t!
    ObjectData obj
    Place place
    objs(t) obj!o
    \  assert(objs(t).place == R_INHAND || objs(t).place == R_LIMBO) 
    \  assert(objs(t).link == NOTHING) 
    if(toting(t))
      holding_count--
    endif
    loc obj.place!
    \ "dropped " %s t wordToName %s " at " %s loc locationToName %s %nl
    if(loc R_INHAND =)
      holding_count++
    elseif(loc R_LIMBO <>)
      places.get(loc) place!o
      place.objects obj.link!
      t place.objects!
    endif
  ;
  
  : carry       \ OBJECT
    ObjectWord t!
    ObjectData obj
    objs(t) obj!o
    obj.place Location loc!
    if(loc  R_INHAND <>)
      if(loc R_LIMBO >)
        \ Remove t from loc's object-list
        removeObjectFrom(t loc)
        R_INHAND obj.place!
        holding_count++
      endif
    endif
  ;
  
spoo before move
  : move        \ OBJECT LOC
    Location loc!
    ObjectWord t!
    carry(t)
    dropObject(t loc)
  ;
  
  : juggle
    ObjectWord t!
    objs(t).place Location loc!
    move(t loc)
  ;
  
  : destroy  \ t
    R_LIMBO move
  ;
  
  : is_at_loc
    Location loc!
    ObjectWord t!
    false bool result!
    if(objs(t).base NOTHING =)
      there(t loc) result!
    else
      \ Check the "alternative" objects based on this one.
      t ObjectWord tt!
      begin
      while(objs(tt).base t =)
        if(there(tt loc))
          true  exit
        endif
        tt++
      repeat
    endif
    result
  ;
  
  : mobilize
    objs ObjectData obj!
    NOTHING obj.base!
  ;
  
  : immobilize
    ObjectWord t!
    objs(t) ObjectData obj!
    t obj.base!
  ;
  
  int __objIndex
  : new_obj   \ ObjectWord t, const char *n, ObjectWord b, Location l
    mko ObjectData obj
    obj.init
    ObjectWord t!
    _objs.set(obj t MIN_OBJ -)
    d[ "new_obj " %s t %d %bl obj.place %d %bl obj.name.get %s %nl ds ]d
    if(is_treasure(t))  -1  else  0  endif obj.prop!
    NOTHING obj.link!
    if(obj.place R_LIMBO >)
      \ Drop the object at the *end* of its list. Combined with the
      \ ordering of the item numbers, this ensures that the CHASM
      \ is described before the TROLL, the DRAGON before the RUG, and so on.
      addObjectTo(t obj.place)
    endif
    obj~
  ;
  : addObjDesc
    mko String description
    description.set
    d[ "addObjDesc " %s description.get %s %nl ]d
    ObjectData obj
    objs obj!o
    obj.desc.push(description)
    description~
  ;
  
spoo before build_object_table
  m: build_object_table

    new_obj(KEYS  "Set of keys" 0  R_HOUSE) 
    addObjDesc(KEYS "There are some keys on the ground here.")
    new_obj(LAMP  "Brass lantern" 0  R_HOUSE) 
    addObjDesc(LAMP "There is a shiny brass lamp nearby.")
    addObjDesc(LAMP "There is a lamp shining nearby.")
    new_obj(GRATE  0  GRATE  R_OUTSIDE) 
    new_obj(GRATE_  0  GRATE  R_INSIDE) 
    addObjDesc(GRATE "The grate is locked.")
    addObjDesc(GRATE "The grate is open.")
    new_obj(CAGE  "Wicker cage" 0  R_COBBLES) 
    addObjDesc(CAGE "There is a small wicker cage discarded nearby.")
    new_obj(ROD  "Black rod" 0  R_DEBRIS) 
    addObjDesc(ROD "A three-foot black rod with a rusty star on an end lies nearby.")
    new_obj(ROD2  "Black rod" 0  R_LIMBO) 
    addObjDesc(ROD2 "A three-foot black rod with a rusty mark on an end lies nearby.")
    new_obj(TREADS  0  TREADS  R_SPIT) 
    new_obj(TREADS_  0  TREADS  R_EMIST) 
    addObjDesc(TREADS "Rough stone steps lead down the pit.")
    addObjDesc(TREADS "Rough stone steps lead up the dome.")
    new_obj(BIRD  "Little bird in cage" 0  R_BIRD) 
    addObjDesc(BIRD "A cheerful little bird is sitting here singing.")
    addObjDesc(BIRD "There is a little bird in the cage.")
    new_obj(RUSTY_DOOR  0  RUSTY_DOOR  R_IMMENSE) 
    addObjDesc(RUSTY_DOOR "The way north is barred by a massive, rusty, iron door.")
    addObjDesc(RUSTY_DOOR "The way north leads through a massive, rusty, iron door.")
    new_obj(PILLOW  "Velvet pillow" 0  R_SOFT) 
    addObjDesc(PILLOW "A small velvet pillow lies on the floor.")
    new_obj(SNAKE  0  SNAKE  R_HMK) 
    addObjDesc(SNAKE "A huge green fierce snake bars the way!")
    addObjDesc(SNAKE null)  \ chased away
    new_obj(FISSURE  0  FISSURE  R_EFISS) 
    new_obj(FISSURE_  0  FISSURE  R_WFISS) 
    addObjDesc(FISSURE null)
    addObjDesc(FISSURE "A crystal bridge now spans the fissure.")
    new_obj(TABLET  0  TABLET  R_DARK) 
    \ Woods has "imbedded".
    addObjDesc(TABLET "A massive stone tablet embedded in the wall reads:\n\+
\"CONGRATULATIONS ON BRINGING LIGHT INTO THE DARK-ROOM!\"")
    new_obj(CLAM  "Giant clam >GRUNT!<"  0  R_SHELL) 
    addObjDesc(CLAM "There is an enormous clam here with its shell tightly closed.")
    new_obj(OYSTER  "Giant oyster >GROAN!<"  0  R_LIMBO) 
    addObjDesc(OYSTER "There is an enormous oyster here with its shell tightly closed.")
    new_obj(MAG  "\"Spelunker Today\""  0  R_ANTE) 
    addObjDesc(MAG "There are a few recent issues of \"Spelunker Today\" magazine here.")
    new_obj(DWARF  0  DWARF  R_LIMBO) 
    new_obj(KNIFE  0  0  R_LIMBO) 
    new_obj(FOOD  "Tasty food" 0  R_HOUSE) 
    addObjDesc(FOOD "There is food here.")
    new_obj(BOTTLE  "Small bottle" 0  R_HOUSE) 
    addObjDesc(BOTTLE "There is a bottle of water here.")
    addObjDesc(BOTTLE "There is an empty bottle here.")
    addObjDesc(BOTTLE "There is a bottle of oil here.")
    \ WATER and OIL never appear on the ground) they are invariably
    \ either in R_LIMBO or R_INHAND.
    new_obj(WATER  "Water in the bottle" 0  R_LIMBO) 
    new_obj(OIL  "Oil in the bottle" 0  R_LIMBO) 
    new_obj(MIRROR  0  MIRROR  R_MIRROR) 
    new_obj(MIRROR_  0  MIRROR  R_LIMBO)   \ joins up with MIRROR later
    addObjDesc(MIRROR null)
    new_obj(PLANT  0  PLANT  R_WPIT) 
    addObjDesc(PLANT "There is a tiny little plant in the pit, murmuring \"Water, water, ...\"")
    addObjDesc(PLANT "There is a 12-foot-tall beanstalk stretching up out of the pit,\n\+
bellowing \"Water!! Water!!\"")
    addObjDesc(PLANT "There is a gigantic beanstalk stretching all the way up to the hole.")
    new_obj(PLANT2  0  PLANT2  R_W2PIT) 
    new_obj(PLANT2_  0  PLANT2  R_E2PIT) 
    addObjDesc(PLANT2 null)
    addObjDesc(PLANT2 "The top of a 12-foot-tall beanstalk is poking out of the west pit.")
    addObjDesc(PLANT2 "There is a huge beanstalk growing out of the west pit up to the hole.")
    new_obj(STALACTITE  0  STALACTITE  R_TITE) 
    addObjDesc(STALACTITE null)
    new_obj(SHADOW  0  SHADOW  R_WINDOE) 
    new_obj(SHADOW_  0  SHADOW  R_WINDOW) 
    addObjDesc(SHADOW "The shadowy figure seems to be trying to attract your attention.")
    new_obj(AXE  "Dwarf's axe" 0  R_LIMBO) 
    addObjDesc(AXE "There is a little axe here.")
    addObjDesc(AXE "There is a little axe lying beside the bear.")
    new_obj(DRAWINGS  0  DRAWINGS  R_ORIENTAL) 
    addObjDesc(DRAWINGS null)
    new_obj(PIRATE  0  PIRATE  R_LIMBO) 
    new_obj(DRAGON  0  DRAGON  R_SCAN1) 
    new_obj(DRAGON_  0  DRAGON  R_SCAN3) 
    addObjDesc(DRAGON "A huge green fierce dragon bars the way!")
    addObjDesc(DRAGON "The body of a huge green dead dragon is lying off to one side.")
    new_obj(CHASM  0  CHASM  R_SWSIDE) 
    new_obj(CHASM_  0  CHASM  R_NESIDE) 
    addObjDesc(CHASM "A rickety wooden bridge extends across the chasm, vanishing into the\n\+
mist. A sign posted on the bridge reads, \"STOP! PAY TROLL!\"")
    addObjDesc(CHASM "The wreckage of a bridge (and a dead bear) can be seen at the bottom\n\+
of the chasm.")
    new_obj(TROLL  0  TROLL  R_SWSIDE) 
    new_obj(TROLL_  0  TROLL  R_NESIDE) 
    addObjDesc(TROLL "A burly troll stands by the bridge and insists you throw him a\n\+
treasure before you may cross.")
    addObjDesc(TROLL null)  \ not present, but not paid off either
    addObjDesc(TROLL null)  \ chased away
    new_obj(NO_TROLL  0  NO_TROLL  R_LIMBO) 
    new_obj(NO_TROLL_  0  NO_TROLL  R_LIMBO) 
    addObjDesc(NO_TROLL "The troll is nowhere to be seen.")
    new_obj(BEAR  0  BEAR  R_BARR) 
    addObjDesc(BEAR "There is a ferocious cave bear eying you from the far end of the room!")
    addObjDesc(BEAR "There is a gentle cave bear sitting placidly in one corner.")
    addObjDesc(BEAR "There is a contented-looking bear wandering about nearby.")
    addObjDesc(BEAR null)  \ the dead bear remains as scenery where it fell
    new_obj(MESSAGE  0  MESSAGE  R_LIMBO) 
    addObjDesc(MESSAGE "There is a message scrawled in the dust in a flowery script, reading:\n\+
\"This is not the maze where the pirate hides his treasure chest.\"")

    new_obj(GORGE  0  GORGE  R_VIEW) 
    addObjDesc(GORGE null)  \ it's just scenery
    new_obj(MACHINE  0  MACHINE  R_PONY) 
    addObjDesc(MACHINE "There is a massive vending machine here. The instructions on it read:\n\+
\"Drop coins here to receive fresh batteries.\"")
    new_obj(BATTERIES  "Batteries" 0  R_LIMBO) 
    addObjDesc(BATTERIES "There are fresh batteries here.")
    addObjDesc(BATTERIES "Some worn-out batteries have been discarded nearby.")
    new_obj(MOSS  0  MOSS  R_SOFT) 
    addObjDesc(MOSS null)  \ it's just scenery
    new_obj(GOLD  "Large gold nugget" 0  R_NUGGET) 
    addObjDesc(GOLD "There is a large sparkling nugget of gold here!")
    new_obj(DIAMONDS  "Several diamonds" 0  R_WFISS) 
    addObjDesc(DIAMONDS "There are diamonds here!")
    new_obj(SILVER  "Bars of silver" 0  R_NS) 
    addObjDesc(SILVER "There are bars of silver here!")
    new_obj(JEWELS  "Precious jewelry" 0  R_SOUTH) 
    addObjDesc(JEWELS "There is precious jewelry here!")
    new_obj(COINS  "Rare coins" 0  R_WEST) 
    addObjDesc(COINS "There are many coins here!")

    new_obj(CHEST  "Treasure chest" 0  R_LIMBO) 
    addObjDesc(CHEST "The pirate's treasure chest is here!")
    new_obj(EGGS  "Golden eggs" 0  R_GIANT) 
    addObjDesc(EGGS "There is a large nest here  full of golden eggs!")
    new_obj(TRIDENT  "Jeweled trident" 0  R_FALLS) 
    addObjDesc(TRIDENT "There is a jewel-encrusted trident here!")
    new_obj(VASE  "Ming vase" 0  R_ORIENTAL) 
    addObjDesc(VASE "There is a delicate, precious, Ming vase here!")
    addObjDesc(VASE "The floor is littered with worthless shards of pottery.")
    new_obj(EMERALD  "Egg-sized emerald" 0  R_PLOVER) 
    addObjDesc(EMERALD "There is an emerald here the size of a plover's egg!")
    new_obj(PYRAMID  "Platinum pyramid" 0  R_DARK) 
    addObjDesc(PYRAMID "There is a platinum pyramid here, 8 inches on a side!")
    new_obj(PEARL  "Glistening pearl" 0  R_LIMBO) 
    addObjDesc(PEARL "Off to one side lies a glistening pearl!")
    new_obj(RUG_  0  RUG  R_SCAN3) 
    new_obj(RUG  "Persian rug" RUG  R_SCAN1) 
    addObjDesc(RUG "There is a Persian rug spread out on the floor!")
    addObjDesc(RUG "The dragon is sprawled out on a Persian rug!!")
    new_obj(SPICES  "Rare spices" 0  R_CHAMBER) 
    addObjDesc(SPICES "There are rare spices here!")
    new_obj(CHAIN  "Golden chain" CHAIN  R_BARR) 
    addObjDesc(CHAIN "There is a golden chain lying in a heap on the floor!")
    addObjDesc(CHAIN "The bear is locked to the wall with a golden chain!")
    addObjDesc(CHAIN "There is a golden chain locked to the wall!")
  ;m

  : writeToSaveFile
    ptrTo byte line!
    if(objIsNull(saveStream))
      \ this is first write to save file, create the file, output the line, then add the seed string
      new FileOutStream saveStream!
      saveStream.open(saveFileName.get "w") drop
      saveStream.putString(line)
      if(objIsNull(restoreStream))    \ don't add the seed string line if we are restoring
        mko String seedString
        seedString.appendFormatted("seed %d" getRandomSeed 1)
        saveStream.putLine(seedString.get)
        seedString~
      endif
    else
      saveStream.open(saveFileName.get "a") drop
      saveStream.putString(line)
    endif
    saveStream.close
  ;
    
  \ ========== Input routines. ==============================================
  \ This section corresponds to sections 71--73 in Knuth.

  : getInputLine
    dup if %s endif
    if(objNotNull(restoreStream))
      restoreStream
    else
      consoleInStream
    endif
    InStream inStream
    inStream!o
    inStream.setTrimEOL(false)
    0 inBuffer.base c!
    inStream.getLine(inBuffer.base BUF_SIZE) drop
    if(objNotNull(restoreStream))
      if(restoreStream.atEOF)
        restoreStream.close
        restoreStream~
      endif
      inBuffer.base %s
    endif
    \ fgets(inBuffer.base BUF_SIZE stdin) drop
    inBuffer.base
  ;
    
  : yes  \ takes const char *q, const char *y, const char *n, returns bool
    ptrTo byte n!
    ptrTo byte y!
    ptrTo byte q!
    begin
      q %s
      getInputLine("\n**") ptrTo byte answer!
      writeToSaveFile(answer)
      if(tolower(answer c@) 'y' =)
        if(y null <>)
          y %s %nl
        endif
        true
        exit
      else
        if(tolower(answer c@) 'n' =)
          if(n null <>)
            n %s %nl
            endif
            false
            exit
        else
          " Please answer Yes or No.\n" %s
        endif
      endif
    again
  ;
  
spoo before listen  
  : listen
    ptrTo byte p
    ptrTo byte q
    begin
continue:
      getInputLine("* ") p!
      begin
      while(p c@ isspace)  \ skip over whitespace
        p++
      repeat
      if(p c@ 0=)
        " Tell me to do something.\n" %s
        continue
      endif
      \ Notice that this algorithm depends on the buffer's being
      \ terminated by "\n\0" or at least some whitespace character.
      inWord1.base q!
      begin
      while(not(isspace(p c@)))
        tolower(p c@) q c!
        p++   q++
      repeat
      '\z' q c!
      p++
      begin
      while(isspace(p c@))
        p++
      repeat
      inWord2.base q!
      if(p c@ 0=)
        '\z' q c!
        exit
      endif
      begin
      while(not(isspace(p c@)))
        tolower(p c@) q c!
        p++   q++
      repeat
      '\z' q c!
      p++
      begin
      while(isspace(p c@))
        p++
      repeat
      if(p c@ 0=)
        exit
      endif
      " Please stick to 1- and 2-word commands.\n" %s
    again
    "done listen\n" %s
  ;

  : shift_words
    strcpy(inWord1.base inWord2.base) 
    '\z' inWord2.base c!
  ;


  \ ========== Dwarves and pirate. ==========================================
  \ This section corresponds to sections 159--175 in Knuth.

  : addDwarf
    mko Dwarf dwarf
    dwarf.loc!
    R_LIMBO dwarf.oldloc!
    false dwarf.seen!
    dwarves.count dwarf.index!
    dwarves.push(dwarf)
    dwarf~
  ;

  : initDwarves
    addDwarf(R_PIRATES_NEST)  \ this one is really the pirate
    addDwarf(R_HMK)
    addDwarf(R_WFISS)
    addDwarf(R_Y2)
    addDwarf(R_LIKE3)
    addDwarf(R_COMPLEX)
    dwarves.get(0) pirate!
  ;

  : dwarf_at  \ is a dwarf present? Section 160 in Knuth.
    Location loc!
    if(dwarfAngerLevel 2 <)
      false
      exit
    endif
    do(6 0)
      if(loc dwarves.get(i).loc =)
        true
        unloop
        exit
      endif
    loop
    false
  ;

  : return_pirate_to_lair
    bool with_chest!
    if(with_chest)
      dropObject(CHEST R_PIRATES_NEST) 
      dropObject(MESSAGE R_PONY) 
    endif
    R_PIRATES_NEST dup pirate.loc! pirate.oldloc!
    false pirate.seen!
  ;

  : too_easy_to_steal
    Location loc!
    ObjectWord t!
    and(t PYRAMID = or(loc R_PLOVER =  loc R_DARK =))
  ;

  : steal_all_your_treasure  \ sections 173--174 in Knuth
    Location loc!
    puts("Out from the shadows behind you pounces a bearded pirate!  \"Har, har,\"\n\+
he chortles. \"I'll just take all this booty and hide it away with me\n\+
chest deep in the maze!\"  He snatches your treasure and vanishes into\n\+
the gloom.")
    do(MAX_OBJ 1+ MIN_OBJ)
      continueIf(not(is_treasure(i)))
      continueIf(too_easy_to_steal(i loc))
      if(here(i loc))  andif(not(is_immobile(i)))
        \ The vase, rug, and chain can all be immobile at times.
        move(i R_PIRATES_NEST)
      endif
continue:
    loop
  ;

  : pirate_tracks_you
    Location loc!
    there(MESSAGE R_LIMBO) bool chest_needs_placing!
    false bool stalking!
    \ The pirate leaves you alone once you've found the chest.
    if(loc R_PIRATES_NEST =) orif(objs(CHEST).prop 0>=)
      exit
    endif
    do(MAX_OBJ 1+ MIN_OBJ)
      continueIf(not(is_treasure(i)))
      continueIf(too_easy_to_steal(i loc))
      if(toting(i))
        steal_all_your_treasure(loc) 
        return_pirate_to_lair(chest_needs_placing) 
        unloop
        exit
      endif
      if(there(i loc))
        \ There is a treasure in this room, but we're not carrying
        \ it. The pirate won't pounce unless we're carrying the
        \ treasure; so he'll try to remain quiet.
        true stalking!
      endif
continue:
    loop
    \ tally is the number of treasures we haven't seen; lost_treasures is
    \ the number we never will see (due to killing the bird or destroying
    \ the troll bridge).
    if(tally lost_treasures 1+ =) andif(not(stalking)) andif(chest_needs_placing)
     andif(objs(LAMP).prop) andif(here(LAMP loc))
      \ As soon as we've seen all the treasures (except the ones that are
      \ lost forever), we "cheat" and let the pirate be spotted. Of course
      \ there have to be shadows to hide in, so check the lamp.
      puts("There are faint rustling noises from the darkness behind you. As you\n\+
turn toward them, the beam of your lamp falls across a bearded pirate.\n\+
He is carrying a large chest. \"Shiver me timbers!\" he cries, \"I've\n\+
been spotted! I'd best hie meself off to the maze to hide me chest!\"\n\+
With that, he vanishes into the gloom.")
      return_pirate_to_lair(true)
      exit
    endif
    if(pirate.oldloc pirate.loc <>) andif(pct(20))
      puts("There are faint rustling noises from the darkness behind you.")
    endif
  ;

  : forbidden_to_pirate \ (Location loc)
    R_PIRATES_NEST >
  ;

  \ Return true if the player got killed by a dwarf this turn.
  \ This function represents sections 161--168, 170--175 in Knuth.
  : move_dwarves_and_pirate
    Location loc!
    false bool playerWasKilled!
    Dwarf dwarf
    \ assert(R_LIMBO <= loc && loc <= MAX_LOC) 
    if(forbidden_to_pirate(loc)) orif(loc R_LIMBO =)
      \ Bypass all dwarf motion if you are in a place forbidden to the
      \ pirate, or if your next motion is forced. Besides the cases that
      \ Knuth mentions (dwarves can't meet the bear, dwarves can't enter
      \ most dead ends), this also prevents the axe-toting dwarf from
      \ showing up in the middle of a forced move and dropping the axe
      \ in an inaccessible pseudo-location.
    elseif(dwarfAngerLevel 0=)
      if(loc MIN_LOWER_LOC >=) 1 dwarfAngerLevel! endif
    elseif(dwarfAngerLevel 1 =)
      if(loc MIN_LOWER_LOC >=) andif(pct(5))
        \ When level 2 of the cave is reached, we silently kill 0, 1,
        \ or 2 of the dwarves. Then if any of the survivors is in
        \ the current location, we move him to R_NUGGET; thus no
        \ dwarf is presently tracking you. Another dwarf does,
        \ however, toss an axe and grumpily leave the scene.
        2 dwarfAngerLevel!
        if(pct(50)) dwarves.get(ran(5) 1+).setLoc(R_LIMBO) endif
        if(pct(50)) dwarves.get(ran(5) 1+).setLoc(R_LIMBO) endif
        do(6 1)
          dwarves.get(i) dwarf!o
          if(dwarf.loc loc =) dwarf.setLoc(R_NUGGET) endif
          dwarf.loc dwarf.oldloc!
        loop
        \ Knuth quietly fixes the garden-path grammar here:
        \   A little dwarf just walked around a corner, saw you, threw a
        \   little axe at you, cursed, and ran away. (The axe missed.)
        \ But Woods' version matches Crowther's original source code,
        \ and I don't think the deviation is justified.
        puts("A little dwarf just walked around a corner, saw you, threw a little\n\+
axe at you which missed, cursed, and ran away.")
        dropObject(AXE loc)
      endif
    else
      \ Move dwarves and the pirate.
      0 int dtotal!   \ this many dwarves are in the room with you
      0 int attack!   \ this many have had time to draw their knives
      0 int stick!    \ this many have hurled their knives accurately
      20 arrayOf Location ploc
      do(6 0)
        dwarves.get(i) dwarf!o
        0 int ii!
        if(dwarf.loc R_LIMBO <>)
          \ Make a table of all potential exits.
          \ Dwarves think R_SCAN1, R_SCAN2, R_SCAN3 are three different locations,
          \ although you will never have that perception.
          start(dwarf.loc) ptrTo Instruction q!
          begin
          while(q start(dwarf.loc 1+) <)
            q.dest Location newloc!
            if(ii 0<>) andif(newloc ploc(ii 1-) =)
              continue
            endif
            continueIf(newloc MIN_LOWER_LOC <)  \ don't wander above level 2
            continueIf(or(newloc dwarf.oldloc =  newloc dwarf.loc =))  \ don't double back
            continueIf(q.cond 100 =)
            if(dwarf pirate =)
              continueIf(forbidden_to_pirate(newloc))
            endif
            continueIf(or(is_forced(newloc)  newloc MAX_LOC >))
            newloc ploc!(ii)
            ii++
continue:
            sizeOf Instruction q!+
          repeat
          if(ii 0=)
            dwarf.oldloc ploc!(ii)
            ii++
          endif
          dwarf.loc dwarf.oldloc!
          dwarf.setLoc(ploc(ran(ii))) \ this is the random walk
              
          \ Dwarves follow the player once they've spotted him. But
          \ they won't follow outside the lower cave.
          if(dwarf.loc loc =) orif(dwarf.oldloc loc =)
            true dwarf.seen!
          else
            if(loc MIN_LOWER_LOC <)
              false dwarf.seen!
            endif
          endif
              
          if(dwarf.seen)
            \ Make dwarf d follow 
            loc dwarf.loc!
            if(dwarf pirate =)
              pirate_tracks_you(loc) 
            else
              dtotal++
              if(dwarf.oldloc dwarf.loc =)
                attack++
                loc last_knife_loc!
                if(ran(1000) 95 dwarfAngerLevel 2- * <) stick++ endif
              endif
            endif
          endif
        endif
      loop
          
      if(dtotal 0<>)
        \ Make the threatening dwarves attack.
        if(dtotal 1 =)
          puts("There is a threatening little dwarf in the room with you!")
        else 
          "There are " %s dtotal %d " threatening little dwarves in the room with you!\n" %s
        endif
        if(attack)
          if(dwarfAngerLevel 2 =) 3 dwarfAngerLevel! endif
          if(attack 1 =)
            puts("One sharp nasty knife is thrown at you!")
            if(stick 0=)
              puts("It misses!")
            else
              puts("It gets you!")
            endif
          else
            attack %d " of them throw knives at you!\n" %s 
            if(stick 0=)
              puts("None of them hit you!")
            else
              if(stick 1 =)
                puts("One of them gets you!")
              else
                stick %d " of them get you!\n" %s
              endif
            endif
                
            if(stick)
              true playerWasKilled! \ goto death
            endif
                
          endif
              
        endif \ if attack
      endif  \ dtotal nonzero
    endif  \ else dwarfAngerLevel > 1
    playerWasKilled
  ;


  \ ========== Closing the cave. ============================================
  \ This section corresponds to sections 103, 178--181 in Knuth.
  
spoo before cave is closing
  : cave_is_closing
    clock1 0<
  ;
  
  : closeMove
    int prop!
    Location loc!
    ObjectWord t!
    
    move(t loc)
    objs(t).setProp(prop)
  ;
    
  : close_the_cave
    \ Woods has "entones" but that's not a word, so I'm changing it.
    \ Knuth writes "Then your eyes refocus;" in place of "As your eyes
    \ refocus," but I don't see any reason to deviate from Woods'
    \ wording there. Maybe Knuth was working from a slightly earlier
    \ or later version than the one I'm looking at.
    puts("The sepulchral voice intones, \"The cave is now closed.\"  As the echoes\n\+
fade, there is a blinding flash of light (and a small puff of orange\n\+
smoke). . . .    As your eyes refocus, you look around and find...")
    closeMove(BOTTLE R_NEEND -2)     \ empty
    closeMove(PLANT R_NEEND -1)
    closeMove(OYSTER R_NEEND -1)
    closeMove(LAMP R_NEEND -1)
    closeMove(ROD R_NEEND -1)
    closeMove(DWARF R_NEEND -1)
    closeMove(MIRROR R_NEEND -1)
    closeMove(GRATE R_SWEND 0)
    closeMove(SNAKE R_SWEND -2)     \ not blocking the way
    closeMove(BIRD R_SWEND -2)      \ caged
    closeMove(CAGE R_SWEND -1)
    closeMove(ROD2 R_SWEND -1)
    closeMove(PILLOW R_SWEND -1)
    move(MIRROR_ R_SWEND) 
    do(MAX_OBJ 1+ MIN_OBJ)
      if(toting(i))
        destroy(i)
      endif
    loop
    true closed!
    10 bonus!
  ;

  \ Return true if the cave just closed.
  : check_clocks_and_lamp
    Location loc!
    Dwarf dwarf
    if(tally 0=) andif(loc MIN_LOWER_LOC >=) andif(loc R_Y2 <>)
      clock1--
    endif
    if(clock1 0=)
      \ At the time of first warning, we lock the grate, destroy the
      \ crystal bridge, kill all the dwarves (and the pirate), and
      \ remove the troll and bear (unless dead).
      \ It's too much trouble to move the dragon, so we leave it.
      \ From now on until clock2 runs out, you cannot unlock the grate,
      \ move to any location outside the cave, or create the bridge.
      \ Nor can you be resurrected if you die.
      puts("A sepulchral voice, reverberating through the cave, says \"Cave\n\+
closing soon.  All adventurers exit immediately through main office.\"")
      -1 clock1!
      objs(GRATE).setProp(0)
      objs(FISSURE).setProp(0)
      do(6 0)
        dwarves.get(i) dwarf!o
        false dwarf.seen!
        R_LIMBO dwarf.loc!
      loop
      destroy(TROLL)  destroy(TROLL_) 
      move(NO_TROLL R_SWSIDE)  move(NO_TROLL_ R_NESIDE) 
      juggle(CHASM)  juggle(CHASM_) 
      if(objs(BEAR).prop 3 <>) destroy(BEAR) endif
      objs(CHAIN).setProp(0) mobilize(CHAIN) 
      objs(AXE).setProp(0) mobilize(AXE) 
    else
      if(cave_is_closing) clock2-- endif
      if(clock2 0=)
        close_the_cave
        true
        exit
      else 
        \ On every turn (if the cave is not closed), we check to see
        \ if you are in trouble lampwise.
        if(objs(LAMP).prop 1 =) lamp_limit-- endif
        if(lamp_limit 30 <=) andif(here(LAMP loc)) andif(here(BATTERIES loc)) andif(objs(BATTERIES).prop 0=)
          puts("Your lamp is getting dim.  I'm taking the liberty of replacing\nthe batteries.")
          objs(BATTERIES).setProp(1)
          if(toting(BATTERIES)) dropObject(BATTERIES loc) endif
          2500 lamp_limit!
        elseif(lamp_limit 0=)
          if(here(LAMP loc)) puts("Your lamp has run out of power.") endif
          objs(LAMP).setProp(0)
          -1 lamp_limit!
        elseif(lamp_limit 0<) andif(loc MIN_IN_CAVE <)
          
          puts("There's not much point in wandering around out here, and you can't\n\+
explore the cave without a lamp.  So let's just call it a day.")
          give_up
        elseif(lamp_limit 30 <) andif(not(warnedOfClosing)) andif(here(LAMP loc))
          "Your lamp is getting dim" %s
          if(objs(BATTERIES).prop 1 =)
            puts(", and you're out of spare batteries.  You'd\nbest start wrapping this up.")
          elseif(there(BATTERIES R_LIMBO))
            puts(".  You'd best start wrapping this up, unless\n\+
you can find some fresh batteries.  I seem to recall that there's\n\+
a vending machine in the maze.  Bring some coins with you.")
          else
            puts(".  You'd best go back for those batteries.")
          endif
          true warnedOfClosing!
        endif
      endif
    endif
    false
  ;

  : panic_at_closing_time
    \ If you try to get out while the cave is closing, we assume that
    \ you panic; we give you a few additional turns to get frantic
    \ before we close.
    if(not(closingPanic))
      15 clock2!
      true closingPanic!
    endif
    puts("A mysterious recorded voice groans into life and announces:\n\+
   \"This exit is closed.  Please leave via main office.\"")
    \ Woods does NOT set "was_dark = false" at this point.
    \ This means that if you've gotten into the habit of turning
    \ off your lamp before you use a magic word to teleport out
    \ of the cave, we might well add injury to insult by causing
    \ you to fall into a pit and die (thus ending the game) right
    \ at this point.
  ;

  \ ========== The main loop. ===============================================
  \ This section corresponds to sections 74--75 in Knuth.

  : give_optional_plugh_hint
    Location loc!
    if(loc R_Y2 =) andif(pct(25)) andif(not(cave_is_closing()))
      puts("A hollow voice says \"PLUGH\".")
    endif
  ;

  m: offer
    int hintNum!
    Hint hint
    hints.get(hintNum) hint!o
    if(hintNum 1 >)
      if(not(yes(hint.prompt.get " I am prepared to give you a hint," okMsg)))
        exit
      endif
      " but it will cost you " %s hint.cost %d " points.  " %s 
      yes("Do you want the hint?" hint.hint.get okMsg) hint.given!
    
    else
      yes(hint.prompt.get hint.hint.get okMsg) hint.given!
    endif
    if(and(hint.given  lamp_limit 30 >))
      \ Give the lamp some more power.
      30 hint.cost * lamp_limit!+
    endif
  ;m
  
spoo before spot_treasure
  : spot_treasure
    ObjectWord t!
    if(objs(t).prop 0>=)
      exit
    endif
    \ assert(is_treasure(t) && !closed)   \ You've spotted a treasure
    case(t)
      of(RUG)  \ trapped
        objs(t).setProp(1)
      endof
      of(CHAIN)  \ locked
        objs(t).setProp(1)
      endof
      objs(t).setProp(0)
    endcase
    tally--
    if(tally lost_treasures =) andif(tally 0>) andif(lamp_limit 35 >)
      \ Zap the lamp if the remaining treasures are too elusive
      35 lamp_limit!
    endif
  ;

  : describe_object
    Location loc!
    ObjectWord t!
    if(t TREADS =) andif(toting(GOLD))
      \ The rough stone steps disappear if we are carrying the nugget.
      exit
    endif
    \ I'm not sure going_up is correct - how does a bool->int cast work in C99? true/false -> 0/1?
    if(and(t TREADS =  loc R_EMIST =)) 1 else 0 endif int going_up!
    String obj_description
    objs(t).desc.get(objs(t).prop going_up +) obj_description!
    if(objNotNull(obj_description))
      puts(obj_description.get) 
    endif
  ;

  : look_around
    bool was_dark!
    bool dark!
    Location loc!
    ObjectData obj
    places.get(loc) Place place!
    null ptrTo byte room_description!
    if(dark) andif(not(is_forced(loc)))
      if(was_dark) andif(pct(35))
        'p'  \ goto pitch_dark;
        exit
      endif
      pitchDarkMsg room_description!
    elseif(place.short_desc.length 0=) orif(mod(place.visits verbose_interval) 0=)
      place.long_desc.get room_description!
    else
      place.short_desc.get room_description!
    endif
    if(toting(BEAR))
      puts("You are being followed by a very large, tame bear.")
    endif
    if(room_description null <>)
      \ R_CHECK's description is null.
      %nl room_description %s %nl
    endif
    if(is_forced(loc))
      't'  \ goto try_move;
      exit
    endif
    give_optional_plugh_hint(loc) 
    if(not(dark))
      place.visits++
      \ Describe the objects at this location.
      place.objects ObjectWord t!
      begin
      while(t NOTHING <>)
        objs(t) obj!o
        if(obj.base) obj.base else t endif ObjectWord tt!
        if(closed)
          continueIf(objs(tt).prop 0<)  \ scenery objects
        endif
        spot_treasure(tt) 
        describe_object(tt loc)
continue:
        objs(t).link t!
      repeat
    endif
    0  \ just continue normally
  ;

  spoo maybe_give_a_hint
  : maybe_give_a_hint \ (Location loc, Location oldloc, Location oldoldloc, ObjectWord oldobj)
    ObjectWord oldobj!
    Location oldoldloc!
    Location oldloc!
    Location loc!
    Hint hint
    \ Check if a hint applies, and give it if requested.
    F_CAVE_HINT uint k!
    do(8 2)
      hints.get(i) hint!o

      continueIf(hint.given)
    
      if((places.get(loc).flags k and) 0=)
        \ We've left the map region associated with this hint.
        0 hint.count!
        continue
      endif
      \ Count the number of turns spent here.
      hint.count++
      if(hint.count hint.thresh >=)
        case(i)
      
          of(2)  \ How to get into the cave.
            if(not(objs(GRATE).prop)) andif(not(here(KEYS loc)))
              offer(i) 
            endif
            hint.count~
          endof
          of(3)  \ Are you trying to catch the bird?
            \ Notice that this hint is offered even when the bird has
            \ already been caught, if you spend enough time in the bird
            \ chamber carrying the rod and experimenting with the bird.
            \ This behavior is in Woods' Fortran original.
            if(here(BIRD loc)) andif(oldobj BIRD =) andif(toting(ROD))
              offer(i) 
              hint.count~
            endif
          endof
          of(4)  \ How to deal with the snake.
            if(here(SNAKE loc)) andif(not(here(BIRD loc)))
              offer(i) 
            endif
            hint.count~
          endof
          of(5)  \ How to map the twisty passages all alike.
            if(places.get(loc).objects NOTHING =)
            andif(places.get(oldloc).objects NOTHING =)
            andif(places.get(oldoldloc).objects NOTHING =)
            andif(holding_count 1 >)
              offer(i) 
            endif
            hint.count~
          endof
          of(6)  \ How to explore beyond the Plover Room.
            if(objs(EMERALD).prop -1 <>)  andif(objs(PYRAMID).prop -1 =)
              offer(i)
            endif
            hint.count~
          endof
          of(7)  \ How to get out of Witt's End.
            offer(i) 
            hint.count~
          endof
        endcase
      endif
continue:
      k 2* k!
    loop
  ;

  \ ========== Scoring. =====================================================
  \ This section corresponds to sections 193 and 197 in Knuth.

  : score
    2 int s!
    if(dwarfAngerLevel 0<>)
      25 s!+
    endif
    do(MAX_OBJ 1+ MIN_OBJ)
      continueIf(not(is_treasure(i)))
      if(objs(i).prop 0>=)
        2 s!+  \ two points just for seeing a treasure
        if(there(i R_HOUSE)) andif(objs(i).prop 0=)
          if(i CHEST <)
            10 s!+
          elseif(i CHEST =)
            12 s!+
          else
            14 s!+
          endif
        endif
      endif
continue:
    loop
    MAX_DEATHS death_count - 10 * s!+
    if(not(gave_up)) 4 s!+ endif
    if(there(MAG R_WITT)) s++ endif  \ last lousy point
    if(cave_is_closing) 25 s!+ endif \ bonus for making it this far
    bonus s!+
    do(8 0)
      if(hints.get(i).given)
        hints.get(i).cost s!-
      endif
    loop
    s
  ;

  : say_score
    "If you were to quit now, you would score " %s score 4 - %d %nl
    "out of a possible " %s MAX_SCORE %d ".\n" %s
    if(yes("Do you indeed wish to quit now?" okMsg okMsg))
      give_up
    endif
  ;
  
  : addScoreClass
    mko String desc
    desc.set
    classMessages.push(desc)
    desc~
    classScores.push
  ;
  
  : initScoreClasses
    classMessages.clear
    classScores.clear
    addScoreClass(35 "You are obviously a rank amateur.  Better luck next time.")
    addScoreClass(100 "Your score qualifies you as a novice class adventurer.")
    addScoreClass(130 "You have achieved the rating \"Experienced Adventurer\".")
    addScoreClass(200 "You may now consider yourself a \"Seasoned Adventurer\".")
    addScoreClass(250 "You have reached \"Junior Master\" status.")
    addScoreClass(300 "Your score puts you in Master Adventurer Class C.")
    addScoreClass(330 "Your score puts you in Master Adventurer Class B.")
    addScoreClass(349 "Your score puts you in Master Adventurer Class A.")
    addScoreClass(9999 "All of Adventuredom gives tribute to you, Adventurer Grandmaster!")
  ;

  : give_up
    true gave_up!
    quit
  ;
  
  : quit
    \ Print the score and say adieu.
    score int s!
    0 int rank!
    initScoreClasses
    "You scored " %s s %d " out of a possible " %s MAX_SCORE %d ", using " %s turns %d
    " turn" %s if(turns 1 >) 's' %c endif ".\n" %s
 
    begin
    while(classScores.get(rank) s <=)
      rank++
    repeat
    classMessages.get(rank).get %s "\nTo achieve the next higher rating" %s
    if(rank HIGHEST_CLASS <)
      classScores.get(rank) s - int delta!
      ", you need " %s delta %d " more point" %s if(delta 1 >) 's' %c endif ".\n" %s
    else
      puts(" would be a neat trick!\nCongratulations!!")
    endif
    \ exit   \ TODO! - exit game
  ;

  : addDeathWish
    mko String message
    message.set
    deathWishes.push(message)
    message~
  ;
  
  : initDeathWishes
    addDeathWish("Oh dear, you seem to have gotten yourself killed.  I might be able to\n\+
help you out, but I've never really done this before.  Do you want me\n\+
to try to reincarnate you?")

    addDeathWish("All right. But don't blame me if something goes wr......\n\+
                   --- POOF!! ---\n\+
You are engulfed in a cloud of orange smoke.  Coughing and gasping,\n\+
you emerge from the smoke and find....")

    addDeathWish("You clumsy oaf, you've done it again!  I don't know how long I can\n\+
keep this up.  Do you want me to try reincarnating you again?")

    addDeathWish("Okay, now where did I put my orange smoke?....  >POOF!<\n\+
Everything disappears in a dense cloud of orange smoke.")

    addDeathWish("Now you've really done it!  I'm out of orange smoke!  You don't expect\n\+
me to do a decent reincarnation without any orange smoke, do you?")

    addDeathWish("Okay, if you're so smart, do it yourself!  I'm leaving!")
  ;
  
spoo before kill_the_player
  : kill_the_player
    Location last_safe_place!
    death_count++
    if(cave_is_closing)
      puts("It looks as though you're dead.  Well, seeing as how it's so close to closing time anyway, let's just call it a day.")
      quit
      \ TODO: deal with quit
    else
      if(not(yes(deathWishes.get(death_count 2* 2-).get deathWishes.get(death_count 2* 1-).get okMsg)))
       orif(death_count MAX_DEATHS =)
        quit
        \ TODO: deal with quit
      else
        \ At this point you are reborn. 
        if(toting(LAMP)) objs(LAMP).setProp(0) endif
        objs(WATER).setPlace(R_LIMBO)
        objs(OIL).setPlace(R_LIMBO)
        MAX_OBJ int jj!
        begin
        while(jj MIN_OBJ >=)
          if(toting(jj))
            dropObject(jj if(jj LAMP =) R_ROAD else last_safe_place endif)
          endif
          jj--
        repeat
      endif
    endif
  ;

  \ ========== Main loop. ===================================================
  \ This section corresponds to sections 74--158 in Knuth.

  : now_in_darkness
    Location loc!
    true bool result!
    if(has_light(loc))
      false result!
    else
      if(here(LAMP loc)) andif(objs(LAMP).prop)
        false result!
      endif
    endif
    
    result
  ;

  \ Sections 158, 169, 182 in Knuth
  : adjustments_before_listening
    Location loc!
    if(last_knife_loc loc <>)
      \ When we move, the dwarf's vanishing knife goes out of scope.
      R_LIMBO last_knife_loc!
    endif
    if(closed)
      \ After the cave is closed, we look for objects being toted with
      \ prop < 0; their prop value is changed to -1 - prop. This means
      \ they won't be described until they've been picked up and put
      \ down, separate from their respective piles. Section 182 in Knuth.
      if(objs(OYSTER).prop 0<) andif(toting(OYSTER))
        puts("Interesting. There seems to be something written on the underside of\nthe oyster.")
        do(MAX_OBJ 1+ MIN_OBJ)
          if(toting(i)) andif(objs(j).prop 0<)
            objs(i).setProp(-1 objs(i).prop -)
          endif
        loop
      endif
    endif
  ;

  : attempt_plover_passage    \ section 149 in Knuth
    Location from!
    if(holding_count   if(toting(EMERALD)) 1 else 0 endif   =)
      R_ALCOVE R_PLOVER + from -
    else
      puts("Something you're carrying won't fit through the tunnel with you.\n\+
You'd best take inventory and drop something.")
      from
    endif
  ;

  : attempt_inventory  \ section 94 in Knuth
    false bool holding_anything!
    do(MAX_OBJ 1+ MIN_OBJ)
      if(toting(i)) andif(i BEAR <>)
        if(not(holding_anything))
          true holding_anything!
          puts("You are currently holding the following:")
        endif
        bl %c objs(i).name.get %s %nl
      endif
    loop
    if(toting(BEAR))
      puts("You are being followed by a very large, tame bear.")
    elseif(not(holding_anything))
      puts("You're not carrying anything.")
    endif
  ;

  : attempt_eat    \ section 98 in Knuth
    ObjectWord obj!
    false bool isRidiculous!
    case(obj)
      of(FOOD)
        destroy(FOOD) 
        puts("Thank you, it was delicious!")
        exit
      endof
      of(BIRD) of(SNAKE) of(CLAM) of(OYSTER)
      of(DWARF) of(DRAGON) of(TROLL) of(BEAR) of(DWARF)
        true isRidiculous!
      endof
    endcase
    if(isRidiculous)
      puts("Don't be ridiculous!")
    else
      puts("I think I just lost my appetite.")
    endif
  ;

  : take_something_immobile
    ObjectWord obj!
   
    if(and(obj CHAIN =  objs(BEAR).prop 0<>))
        puts("The chain is still locked.")
    elseif(and(obj BEAR =  objs(BEAR).prop 1 =))
      puts("The bear is still chained to the wall.")
    elseif(and(obj PLANT = objs(PLANT).prop 0<=))
      puts("The plant has exceptionally deep roots and cannot be pulled free.")
    else
      puts("You can't be serious!")
    endif
  ;

  \ The verb is TAKE. Returns true if the action is finished.
  : take_bird_or_cage
    ObjectWord obj!
    if(obj BIRD = objs(BIRD).prop not and)
      if(toting(ROD))
        puts("The bird was unafraid when you entered, but as you approach it becomes\n\+
disturbed and you cannot catch it.")
        true
        exit
      elseif(toting(CAGE) not)
        puts("You can catch the bird, but you cannot carry it.")
        true
        exit
      else
        objs(BIRD).setProp(1)
      endif
    endif
    \ At this point the TAKE action is guaranteed to succeed, so
    \ let's also deal with the cage. Taking a caged bird means also
    \ taking the cage; taking a cage with a bird in it means also
    \ taking the bird.
    if(obj BIRD =) carry(CAGE) endif
    if(obj CAGE = objs(BIRD).prop and) carry(BIRD) endif
    false
  ;

  \ Return true if the command was TAKE WATER or TAKE OIL,
  \ and there's not a bottle of that substance on the ground.
  \ We'll redirect it to FILL BOTTLE and try again.
  : attempt_take
    Location loc!
    ObjectWord obj!
    if(toting(obj))
      puts("You are already carrying it!")
      false
      exit
    elseif(is_immobile(obj))
      take_something_immobile(obj)
      false
      exit
    elseif(obj NOTHING <>) andif(here(BOTTLE loc)) andif(obj bottle_contents() =)
      BOTTLE obj!  \ take the bottle of water or oil
    elseif(obj WATER =) orif(obj OIL =)
      if(toting(BOTTLE))
        true  \ redirect to FILL BOTTLE
        exit
      endif
      puts("You have nothing in which to carry it.")
      false
      exit
    endif
    
    if(holding_count 7 >=)
      puts("You can't carry anything more.  You'll have to drop something first.")
    elseif(take_bird_or_cage(obj))
      \ The bird was uncatchable.
    else
      carry(obj) 
      if(obj BOTTLE =) andif(bottle_contents NOTHING <>)
        objs(bottle_contents).setPlace(R_INHAND)
      endif
      puts(okMsg)
    endif
    false
  ;
  
spoo before attempt drop
  : attempt_drop
    Location loc!
    ObjectWord obj!
    if(obj ROD =) andif(toting(ROD2)) andif(not(toting(ROD)))
      ROD2 obj!
    endif

    if(not(toting(obj)))
      puts("You aren't carrying it!")
    elseif(obj COINS =) andif(here(MACHINE loc))
      \ Put coins in the vending machine.
      destroy(COINS) 
      dropObject(BATTERIES loc) 
      objs(BATTERIES).setProp(0)
      puts("There are fresh batteries here.")
    elseif(obj VASE =) andif(loc R_SOFT <>)
      dropObject(VASE loc) 
      if(there(PILLOW loc))
        \ In Long's "Adventure 6" the response is this message
        \ plus the default "Dropped."
        puts("The vase is now resting, delicately, on a velvet pillow.")
      else
        puts("The Ming vase drops with a delicate crash.")
        objs(VASE).setProp(1)  \ the vase is now broken
        immobilize(VASE) 
      endif
    elseif(obj BEAR =) andif(is_at_loc(TROLL loc))
      \ Chase the troll away.
      puts("The bear lumbers toward the troll, who lets out a startled shriek and\n\+
scurries away.  The bear soon gives up the pursuit and wanders back.")
      destroy(TROLL)  destroy(TROLL_) 
      dropObject(NO_TROLL R_SWSIDE)  dropObject(NO_TROLL_ R_NESIDE) 
      objs(TROLL).setProp(2)
      juggle(CHASM)  juggle(CHASM_)   \ put first in their lists
      dropObject(BEAR loc) 
    elseif(obj BIRD =) andif(here(SNAKE loc))
      puts("The little bird attacks the green snake, and in an astounding flurry\n\+
drives the snake away.")
      if(closed) dwarves_upset endif
      dropObject(BIRD loc) 
      objs(BIRD).setProp(0)  \ no longer in the cage
      destroy(SNAKE) 
      objs(SNAKE).setProp(1)  \ used in conditional Instructions
    elseif(obj BIRD =) andif(is_at_loc(DRAGON loc)) andif(not(objs(DRAGON).prop))
      puts("The little bird attacks the green dragon, and in an astounding flurry\n\+
gets burnt to a cinder.  The ashes blow away.")
      destroy(BIRD) 
      objs(BIRD).setProp(0)  \ no longer in the cage
      \ Now that the bird is dead, you can never get past the snake
      \ into the south side chamber, so the precious jewelry is lost.
      if(there(SNAKE R_HMK)) lost_treasures++ endif
    else
      \ The usual case for dropping any old object.

      if(obj BIRD =) objs(BIRD).setProp(0) endif \ no longer caged
      if(obj CAGE =) andif(objs(BIRD).prop) dropObject(BIRD loc) endif

      \ Special cases for dropping a liquid.
      if(obj WATER =) andif(objs(BOTTLE).prop 0=) BOTTLE obj! endif
      if(obj OIL =) andif(objs(BOTTLE).prop 2 =) BOTTLE obj! endif
      if(obj BOTTLE =) andif(bottle_contents NOTHING <>)
        move(bottle_contents R_LIMBO) 
      endif

      dropObject(obj loc) 
      puts(okMsg) 
    endif
  ;

  : attempt_wave  \ section 99 in Knuth
   Location loc!
   ObjectWord obj!
    if(obj ROD =) andif(or(loc R_EFISS = loc R_WFISS =))
        andif(toting(ROD)) andif(not(cave_is_closing))
      if(objs(FISSURE).prop)
        puts("The crystal bridge has vanished!")
        objs(FISSURE).setProp(0)
      else
        puts("A crystal bridge now spans the fissure.")
        objs(FISSURE).setProp(1)
      endif
    elseif(toting(obj)) orif(and(obj ROD = toting(ROD2)))
      puts("Nothing happens.")
    else
      puts("You aren't carrying it!")
    endif
  ;

  : attempt_blast  \ section 99 in Knuth
    Location loc!
    if(closed) andif(objs(ROD2).prop)
      if(here(ROD2 loc))
        25 bonus!
        puts("There is a loud explosion and you are suddenly splashed across the\n\+
walls of the room.")
      elseif(loc R_NEEND =)
        30 bonus!
        puts("There is a loud explosion and a twenty-foot hole appears in the far\n\+
wall, burying the snakes in the rubble. A river of molten lava pours\n\+
in through the hole, destroying everything in its path, including you!")
      else
        45 bonus!
        puts("There is a loud explosion and a twenty-foot hole appears in the far\n\+
wall, burying the dwarves in the rubble.  You march through the hole\n\+
and find yourself in the main office, where a cheering band of\n\+
friendly elves carry the conquering adventurer off into the sunset.")
      endif
      setState(kStateQuit)
      quit
    else
      puts("Blasting requires dynamite.")
    endif
  ;

  : attempt_rub  \ section 99 in Knuth
    ObjectWord obj!
    if(obj LAMP =)
      puts("Rubbing the electric lamp is not particularly rewarding. Anyway,\n\+
nothing exciting happens.")
    else
      puts("Peculiar.  Nothing unexpected happens.")
    endif
  ;

  : attempt_find  \ section 100 in Knuth
   Location loc!
   ObjectWord obj!
    if(toting(obj))
      puts("You are already carrying it!")
    elseif(closed)
      puts("I daresay whatever you want is around here somewhere.")
    else
      false bool its_right_here!
      if(is_at_loc(obj loc))
        true its_right_here!
      elseif(obj NOTHING <>) andif(obj bottle_contents =) andif(there(BOTTLE loc)) 
        true its_right_here!
      elseif(obj WATER =) andif(has_water(loc))
        true its_right_here!
      elseif(obj OIL =) andif(has_oil(loc))
        true its_right_here!
      elseif(obj DWARF =) andif(dwarf_at(loc))
        true its_right_here!
      endif
      if(its_right_here)
        puts("I believe what you want is right here with you.")
      else
        puts("I can only tell you what you see as you move about and manipulate\n\+
things.  I cannot tell you where remote things are.")
      endif
    endif
  ;

  : attempt_break  \ section 101 in Knuth
    Location loc!
    ObjectWord obj!
    if(obj VASE =) andif(objs(VASE).prop 0=)
      if(toting(VASE))
        dropObject(VASE loc) 
      endif
      puts("You have taken the vase and hurled it delicately to the ground.")
      objs(VASE).setProp(1)  \ worthless shards
      immobilize(VASE) 
    elseif(obj MIRROR =)
      if(closed)
        puts("You strike the mirror a resounding blow, whereupon it shatters into a\nmyriad tiny fragments.")
        dwarves_upset
      else
        puts("It is too far up for you to reach.")
      endif
    else
      puts("It is beyond your power to do that.")
    endif
  ;

  : attempt_wake  \ section 101 in Knuth
    ObjectWord obj!
    if(closed) andif(obj DWARF =)
      puts("You prod the nearest dwarf, who wakes up grumpily, takes one look at\nyou, curses, and grabs for his axe.")
      dwarves_upset
    endif
    puts("Don't be ridiculous!")
  ;

  : attempt_off  \ section 102 in Knuth
    Location loc!
    if(not(here(LAMP loc)))
      puts("You have no source of light.")
    else
      objs(LAMP).setProp(0)
      puts("Your lamp is now off.")
      if(now_in_darkness(loc))
        puts(pitchDarkMsg)
      endif
    endif
  ;

  : kill_a_dwarf
    Location loc!
    Dwarf dwarf
    if(firstDwarfKill)
      puts("You killed a little dwarf.  The body vanishes in a cloud of greasy\nblack smoke.")
      false firstDwarfKill!
    else
      puts("You killed a little dwarf.")
    endif

    do(6 1)
      dwarves.get(i) dwarf!o
      if(dwarf.loc loc =)
        dwarf.setLoc(R_LIMBO)  \ Once killed, a dwarf never comes back.
        false dwarf.seen!
      endif
    loop
  ;

  : throw_axe_at_dwarf  \ section 163 in Knuth
    Location loc!
    if(ran(3) 2 <)
      kill_a_dwarf(loc)
    else
      puts("You attack a little dwarf, but he dodges out of the way.")
    endif
  ;

  \ Return true if we don't know what to fill.
  : attempt_fill  \ sections 110--111 in Knuth
    Location loc!
    ObjectWord obj!
    if(obj VASE =)
      if(not(has_oil(loc))) andif(not(has_water(loc)))
        puts("There is nothing here with which to fill the vase.")
      elseif(not(toting(VASE)))
        puts("You aren't carrying it!")
      else
        \ In Crowther and Woods' original, after shattering the vase this
        \ way, we GOTO the generic "drop" code. This produces a silly
        \ combination of messages --- and repairs the vase! --- if the
        \ pillow is on the ground next to you as you fill the vase.
        \ In Long's "Adventure 6" we skip the pillow-checking code, but
        \ still end up in the default handler, which would normally
        \ print "Dropped." but in this instance prints "There is nothing
        \ here with which to fill the vase."
        puts("The sudden change in temperature has delicately shattered the vase.")
        objs(VASE).setProp(1)  \ worthless shards
        dropObject(VASE loc) 
        immobilize(VASE) 
      endif
    elseif(not(here(BOTTLE loc)))
      if(obj NOTHING =)
        true
        exit
      endif
      puts("You can't fill that.")
    elseif(obj NOTHING <>) andif(obj BOTTLE <>)
      puts("You can't fill that.")
    elseif(bottle_contents NOTHING <>)
      puts("Your bottle is already full.")
    elseif(has_oil(loc))
      puts("Your bottle is now full of oil.")
      objs(BOTTLE).setProp(2)
      if(toting(BOTTLE))
        objs(OIL).setPlace(R_INHAND)
      endif
    elseif(has_water(loc))
      puts("Your bottle is now full of water.")
      objs(BOTTLE).setProp(0)
      if(toting(BOTTLE))
        objs(WATER).setPlace(R_INHAND)
      endif
    else
      puts("There is nothing here with which to fill the bottle.")
    endif
    false  \ just continue normally
  ;

  : attempt_feed  \ section 129 in Knuth
    Location loc!
    ObjectWord obj!
    case(obj)
      of(BIRD)
        puts("It's not hungry (it's merely pinin' for the fjords).  Besides, you\n\+
have no bird seed.")
      endof
      of(TROLL)
        puts("Gluttony is not one of the troll's vices.  Avarice, however, is.")
      endof
      of(DRAGON)
        if(objs(DRAGON).prop)
          puts("Don't be ridiculous!")  \ reject feeding the dead dragon
        else
          puts("There's nothing here it wants to eat (except perhaps you).")
        endif
      endof
      of(SNAKE)
        if(not(closed)) andif(here(BIRD loc))
          destroy(BIRD) 
          objs(BIRD).setProp(0)
          lost_treasures++
          puts("The snake has now devoured your bird.")
        else
          puts("There's nothing here it wants to eat (except perhaps you).")
        endif
      endof
      of(BEAR)
        if(here(FOOD loc))
          \ assert(objs(BEAR).prop 0=) 
          destroy(FOOD) 
          objs(BEAR).setProp(1)
          objs(AXE).setProp(0)
          mobilize(AXE)   \ if it was immobilized by the bear
          puts("The bear eagerly wolfs down your food, after which he seems to calm\n\+
down considerably and even becomes rather friendly.")
        elseif(objs(BEAR).prop 0=)
          puts("There's nothing here it wants to eat (except perhaps you).")
        elseif(objs(BEAR).prop 3 =)
          puts("Don't be ridiculous!")
        else
          \ The bear is tame; therefore the food is gone.
          puts("There is nothing here to eat.")
        endif
      endof
      of(DWARF)
        if(here(FOOD loc))
          dwarfAngerLevel++
          puts("You fool, dwarves eat only coal!  Now you've made him *REALLY* mad!!")
        else
          puts("There is nothing here to eat.")
        endif
      endof
      puts("I'm game.  Would you care to explain how?")
    endcase
  ;

  : attempt_open_or_close  \ sections 130--134 in Knuth
    Location loc!
    ObjectWord obj!
    ActionWord verb!
    if(verb OPEN =) 1 else 0 endif int verb_is_open! \ otherwise it's CLOSE
    
    case(obj)
      of(OYSTER)
      of(CLAM)
        if(obj CLAM =) "clam" else "oyster" endif ptrTo byte clam_oyster!
        if(not(verb_is_open))
          puts("What?")
        elseif(not(toting(TRIDENT)))
          "You don't have anything with which to open the " %s clam_oyster %s ".\n" %s
        elseif(toting(obj))
          "I advise you to put down the " %s clam_oyster %s " before opening it.  " %s
          if(obj CLAM =) ">STRAIN!<\n" else ">WRENCH!<\n" endif %s
        elseif(CLAM obj =)
          \ The pearl will appear in the cul-de-sac no matter where
          \ we are; we don't allow the player to carry the clam out
          \ of the Shell Room area.
          destroy(CLAM) 
          dropObject(OYSTER loc) 
          dropObject(PEARL R_SAC) 
          puts("A glistening pearl falls out of the clam and rolls away.  Goodness,\n\+
this must really be an oyster.  (I never was very good at identifying\n\+
bivalves.)  Whatever it is, it has now snapped shut again.")
        else
          puts("The oyster creaks open, revealing nothing but oyster inside.\n\+
It promptly snaps shut again.")
        endif
      endof
      of(GRATE)
        if(not(here(KEYS loc)))
          puts("You have no keys!")
        elseif(cave_is_closing)
          \ Trying to get out through the grate after closing.
          panic_at_closing_time
        else
          objs(GRATE).prop int was_open!
          objs(GRATE).setProp(verb_is_open)
          case(was_open verb_is_open 2* +)
            of(0) puts("It was already locked.") endof
            of(1) puts("The grate is now locked.") endof
            of(2) puts("The grate is now unlocked.") endof
            of(3) puts("It was already unlocked.") endof
          endcase
        endif
      endof
      of(CHAIN)
        if(not(here(KEYS loc)))
          puts("You have no keys!")
        elseif(verb_is_open)
          \ UNLOCK CHAIN
          if(objs(CHAIN).prop 0=)
            puts("It was already unlocked.")
          elseif(objs(BEAR).prop 0=)
            puts("There is no way to get past the bear to unlock the chain, which is\n\+
probably just as well.")
          else
            objs(CHAIN).setProp(0)
            mobilize(CHAIN) 
            if(objs(BEAR).prop 1 =)
              objs(BEAR).setProp(2)
              mobilize(BEAR) 
            endif
            puts("The chain is now unlocked.")
          endif
        else
          \ LOCK CHAIN 
          if(loc R_BARR <>)
            puts("There is nothing here to which the chain can be locked.")
          elseif(objs(CHAIN).prop)
            puts("It was already locked.")
          else
            objs(CHAIN).setProp(2)
            immobilize(CHAIN) 
            if(toting(CHAIN)) dropObject(CHAIN loc) 
              puts("The chain is now locked.")
            endif
          endif
        endif
      endof
      of(KEYS)
        puts("You can't lock or unlock the keys.")
      endof
      of(CAGE)
        puts("It has no lock.")
      endof
      of(RUSTY_DOOR)
        if(objs(RUSTY_DOOR).prop)
          puts(okMsg) 
        else
          \ Notice that CLOSE DOOR also gives this response.
          puts("The door is extremely rusty and refuses to open.")
        endif
      endof
      puts("I don't know how to lock or unlock such a thing.")
    endcase
  ;

  \ This is some pretty baroque logic for an obscure case.
  \ We're deciding what the noun should be if the player types
  \ "READ" as a one-word command. If we return NOTHING, the
  \ main loop will branch to get_object; otherwise it will
  \ be handled as if we'd typed "READ <obj>".
  : read_what  \ returns ObjectWord
    Location loc!
    if(now_in_darkness(loc))
      NOTHING  \ can't read in the dark
      exit
    endif
    if(closed) andif(toting(OYSTER))
      OYSTER
      exit
    endif

    here(MAG loc) bool magazines_here!
    if(here(TABLET loc))
      if(magazines_here) NOTHING else TABLET endif
      exit
    endif
    if(here(MESSAGE loc))
      if(magazines_here) NOTHING else MESSAGE endif
      exit
    endif
    if(magazines_here) MAG else NOTHING endif
  ;

  : attempt_read  \ section 135 in Knuth
    ObjectWord obj!
    case(obj)
      of(MAG)
        puts("I'm afraid the magazine is written in dwarvish.")
      endof
      of(TABLET)
        puts("\"CONGRATULATIONS ON BRINGING LIGHT INTO THE DARK-ROOM!\"")
      endof
      of(MESSAGE)
        puts("\"This is not the maze where the pirate leaves his treasure chest.\"")
      endof
      of(OYSTER)
        if(closed) andif(toting(OYSTER))
          if(hints.get(1).given)
            puts("It says the same thing it did before.")
          else
            offer(1)
          endif
        else
          puts("I'm afraid I don't understand.")
        endif
      endof
      puts("I'm afraid I don't understand.")
    endcase
  ;

  : check_noun_validity  \ sections 90--91 in Knuth
    Location loc!
    ObjectWord obj!
    if(toting(obj)) orif(is_at_loc(obj loc))
      0  \ no problem
      exit
    endif
    case(obj)
      of(GRATE)
        if(loc MIN_LOWER_LOC <)
          case(loc)
            of(R_ROAD) of(R_VALLEY) of(R_SLIT)
              'd'  \ try moving to DEPRESSION
              exit
            endof
            of(R_COBBLES) of(R_DEBRIS) of(R_AWK) of(R_BIRD) of(R_SPIT)
              'e'  \ try moving to ENTRANCE
              exit
            endof
          endcase
        endif
        'c'  \ can't see it
        exit
      endof
      of(DWARF)
        if(dwarfAngerLevel 2 >=) andif(dwarf_at(loc))
          0
        else
          'c'  \ can't see it
        endif
        exit
      endof
      of(PLANT)
        if(is_at_loc(PLANT2 loc)) andif(objs(PLANT2).prop 0<>)
          'p'  \ obj = PLANT2
        else
          'c'  \ can't see it
        endif
        exit
      endof
      of(KNIFE)
        \ You're allowed to try to GET KNIFE, but only once. The game
        \ informs you that the knife has vanished; and from then on,
        \ it never reappears.
        if(haveTriedToGetKnife) orif((loc last_knife_loc <>))
          'c'  \ can't see it
          exit
        endif
        puts("The dwarves' knives vanish as they strike the walls of the cave.")
        true haveTriedToGetKnife!
        'f'  \ action is finished
        exit
      endof
      of(ROD)
        if(not(here(ROD2 loc)))
          'c'
        else
          'r'  \ obj = ROD2
        endif
        exit
      endof
      of(WATER)
        if(has_water(loc))
          0
          exit
        endif
        if(here(BOTTLE loc)) andif(bottle_contents WATER =)
          0
          exit
        endif
        'c'
        exit
      endof
      of(OIL)
        if(has_oil(loc))
          0
          exit
        endif
        if(here(BOTTLE loc)) andif(bottle_contents OIL =)
          0
          exit
        endif
        'c'
        exit
      endof
    endcase
    'c'
  ;
  
spoo before determine_motion_instruction
  \ Location MotionWord ... Instruction*
  : _determine_motion_instruction  \ returns Instruction*
    MotionWord mot!
    Location loc!
    start(loc) ptrTo Instruction q!
    begin
    while(q start(loc 1+) <)
      breakIf(or(is_forced(loc)  q.mot mot =))
      sizeOf Instruction q!+
    repeat
    if(q start(loc 1+) =)
      \ we didn't find a matching command for mot
      null  \ newloc = loc at this point
      exit
    endif
    begin
      d[ tr{ showInstruction(q) }tr ]d
      q.cond int cond!
      mod(cond 100) MIN_OBJ + int obj!
      if(cond 0=)
        break  \ the usual case
      elseif(cond 100 <=)
        breakIf(pct(cond))  \ dwarves won't take these routes
      elseif(cond 200 <=)
        breakIf(toting(obj))
      elseif(cond 300 <=)
        breakIf(is_at_loc(obj loc))
      else
        cond 100 / 3- int propValue!
        breakIf(objs(obj).prop  propValue <>)
      endif
      \ The verb in this instruction matches the "mot" typed by the user,
      \ but the condition failed, either randomly (j < 100) or because
      \ the prerequisite wasn't satisfied. So instead of taking this
      \ instruction, we'll take the next one that is not a "ditto" of
      \ this one --- regardless of its "mot" field!
      q.dest Location dest!
      begin
      while(and(q.dest dest =   q.cond cond =))
        sizeOf Instruction q!+
      repeat
    again
    q
  ;
  : determine_motion_instruction
    \ 255 setTrace
    _determine_motion_instruction
    \ 0 setTrace
  ;
  
  : report_inapplicable_motion
    ActionWord verb!
    MotionWord mot!
    if(mot CRAWL =)
      puts("Which way?")
    elseif(mot XYZZY =) orif(mot PLUGH =)
      puts("Nothing happens.")
    elseif(verb FIND =) orif(verb INVENTORY =)
      \ This catches inputs such as "FIND BEDQUILT" or "INVENTORY WEST".
      puts("I can only tell you what you see as you move about and manipulate\n\+
things.  I cannot tell you where remote things are.")
    else
      case(mot)
        of(N) of(S) of(E) of(W) of(NE) of(SE) of(NW) of(SW)
        of(U) of(D)
          puts("There is no way to go in that direction.")
        endof
        of(IN) of(OUT)
          puts("I don't know in from out here.  Use compass points or name something\n\+
in the general direction you want to go.")
        endof
        of(FORWARD) of(LEFT) of(RIGHT)
          puts("I am unsure how you are facing.  Use compass points or nearby objects.")
        endof
        puts("I don't know how to apply that word here.")
      endcase
    endif
  ;

  : print_remark
    int which!
    case(which)
      of(0) puts("You don't fit through a two-inch slit!") endof
      of(1) puts("You can't go through a locked steel grate!") endof
      of(2) puts("I respectfully suggest you go across the bridge instead of jumping.") endof
      of(3) puts("There is no way across the fissure.") endof
      of(4) puts("You can't fit this five-foot clam through that little passage!") endof
      of(5) puts("You can't fit this five-foot oyster through that little passage!") endof

      of(6) puts("You have crawled around in some little holes and wound up back in the\n\+
main passage.")
      endof
      of(7)
        puts("You have crawled around in some little holes and found your way\n\+
blocked by a recent cave-in.  You are now back in the main passage.")
      endof
      of(8) puts("It is too far up for you to reach.") endof
      of(9) puts("The door is extremely rusty and refuses to open.") endof
      of(10) puts("The dragon looks rather nasty.  You'd best not try to get by.") endof
      of(11) puts("The troll refuses to let you cross.") endof
      of(12) puts("There is no longer any way across the chasm.") endof
      of(13) puts("Don't be ridiculous!") endof
      of(14) puts("The crack is far too small for you to follow.") endof
      of(15) puts("The dome is unclimbable.") endof
      of(16) puts("You can't get by the snake.") endof
      of(17) puts("The stream flows out through a pair of 1-foot-diameter sewer pipes.\n\+
It would be advisable to use the exit.")
      endof
    endcase
  ;

  : try_going_back_to   \ returns MotionWord
    Location from!
    Location loc!
    \ Interestingly, the BACK command does not simply move the player
    \ back to oldloc. Instead, it attempts to trace a path connecting
    \ loc to oldloc; if no such passage exists, we fail to move. If
    \ such a passage does exist, then we react as if the player had
    \ typed the appropriate motion-word in the first place.
    \ Notice that Woods' code makes an effort to deal with FORCED_MOVE
    \ locations, but doesn't succeed 100 percent. ("l" is set to oldloc,
    \ or to oldoldloc if oldloc is a FORCED_MOVE location.)
    \     R_SWSIDE : CROSS -> R_TROLL -> R_NESIDE : BACK
    \ gives "You can't get there from here." as apparently intended,
    \ because R_TROLL is a special location.
    \     R_WPIT : CLIMB -> R_CHECK -> R_UPNOUT -> R_WPIT : BACK
    \ unintentionally gives "There is nothing here to climb.", because
    \ oldoldloc is R_CHECK, and there *is* a passage connecting our
    \ current location (R_WPIT) to R_CHECK, so we take it again.
    \     R_WPIT : CLIMB -> R_CHECK -> R_DIDIT -> R_W2PIT : BACK
    \ unintentionally gives "You can't get there from here.", because
    \ oldoldloc is R_CHECK, and there is no passage connecting our
    \ current location (R_W2PIT) to R_CHECK.

    if(loc from =)
      puts("Sorry, but I no longer seem to remember how you got here.")
      NOWHERE
      exit
    endif
    start(from) ptrTo Instruction q!
    begin
    while(q start(from 1+) <)
      \ Take the first exit that goes to "l" either directly...
      q.dest Location ll!
      if(ll loc =)
        q.mot
        exit
      endif
      \ ...or via a single forced move.
      if(is_forced(ll)) andif(start(ll).dest loc =)
        q.mot
        exit
      endif
      sizeOf Instruction q!+
    repeat
    puts("You can't get there from here.")
    NOWHERE
  ;

  : collapse_the_troll_bridge
    puts("Just as you reach the other side, the bridge buckles beneath the\n\+
weight of the bear, who was still following you around.  You\n\+
scrabble desperately for support, but as the bridge collapses you\n\+
stumble back and fall into the chasm.")
    objs(CHASM).setProp(1)
    objs(TROLL).setProp(2)
    objs(BEAR).setProp(3)  \ the bear is dead
    dropObject(BEAR R_SWSIDE) 
    immobilize(BEAR) 
    \ If you collapse the troll bridge without seeing the spices, we'll
    \ increment lost_treasures so you can still get to the endgame.
    \ However, if you threw the trident to the troll before opening the
    \ oyster, you're strictly out of luck. And in fact if you pick up
    \ a treasure in the dark, you'll never "see" it until you drop it
    \ in a lighted area; so there can be arbitrarily many unseen
    \ treasures on the far side of the bridge, if the player is doing
    \ it deliberately. But we don't care about that case.
    if(objs(SPICES).prop 0<) andif(objs(SPICES).place R_NESIDE >=)
      lost_treasures++
    endif
  ;

  \ Modify newloc in place, and return true if the player died crossing the troll bridge.
  : determine_next_newloc    \ returns bool
    ActionWord verb!
    MotionWord mot!
    ptrTo Location newloc!
    Location loc!
    determine_motion_instruction(loc mot) ptrTo Instruction q!
    if(q 0=)
      report_inapplicable_motion(mot verb)
      false
      exit
    endif
    q.dest newloc !

    if(newloc @ FIRST_REMARK >=)
      print_remark(newloc @ FIRST_REMARK -) 
      loc newloc !
    elseif(newloc @ R_PPASS =)
      attempt_plover_passage(loc) newloc !
    elseif(newloc @ R_PDROP =)
      dropObject(EMERALD loc) 
      R_Y2 R_PLOVER + loc - newloc !
    elseif(newloc @ R_TROLL =)
      \ Troll bridge crossing is treated as a special motion so
      \ that dwarves won't wander across and encounter the bear.
      \ You can get here only if TROLL is in limbo but NO_TROLL has
      \ taken its place. Moreover, if you're on the southwest side,
      \ objs(TROLL).prop will be nonzero. If objs(TROLL).prop is 1,
      \ you've crossed since paying, or you've stolen away the payment.
      if(objs(TROLL).prop 1 =)
        \ Block the troll bridge and stay put.
        objs(TROLL).setProp(0)
        destroy(NO_TROLL)  destroy(NO_TROLL_) 
        dropObject(TROLL R_SWSIDE)  dropObject(TROLL_ R_NESIDE) 
        juggle(CHASM)  juggle(CHASM_) 
        puts("The troll steps out from beneath the bridge and blocks your way.")
        loc newloc ! \ stay put
      else
        R_NESIDE R_SWSIDE + loc - newloc !  \ cross it
        if(objs(TROLL).prop 0=)
          objs(TROLL).setProp(1)
          if(toting(BEAR))
            \ assert(*newloc R_SWSIDE =) 
            collapse_the_troll_bridge()
            true  \ goto death
            exit
          endif
        endif
      endif
    endif
    false
  ;

  : print_message
    MessageWord msg!
    case(msg)
      of(ABRA)
        puts("Good try, but that is an old worn-out magic word.")
      endof
      of(HELP)
        puts("I know of places, actions, and things. Most of my vocabulary\n\+
describes places and is used to move you there. To move, try words\n\+
like forest, building, downstream, enter, east, west, north, south,\n\+
up, or down. I know about a few special objects, like a black rod\n\+
hidden in the cave. These objects can be manipulated using some of\n\+
the action words that I know. Usually you will need to give both the\n\+
object and action words (in either order), but sometimes I can infer\n\+
the object from the verb alone. Some objects also imply verbs; in\n\+
particular, \"inventory\" implies \"take inventory\", which causes me to\n\+
give you a list of what you're carrying. The objects have side\n\+
effects; for instance, the rod scares the bird. Usually people having\n\+
trouble moving just need to try a few more words. Usually people\n\+
trying unsuccessfully to manipulate an object are attempting something\n\+
beyond their (or my!) capabilities and should try a completely\n\+
different tack. To speed the game you can sometimes move long\n\+
distances with a single word. For example, \"building\" usually gets\n\+
you to the building from anywhere above ground except when lost in the\n\+
forest. Also, note that cave passages turn a lot, and that leaving a\n\+
room to the north does not guarantee entering the next from the south.\n\+
Good luck!")
      endof
      of(TREES)
        puts("The trees of the forest are large hardwood oak and maple, with an\n\+
occasional grove of pine or spruce.  There is quite a bit of under-\n\+
growth, largely birch and ash saplings plus nondescript bushes of\n\+
various sorts.  This time of year visibility is quite restricted by\n\+
all the leaves, but travel is quite easy if you detour around the\n\+
spruce and berry bushes.")
      endof
      of(DIG)
        puts("Digging without a shovel is quite impractical.  Even with a shovel\n\+
progress is unlikely.")
      endof
      of(LOST) puts("I'm as confused as you are.") endof
      of(MIST)
        puts("Mist is a white vapor, usually water, seen from time to time in\n\+
caverns.  It can be found anywhere but is frequently a sign of a deep\n\+
pit leading down to water.")
      endof
      of(FUCK) puts("Watch it!") endof
      of(STOP) puts("I don't know the word \"stop\".  Use \"quit\" if you want to give up.") endof
      of(INFO)
        puts("If you want to end your adventure early, say \"quit\". To get full\n\+
credit for a treasure, you must have left it safely in the building,\n\+
though you get partial credit just for locating it. You lose points\n\+
for getting killed, or for quitting, though the former costs you more.\n\+
There are also points based on how much (if any) of the cave you've\n\+
managed to explore; in particular, there is a large bonus just for\n\+
getting in (to distinguish the beginners from the rest of the pack),\n\+
and there are other ways to determine whether you've been through some\n\+
of the more harrowing sections. If you think you've found all the\n\+
treasures, just keep exploring for a while. If nothing interesting\n\+
happens, you haven't found them all yet. If something interesting\n\+
DOES happen, it means you're getting a bonus and have an opportunity\n\+
to garner many more points in the master's section.\n\+
I may occasionally offer hints if you seem to be having trouble.\n\+
If I do, I'll warn you in advance how much it will affect your score\n\+
to accept the hints. Finally, to save paper, you may specify \"brief\",\n\+
which tells me never to repeat the full description of a place\n\+
unless you explicitly ask me to.")
      endof
      of(SWIM) puts("I don't know how.") endof
    endcase
  ;

  : advise_about_going_west
    ptrTo byte word1!
    \ Here's a freely offered hint that may save you typing.
    if(streq(word1 "west"))
      __west_count++
      if(__west_count 10 =)
        puts(" If you prefer, simply type W rather than WEST.")
      endif
    endif
  ;

  : incantation
    case
      of(0) "fee" endof
      of(1) "fie" endof
      of(2) "foe" endof
      of(3) "foo" endof
      of(4) "fum" endof
      "frabozzle"
    endcase
  ;
  
  : saveAdventure
    mko String saveCommand
    saveCommand.set("copy tmp.sav ")
    saveCommand.append
    saveCommand.append(".sav")
    $shellRun(saveCommand.get)
    saveCommand~
  ;

spoo before simulate_an_adventure
 
  m: dwarves_upset
    puts("The resulting ruckus has awakened the dwarves.  There are now several\n\+
threatening little dwarves in the room with you!  Most of them throw\n\+
knives at you!  All of them get you!")
    quit
  ;m
  

  m: simulate_an_adventure
    R_ROAD mOldOldLoc!
    R_ROAD mOldLoc!
    R_ROAD mLoc!
    R_ROAD mNewLoc!
    NOWHERE mMotion!   \ currently specified motion
    NOTHING mVerb!  \ currently specified action
    NOTHING mOldVerb!  \ mVerb before it was changed
    NOTHING mObj!  \ currently specified object, if any
    NOTHING mOldObj!  \ former value of mObj
    false mWasDark!
    0 mLookCount!
    kStateLoopTop mCurStateNum!
    NOTHING mWordTemp!
    
    AdventureState curState

    begin
      \ sp!
      Game_adventureStates.get(mCurStateNum) curState!o
      d[ tr{
        "<<<STATE>>> " %s mCurStateNum %d %bl
        curState.name.get %s %bl
        " location " %s mLoc locationToName %s %bl
        inWord1.base %s %bl inWord2.base %s %bl %nl
      }tr ]d
      curState.stateOp
      \ sp?(curState.name.get)
    until(or(mCurStateNum kStateQuit =  gave_up))
  ;m
  
  : stateLoopTop    \ state 0
    \ ." >>> pirate at " dwarves.get(0).loc ref Location findEnumSymbol if(0=) "???" endif %s %nl
    \ Check for interference with the proposed move to mNewLoc.
    \ showObjectsAtLocation(mLoc)
    if(cave_is_closing) andif(mNewLoc MIN_IN_CAVE <) andif(mNewLoc R_LIMBO <>)
      panic_at_closing_time
      mLoc mNewLoc!
    elseif(mNewLoc mLoc <>) andif(not(forbidden_to_pirate(mLoc)))
      \ Stay in mLoc if a dwarf is blocking the way to mNewLoc
      do(6 1)
        if(dwarves.get(i).seen) andif(dwarves.get(i).oldloc mNewLoc =)
          puts("A little dwarf with a big knife blocks your way.")
          mLoc mNewLoc!
          leave
        endif
      loop
    endif
    mNewLoc mLoc!  \ hey, we actually moved you
    if(move_dwarves_and_pirate(mLoc))
      mLoc mOldOldLoc!
      setState(kStateDeath)
    endif
    setState(kStateCommence)
  ;

  : stateCommence    \ state 1
    int lookResult
    if(mLoc R_LIMBO =)
      setState(kStateDeath)
      exit
    endif
    look_around(mLoc now_in_darkness(mLoc) mWasDark) lookResult!
    case(lookResult)
      of('p') setState(kStatePitchDark) exit endof
      of('t') setState(kStateTryMove) exit endof
    endcase
    setState(kStateInnerLoop)
  ;

  : stateInnerLoop    \ state 2
    NOTHING dup mVerb! mOldVerb!
    mObj mOldObj!
    NOTHING mObj!
    setState(kStateCycle)
  ;
  
  : stateCycle    \ state 3
    maybe_give_a_hint(mLoc mOldLoc mOldOldLoc mOldObj) 
    now_in_darkness(mLoc) mWasDark!
    adjustments_before_listening(mLoc) 
    listen
    setState(kStatePreParse)
  ;
  
  : statePreParse    \ state 4
    if(streq(inWord1.base "save"))
      if(inWord2.base c@ 0<>)
        inWord2.base
      else
        "adventure"
      endif
      saveAdventure
      setState(kStateInnerLoop)
      exit
    endif

    \ writing user input to save file is delayed until here so line "save blah" isn't last line of save file
    writeToSaveFile(inBuffer.base)
    
    if(streq(inWord1.base "seed")) andif(inWord2.base c@ 0<>)
      setRandomSeed(atoi(inWord2.base))
      setState(kStateInnerLoop)
      exit
    endif
    
    if(streq(inWord1.base "save"))
      if(inWord2.base c@ 0<>)
        inWord2.base
      else
        "adventure"
      endif
      saveAdventure
      setState(kStateInnerLoop)
      exit
    endif
    
    turns++
    if(mVerb SAY =)
      \ Maybe you said "say" and we said "say what?" and you replied
      \ with two things to say. Then we assume you don't really want
      \ us to say anything. Section 82 in Knuth.
      if(inWord2.base c@ 0<>)
        NOTHING mVerb!
      else
        setState(kStateTransitive)
        exit
      endif
    endif

    \ Just after every command you give, we make the foobar counter
    \ negative if you're on track, otherwise we zero it.
    \ This is section 138 in Knuth.
    if(foobar 0>) negate(foobar) else 0 endif foobar!

    if(check_clocks_and_lamp(mLoc))
      \ The cave just closed!
      R_NEEND dup mLoc! mOldLoc!
      NOWHERE mMotion!
      setState(kStateTryMove)
      exit
    endif

    \ Handle additional special cases of input (Knuth sections 83, 105)
    if(streq(inWord1.base "enter"))
      if(streq(inWord2.base "water")) orif(streq(inWord2.base "strea"))
        if(has_water(mLoc))
          puts("Your feet are now wet.")
        else
          puts("Where?")
        endif
        setState(kStateInnerLoop)
        exit
      elseif(inWord2.base c@  0<>)
        \ ENTER TUNNEL becomes just TUNNEL.
        shift_words
        setState(kStateParse)
        exit
      endif
    endif
    if(streq(inWord1.base "water")) orif(streq(inWord1.base "oil"))
      \ Sometimes "water" and "oil" are used as verbs.
      if(streq(inWord2.base "plant")) andif(there(PLANT mLoc))
        strcpy(inWord2.base "pour")
      endif
      if(streq(inWord2.base "door")) andif(there(RUSTY_DOOR mLoc))
        strcpy(inWord2.base "pour")
      endif
    endif
    setState(kStateParse)
  ;
  
  : stateParse    \ state 5
    advise_about_going_west(inWord1.base) 
    lookup(inWord1.base) mWordTemp!
    case(word_class(mWordTemp))
            
      of(WordClass_None)
        "Sorry, I don't know the word \"" %s inWord1.base %s "\".\n" %s 
        setState(kStateCycle)
        exit
      endof
      
      of(WordClass_Motion)
        mWordTemp mMotion!
        setState(kStateTryMove)
        exit
      endof
            
      of(WordClass_Object)
        mWordTemp mObj!
        \ Make sure mObj is meaningful at the current location.
        \ When you specify an object, it must be here, unless
        \ the verb is already known to be FIND or INVENTORY.
        \ A few other special cases are permitted; for example,
        \ water and oil might be present inside the bottle or
        \ as a feature of the location.
        case(check_noun_validity(mObj mLoc))
          of('c')
            setState(kStateCantSeeIt)
            exit
          endof
          of('d')
            DEPRESSION mMotion!
            setState(kStateTryMove)
            exit
          endof
          of('e')
            ENTRANCE mMotion!
            setState(kStateTryMove)
            exit
          endof
          of('f')
            setState(kStateInnerLoop)
            exit
          endof
          of('p')
            PLANT2 mObj!
          endof
          of('r')
            ROD2 mObj!
          endof
          \ of(0) endof
        endcase
        if(inWord2.base c@ 0=)
          if(mVerb NOTHING <>)
            setState(kStateTransitive)
            exit
          endif
          "What do you want to do with the " %s inWord1.base %s "?\n" %s
          setState(kStateCycle)
        endif
      endof
            
      of(WordClass_Action)
        mWordTemp mVerb!
        if(mVerb SAY =)
          \ "FOO SAY" results in the incorrect message
          \ "Say what?" instead of "Okay, "foo"".
          if(inWord2.base c@ 0=)
            setState(kStateIntransitive)
            exit
          endif
          setState(kStateTransitive)
          exit
        endif
        if(inWord2.base c@ 0=)
          if(mObj NOTHING <>)
            setState(kStateTransitive)
            exit
          endif
          setState(kStateIntransitive)
          exit
        endif
      endof
            
      of(WordClass_Message)
        print_message(mWordTemp) 
        setState(kStateInnerLoop)
        exit
      endof
    endcase

    shift_words
    setState(kStateParse)
  ;
  
  : stateIntransitive    \ state 6
    ObjectWord object_here
    case(mVerb)
      of(GO)
        puts("Where?")
        setState(kStateInnerLoop)
        exit
      endof
      of(RELAX)
        puts(okMsg) 
        setState(kStateInnerLoop)
        exit
      endof
      of(ON) of(OFF) of(POUR) of(FILL) of(DRINK) of(BLAST) of(KILL)
        setState(kStateTransitive)
        exit
      endof
      of(TAKE)
        \ TAKE makes sense by itself if there's only one possible thing to take.
        places.get(mLoc).objects object_here!
        if(dwarf_at(mLoc))
          setState(kStateGetObject)
          exit
        endif
        if(object_here NOTHING <>) andif(objs(object_here).link NOTHING =)
          object_here mObj!
          setState(kStateTransitive)
          exit
        endif
        setState(kStateGetObject)
        exit
      endof
      of(EAT)
        if(not(here(FOOD mLoc)))
          setState(kStateGetObject)
          exit
        endif
        FOOD mObj!
        setState(kStateTransitive)
        exit
      endof
      of(OPEN) of(CLOSE)
        if(is_at_loc(GRATE mLoc))
          GRATE mObj!
        elseif(there(RUSTY_DOOR mLoc))
          RUSTY_DOOR mObj!
        elseif(here(CLAM mLoc))
          CLAM mObj!
        elseif(here(OYSTER mLoc))
          OYSTER mObj!
        endif
        if(here(CHAIN mLoc))
          if(mObj)
            setState(kStateGetObject)
            exit
          endif
          CHAIN mObj!
        endif
        if(mObj)
          setState(kStateTransitive)
          exit
        endif
        puts("There is nothing here with a lock!")
        setState(kStateInnerLoop)
        exit
      endof
      of(READ)
        read_what(mLoc) mObj!
        if(mObj NOTHING <>)
          setState(kStateTransitive)
          exit
        endif
        setState(kStateGetObject)
        exit
      endof
      of(INVENTORY)
        attempt_inventory
        setState(kStateInnerLoop)
        exit
      endof
      of(BRIEF)
        1000 verbose_interval!
        3 mLookCount!
        puts("Okay, from now on I'll only describe a place in full the first time\n\+
you come to it.  To get the full description, say \"LOOK\".")
        setState(kStateInnerLoop)
        exit
      endof
      of(SCORE)
        say_score
        setState(kStateInnerLoop)
        exit
      endof
      of(QUIT)
        if(yes("Do you really want to quit now?" okMsg okMsg))
          give_up
        endif
        setState(kStateInnerLoop)
        exit
      endof
#ifdef SAVE_AND_RESTORE
      of(SAVE)
        case(attempt_save)
          of(0) puts("Save failed!") endof
          of(1) puts("Saved.") endof
          of(2) puts("Restored.") endof
        endcase
        setState(kStateInnerLoop)
        exit
      endof
      of(RESTORE)
        \ On the fizmo interpreter, @restore yields 2
        \ when the save file doesn't exist, or when it
        \ has the wrong serial number for this game.
        \ I don't know what return value 0 would mean.
        attempt_restore
        puts("Restore failed!")
        setState(kStateInnerLoop)
        exit
      endof
#endif \ SAVE_AND_RESTORE
      of(FEEFIE)
        0 int kk!
        begin
        while(not(streq(inWord1.base incantation(kk))))
          kk++
        repeat
        if(foobar negate(kk) =)
          kk 1+ foobar!
          if(foobar 4 <>)
            puts(okMsg) 
            setState(kStateInnerLoop)
            exit
          endif
          0 foobar!
          if(there(EGGS R_GIANT)) orif(toting(EGGS)) andif(mLoc R_GIANT =)
            puts("Nothing happens.")
            setState(kStateInnerLoop)
            exit
          endif
          if(there(EGGS R_LIMBO)) andif(there(TROLL R_LIMBO)) andif(objs(TROLL).prop 0=)
            objs(TROLL).setProp(1)   \ the troll returns
          endif
          if(mLoc R_GIANT =)
            puts("There is a large nest here, full of golden eggs!")
          elseif(here(EGGS mLoc))
            puts("The nest of golden eggs has vanished!")
          else
            puts("Done!")
          endif
          move(EGGS R_GIANT) 
          setState(kStateInnerLoop)
          exit
                
        elseif(foobar 0=)
          puts("Nothing happens.")
        else
          puts("What's the matter, can't you read?  Now you'd best start over.")
        endif
        setState(kStateInnerLoop)
        exit
      endof
      setState(kStateGetObject)
      exit
    endcase
    \ does this fall thru to transitive state?
    setState(kStateTransitive)
  ;
  
  : stateTransitive    \ state 7
    int kSay
    case(mVerb)
      of(SAY)
        if(inWord2.base c@ 0<>) strcpy(inWord1.base inWord2.base) endif
        lookup(inWord1.base) kSay!
        case(kSay)
          of(XYZZY) of(PLUGH) of(PLOVER)
            kSay mMotion!
            setState(kStateTryMove)
            exit
          endof
          of(FEEFIE)
            kSay mVerb!
            setState(kStateIntransitive)
            exit
          endof
          "Okay, \"" %s inWord1.base %s "\".\n" %s
          setState(kStateInnerLoop)
          exit
         endcase
      endof
      of(EAT)
        attempt_eat(mObj) 
        setState(kStateInnerLoop)
        exit
      endof
      of(WAVE)
        attempt_wave(mObj mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(BLAST)
        attempt_blast(mLoc) 
        if(mCurStateNum kStateQuit <>)
          setState(kStateInnerLoop)
        endif
        exit
      endof
      of(RUB)
        attempt_rub(mObj) 
        setState(kStateInnerLoop)
        exit
      endof
      of(FIND) of(INVENTORY)
        attempt_find(mObj mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(BREAK)
        attempt_break(mObj mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(WAKE)
        attempt_wake(mObj) 
        setState(kStateInnerLoop)
        exit
      endof
      of(ON)
        if(not(here(LAMP mLoc)))
          puts("You have no source of light.")
          setState(kStateInnerLoop)
          exit
        endif
        if(lamp_limit 0<)
          puts("Your lamp has run out of power.")
          setState(kStateInnerLoop)
          exit
        endif
        objs(LAMP).setProp(1)
        puts("Your lamp is now on.")
        if(mWasDark)
          setState(kStateCommence)
          exit
        endif
        setState(kStateInnerLoop)
        exit
      endof
      of(OFF)
        attempt_off(mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(DRINK)
        has_water(mLoc) bool stream_here!
        false bool evian_here!
        if(here(BOTTLE mLoc)) andif(bottle_contents WATER =)
          true evian_here!
        endif
        if(mObj NOTHING =)
          if(not(stream_here)) andif(not(evian_here))
            setState(kStateGetObject)
            exit
          endif
        elseif(mObj WATER <>)
          puts("Don't be ridiculous!")
          setState(kStateInnerLoop)
          exit
        endif
        \ Drink from the bottle if we can; otherwise from the stream.
        if(evian_here)
          objs(BOTTLE).setProp(1)  \ empty
          objs(WATER).setPlace(R_LIMBO)
          puts("The bottle of water is now empty.")
        else
          puts("You have taken a drink from the stream.  The water tastes strongly of\n\+
minerals, but is not unpleasant.  It is extremely cold.")
        endif
        setState(kStateInnerLoop)
        exit
      endof
      of(POUR)
        if(mObj NOTHING =) orif(mObj BOTTLE =)
          bottle_contents mObj!
          if(mObj NOTHING =)
            setState(kStateGetObject)
            exit
          endif
          \ Notice that POUR BOTTLE when the bottle is empty
          \ will actually result in the message "Bottle what?".
        endif
        if(toting(mObj))
          if(mObj WATER <>) andif(mObj OIL <>)
            puts("You can't pour that.")
            setState(kStateInnerLoop)
            exit
          endif
          objs(BOTTLE).setProp(1)  \ empty
          objs(mObj).setPlace(R_LIMBO)
          if(there(PLANT mLoc))
            \ Try to water the plant.
            if(mObj WATER <>)
              puts("The plant indignantly shakes the oil off its leaves and asks, \"Water?\"")
              setState(kStateInnerLoop)
              exit
            else
              if(objs(PLANT).prop 0=)
                puts("The plant spurts into furious growth for a few seconds.")
                objs(PLANT).setProp(1)
              elseif(objs(PLANT).prop 1 =)
                puts("The plant grows explosively, almost filling the bottom of the pit.")
                objs(PLANT).setProp(2)
              elseif(objs(PLANT).prop 2 =)
                puts("You've over-watered the plant! It's shriveling up! It's, it's...")
                objs(PLANT).setProp(0)
              endif
              objs(PLANT2).setProp(objs(PLANT).prop)
              NOWHERE mMotion!
              setState(kStateTryMove)
              exit
            endif
          elseif(there(RUSTY_DOOR mLoc))
            \ Pour water or oil on the door.
            case(mObj)
              of(WATER)
                objs(RUSTY_DOOR).setProp(0)
                puts("The hinges are quite thoroughly rusted now and won't budge.")
              endof
              of(OIL)
                objs(RUSTY_DOOR).setProp(1)
                puts("The oil has freed up the hinges so that the door will now open.")
              endof
            endcase
          else
            puts("Your bottle is empty and the ground is wet.")
          endif
        else
          puts("You aren't carrying it!")
        endif
        setState(kStateInnerLoop)
        exit
      endof
      of(FILL)
        if(attempt_fill(mObj mLoc))
          setState(kStateGetObject)
          exit
        endif
        setState(kStateInnerLoop)
        exit
      endof
      of(TAKE)
        if(attempt_take(mObj mLoc))
          TAKE mOldVerb!
          FILL mVerb!
          BOTTLE mObj!
          setState(kStateTransitive)
          exit
        endif
        setState(kStateInnerLoop)
        exit
      endof
      of(DROP)
        attempt_drop(mObj mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(TOSS)
        if(mObj ROD =) andif(toting(ROD2)) andif(not(toting(ROD)))
          ROD2 mObj!
        endif
        if(not(toting(mObj)))
          puts("You aren't carrying it!")
          setState(kStateInnerLoop)
          exit
        endif
        if(is_treasure(mObj)) andif(is_at_loc(TROLL mLoc))
          \ Snarf a treasure for the troll.
          dropObject(mObj R_LIMBO) 
          destroy(TROLL)  destroy(TROLL_) 
          dropObject(NO_TROLL R_SWSIDE)  dropObject(NO_TROLL_ R_NESIDE) 
          juggle(CHASM)  juggle(CHASM_) 
          puts("The troll catches your treasure and scurries away out of sight.")
          setState(kStateInnerLoop)
          exit
        endif
        if(mObj FOOD =) andif(here(BEAR mLoc))
          TOSS mOldVerb!
          FEED mVerb!
          BEAR mObj!
          setState(kStateTransitive)
          exit
        endif
        if(mObj AXE <>)
          TOSS mOldVerb!
          DROP mVerb!
          setState(kStateTransitive)
          exit
        endif
        if(dwarf_at(mLoc))
          throw_axe_at_dwarf(mLoc) 
        elseif(is_at_loc(DRAGON mLoc)) andif(not(objs(DRAGON).prop))
          puts("The axe bounces harmlessly off the dragon's thick scales.")
        elseif(is_at_loc(TROLL mLoc))
          puts("The troll deftly catches the axe, examines it carefully, and tosses it\n\+
back, declaring, \"Good workmanship, but it's not valuable enough.\"")
        elseif(here(BEAR mLoc)) andif(objs(BEAR).prop 0=)
          \ Throw the axe at the (un-tame) bear.
          dropObject(AXE mLoc) 
          objs(AXE).setProp(1)
          immobilize(AXE) 
          juggle(BEAR)   \ keep bear first in the list
          puts("The axe misses and lands near the bear where you can't get at it.")
          setState(kStateInnerLoop)
          exit
        else
          NOTHING mObj!
          TOSS mOldVerb!
          KILL mVerb!
          setState(kStateTransitive)
          exit
        endif
        dropObject(AXE mLoc) 
        NOWHERE mMotion!
        setState(kStateTryMove)
        exit
      endof
      of(KILL)
        if(mObj NOTHING =)
          \ See if there's a unique object to attack.
          0 int kKill!
          if(dwarf_at(mLoc)) kKill++   DWARF mObj! endif
          if(here(SNAKE mLoc)) kKill++   SNAKE mObj! endif
          if(is_at_loc(DRAGON mLoc)) andif(not(objs(DRAGON).prop)) kKill++   DRAGON mObj! endif
          if(is_at_loc(TROLL mLoc)) kKill++   TROLL mObj! endif
          if(here(BEAR mLoc)) andif(not(objs(BEAR).prop)) kKill++   BEAR mObj! endif
          if(kKill 0=)
            \ no enemies present
            if(here(BIRD mLoc)) andif(mOldVerb TOSS <>) kKill++   BIRD mObj! endif
            if(here(CLAM mLoc)) orif(here(OYSTER mLoc)) kKill++   CLAM mObj! endif
          endif
          if(kKill 1 >)
            setState(kStateGetObject)
            exit
          endif
        endif
        case(mObj)
          of(NOTHING)
            puts("There is nothing here to attack.")
            setState(kStateInnerLoop)
            exit
          endof
          of(BIRD)
            if(closed)
              puts("Oh, leave the poor unhappy bird alone.")
            else
              destroy(BIRD) 
              objs(BIRD).setProp(0)
              if(there(SNAKE R_HMK)) lost_treasures++ endif
              puts("The little bird is now dead.  Its body disappears.")
            setState(kStateInnerLoop)
            exit
            endif
          endof
          of(DRAGON)
            if(objs(DRAGON).prop)
              puts("For crying out loud, the poor thing is already dead!")
              setState(kStateInnerLoop)
              exit
            else
              \ If you insist on attacking the dragon, you win!
              \ He dies, the Persian rug becomes free, and R_SCAN2
              \ takes the place of R_SCAN1 and R_SCAN3.
              puts("With what?  Your bare hands?")
              NOTHING mVerb!
              NOTHING mObj!
              listen
              if(streq(inWord1.base "yes")) orif(streq(inWord1.base "y"))
                puts("Congratulations!  You have just vanquished a dragon with your bare\n\+
hands! (Unbelievable, isn't it?)")
                objs(DRAGON).setProp(1)  \ dead
                objs(RUG).setProp(0)
                mobilize(RUG) 
                destroy(DRAGON_) 
                destroy(RUG_) 
                do(MAX_OBJ 1+ MIN_OBJ)
                  if(there(i R_SCAN1)) orif(there(i R_SCAN3))
                    move(i R_SCAN2) 
                  endif
                loop
                R_SCAN2 mLoc!
                NOWHERE mMotion!
                setState(kStateTryMove)
                exit
              else
                setState(kStatePreParse)
                exit
              endif
            endif
          endof
          of(CLAM)
          of(OYSTER)
            puts("The shell is very strong and impervious to attack.")
            setState(kStateInnerLoop)
            exit
          endof
          of(SNAKE)
            puts("Attacking the snake both doesn't work and is very dangerous.")
            setState(kStateInnerLoop)
            exit
          endof
          of(DWARF)
            if(closed) dwarves_upset endif
            puts("With what?  Your bare hands?")
            setState(kStateInnerLoop)
            exit
          endof
          of(TROLL)
            puts("Trolls are close relatives with the rocks and have skin as tough as\n\+
a rhinoceros hide.  The troll fends off your blows effortlessly.")
            setState(kStateInnerLoop)
            exit
          endof
          of(BEAR)
            case(objs(BEAR).prop)
              of(0) puts("With what?  Your bare hands?  Against *HIS* bear hands??") endof
              of(3) puts("For crying out loud, the poor thing is already dead!") endof
              puts("The bear is confused; he only wants to be your friend.")
            endcase
            setState(kStateInnerLoop)
            exit
          endof
          puts("Don't be ridiculous!")
          setState(kStateInnerLoop)
          exit
        endcase
      endof
      of(FEED)
        attempt_feed(mObj mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(OPEN) of(CLOSE)
        attempt_open_or_close(mVerb mObj mLoc) 
        setState(kStateInnerLoop)
        exit
      endof
      of(READ)
        if(now_in_darkness(mLoc))
          setState(kStateCantSeeIt)
          exit
        endif
        attempt_read(mObj) 
        setState(kStateInnerLoop)
        exit
      endof
      of(CALM)
        puts("I'm game. Would you care to explain how?")
        setState(kStateInnerLoop)
        exit
      endof
      of(GO)
        puts("Where?")
        setState(kStateInnerLoop)
        exit
      endof
      of(RELAX)
        puts(okMsg)   \ this corresponds to the command "NOTHING LAMP"
        setState(kStateInnerLoop)
        exit
      endof
      of(FEEFIE)
        puts("I don't know how.")  \ "FOO BARS"
        setState(kStateInnerLoop)
        exit
      endof
      of(BRIEF)
        puts("On what?")  \ har har har
        setState(kStateInnerLoop)
        exit
      endof
      of(SCORE)
      of(QUIT)
#ifdef SAVE_AND_RESTORE
      of(SAVE)
      of(RESTORE)
#endif \ SAVE_AND_RESTORE
        puts("Eh?")
        setState(kStateInnerLoop)
        exit
      endof
    endcase
    setState(kStateGetObject)
  ;
  
  : stateGetObject    \ state 8
    toupper(inWord1.base c@) inWord1.base c!
    inWord1.base %s " what\n" %s
    setState(kStateCycle)
  ;
  
  : stateCantSeeIt    \ state 9
    if(mVerb FIND = mVerb INVENTORY = or inWord2.base c@ 0<> and)
      setState(kStateTransitive)
      exit
    endif
            
    "I see no " %s inWord1.base %s " here.\n" %s
    setState(kStateInnerLoop)
  ;
  
  : stateTryMove    \ state 10
    \ A major cycle comes to an end when a motion verb mMotion has been
    \ given and we have computed the appropriate mNewLoc accordingly.
    \ assert(R_LIMBO <= mOldLoc) andif(mOldLoc <= MAX_LOC) 
    \ assert(R_LIMBO < mLoc) andif(mLoc <= MAX_LOC) 
    Location backLoc
    mLoc mNewLoc!   \ by default we will stay put

    if(mMotion CAVE =)
      \ No CAVE appears in the travel table; its sole purpose is to
      \ give these messages.
      if(mLoc MIN_IN_CAVE <)
        puts("I can't see where the cave is, but hereabouts no stream can run on\n\+
the surface for long. I would try the stream.")
      else
        puts("I need more detailed instructions to do that.")
      endif
    elseif(mMotion LOOK =)
      \ Repeat the long description and continue.
      mLookCount++
      if(mLookCount 3 <=)
        puts("Sorry, but I am not allowed to give more detail.  I will repeat the\n\+
long description of your location.")
      endif
      false mWasDark!  \ pretend it wasn't dark, so you won't fall into a pit
      places.get(mLoc).setVisits(0)
    else
      if(mMotion BACK =)
        if(is_forced(mOldLoc))  mOldOldLoc  else  mOldLoc endif backLoc!
        try_going_back_to(backLoc mLoc) mMotion!  \ may return NOWHERE
      endif

      if(mMotion NOWHERE <>)
        \ Determine the next mNewLoc.
        mOldLoc mOldOldLoc!
        mLoc mOldLoc!
        if(determine_next_newloc(mLoc ref mNewLoc mMotion mVerb))
          \ Player died trying to cross the troll bridge.
          mNewLoc mOldOldLoc!  \ if you are revived, you got across
          setState(kStateDeath)
          exit
        endif
      endif
    endif
    setState(kStateLoopTop)
  ;

  : statePitchDark    \ state 11
    puts("You fell into a pit and broke every bone in your body!")
    mLoc mOldOldLoc!
    setState(kStateDeath)
  ;
    
  : stateDeath    \ state 12
    \ When you die, mNewLoc is undefined (often R_LIMBO) and mOldLoc is what
    \ killed you. So we look at mOldOldLoc, the last place you were safe.
    kill_the_player(mOldOldLoc) 
    R_HOUSE mLoc!
    R_HOUSE mOldLoc!
    setState(kStateCommence)
  ;
    
  : stateQuit    \ state 13
    true mQuit!
  ;
  
  : makeState
    mko String root
    mko String result
    mko AdventureState advState
    root.set(blword)
    result.resize(64)
    result.appendFormatted("\"%s\" \s state%s kState%s" root.get dup dup 3)
    d[ "Defining " %s root.get %s  %bl result.get %s %nl ]d
    result.get $evaluate
    advState.init
    Game_adventureStates.push(advState)
    advState~
    root~
    result~
  ;

  makeState LoopTop
  makeState Commence
  makeState InnerLoop
  makeState Cycle
  makeState PreParse
  makeState Parse
  makeState Intransitive
  makeState Transitive
  makeState GetObject
  makeState CantSeeIt
  makeState TryMove
  makeState PitchDark
  makeState Death
  makeState Quit
  
  
  m: initialSetup
    offer(0) 
    \ Reading the instructions marks you as a newbie who will need the
    \ extra lamp power. However, it will also cost you 5 points.
    if(hints.get(0).given) 1000 else 330 endif lamp_limit!
    initDwarves
  ;m
  
;class



loaddone

\ =====================================================================================================
\ =====================================================================================================
\ =====================================================================================================
\ =====================================================================================================
