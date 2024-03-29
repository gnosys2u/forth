\ ========== Locations. ===================================================
\ This section corresponds to sections 18--62 in Knuth.

: advLocations ;

\ struct Place places[MAX_LOC + 1];

null ptrTo Instruction __nextLoc!

\ make_loc(locationId longDescription shortDescription)
\   adds a Place to game.places
\   sets game.start[locationId] = first index in game.travels for this location

\ make_loc_hint(locationId longDescription shortDescription flags)
\   adds a Place to game.places with hint flags

\ make_inst(motionWord conditionId destLocationId)
\   sets motion info in current entry in game.travels, doesn't increment current entry

\ make_ins(motionWord destLocationId)
\   adds a non-conditional entry to game.travels, increments current entry

\ make_cond_ins(motionWord conditionId destLocationId)
\   adds a conditional entry to game.travels, increments current entry

\ ditto(motionWord)
\    adds another entry to game.travels, same as previous entry except for motionWord

\ void make_loc(Instruction *q, Location x, const char *l, const char *s, unsigned int f)

\ create a location with a hint
: make_loc_hint
  mko Place place
  place.init
  d[ tr{ "make_loc " %s place.long_desc.get %s %bl place.locID %d %nl ds }tr ]d

  if(place.locID game.places.count <>)
    "WARNING: adding location " %s place.locID %d " at index " %s game.places.count %d %nl
    place.show
  else
    game.places.push(place)
  endif
  __nextLoc game.start!(place.locID)
  \ place.show
  place~
;

\ create a location with no hints
: make_loc 0 make_loc_hint ;

\ void make_inst(Instruction *q, MotionWord m, int cond, Location dest)
: make_inst
\    assert(&travels[0] <= q && q < &travels[733]);
\    assert(m == 0 || (MIN_MOTION <= m && m <= MAX_MOTION));
  __nextLoc.dest!
  __nextLoc.cond!
  __nextLoc.mot!
  d[ tr{ "make_inst " %s __nextLoc.dest %d %nl }tr ]d
;

\ #define make_ins(m, d) make_inst(q++, m, 0, d)
: make_ins
  int d!
  int m!
  make_inst(m 0 d)
  sizeOf Instruction __nextLoc!+
;

\ #define make_cond_ins(m, c, d) make_inst(q++, m, c, d)
: make_cond_ins
  make_inst
  sizeOf Instruction __nextLoc!+
;

\ #define ditto(m) make_inst(q, m, q[-1].cond, q[-1].dest)  ++q;
: ditto
  int m!
  __nextLoc sizeOf Instruction - ptrTo Instruction previousInstruction!
  make_inst(m previousInstruction.cond previousInstruction.dest)
  sizeOf Instruction __nextLoc!+
;

\ cond values:
\ 0            no condition, always obey motion word
\ 100 - 199    obey command if carrying object(cond - 100)
\ 200 - 299    obey command if object(cond-200) is here
\ 300 - ?      obey command if property(cond % 100) has value ((cond-300) / 100)
: only_if_toting
  MIN_OBJ - 100+
;

: only_if_here
  MIN_OBJ - 200+
;

\ #define unless_prop(t, p) (300 + (t-MIN_OBJ) + 100*p)
: unless_prop
  100 * MIN_OBJ - + 300 +
;

: remark FIRST_REMARK + ;

: twist
  ptrTo byte m!
  int d!
  int u!
  int sw!
  int nw!
  int se!
  int ne!
  int w!
  int e!
  int s!
  int n!
  int name!
  make_loc(name m null)
  make_ins(N n)  make_ins(S s)  make_ins(E e)  make_ins(W w)
  make_ins(NE ne)  make_ins(SE se)  make_ins(NW nw)  make_ins(SW sw)
  make_ins(U u)  make_ins(D d) 
;

"You are in a maze of twisty little passages, all alike." $constant all_alike
"Dead end." $constant dead_end

: build_travel_table
  game.travels&(0) __nextLoc!
  game.places.push(null)   \ TODO: why do we need to do this?
  
  make_loc(R_ROAD "You are standing at the end of a road before a small brick building.\n\+
Around you is a forest.  A small stream flows out of the building and\n\+
down a gully."  "You're at end of road again.")
  make_ins(W R_HILL)  ditto(U)  ditto(ROAD) 
  make_ins(E R_HOUSE)  ditto(IN)  ditto(HOUSE)  ditto(ENTER) 
  make_ins(S R_VALLEY)  ditto(D)  ditto(GULLY)  ditto(STREAM)  ditto(DOWNSTREAM) 
  make_ins(N R_FOREST)  ditto(FOREST) 
  make_ins(DEPRESSION R_OUTSIDE) 
  
  make_loc(R_HILL  "You have walked up a hill, still in the forest.  The road slopes back\n\+
down the other side of the hill.  There is a building in the distance."  "You're at hill in road.") 
  make_ins(ROAD R_ROAD)  ditto(HOUSE)  ditto(FORWARD)  ditto(E)  ditto(D) 
  make_ins(FOREST R_FOREST)  ditto(N)  ditto(S) 

  make_loc(R_HOUSE "You are inside a building, a well house for a large spring." "You're inside building.") 
  make_ins(ENTER R_ROAD)  ditto(OUT)  ditto(OUTDOORS)  ditto(W) 
  make_ins(XYZZY R_DEBRIS) 
  make_ins(PLUGH R_Y2) 
  make_ins(DOWNSTREAM remark(17)) ditto(STREAM) 

  make_loc(R_VALLEY "You are in a valley in the forest beside a stream tumbling along a\n\+
rocky bed." "You're in valley.")
  make_ins(UPSTREAM R_ROAD)  ditto(HOUSE)  ditto(N) 
  make_ins(FOREST R_FOREST)  ditto(E)  ditto(W)  ditto(U) 
  make_ins(DOWNSTREAM R_SLIT)  ditto(S)  ditto(D) 
  make_ins(DEPRESSION R_OUTSIDE) 

  make_loc(R_FOREST "You are in open forest, with a deep valley to one side." "You're in forest.") 
  make_ins(VALLEY R_VALLEY)  ditto(E)  ditto(D) 
  make_cond_ins(FOREST 50 R_FOREST)  ditto(FORWARD)  ditto(N) 
  make_ins(FOREST R_FOREST2) 
  make_ins(W R_FOREST)  ditto(S) 

  make_loc(R_FOREST2 "You are in open forest near both a valley and a road." "You're in forest.") 
  make_ins(ROAD R_ROAD)  ditto(N) 
  make_ins(VALLEY R_VALLEY)  ditto(E)  ditto(W)  ditto(D) 
  make_ins(FOREST R_FOREST)  ditto(S) 
  make_loc(R_SLIT "At your feet all the water of the stream splashes into a 2-inch slit\n\+
in the rock.  Downstream the streambed is bare rock." "You're at slit in streambed.") 
  make_ins(HOUSE R_ROAD) 
  make_ins(UPSTREAM R_VALLEY)  ditto(N) 
  make_ins(FOREST R_FOREST)  ditto(E)  ditto(W) 
  make_ins(DOWNSTREAM R_OUTSIDE)  ditto(ROCK)  ditto(BED)  ditto(S) 
  make_ins(SLIT remark(0)) ditto(STREAM)  ditto(D) 
  
  make_loc_hint(R_OUTSIDE "You are in a 20-foot depression floored with bare dirt.  Set into the\n\+
dirt is a strong steel grate mounted in concrete.  A dry streambed\n\+
leads into the depression."  "You're outside grate." F_CAVE_HINT) 
  make_ins(FOREST  R_FOREST)  ditto(E)  ditto(W)  ditto(S) 
  make_ins(HOUSE  R_ROAD) 
  make_ins(UPSTREAM  R_SLIT)  ditto(GULLY)  ditto(N) 
  make_cond_ins(ENTER  unless_prop(GRATE  0)  R_INSIDE)  ditto(IN)  ditto(D) 
  make_ins(ENTER  remark(1))
  
  make_loc(R_INSIDE  "You are in a small chamber beneath a 3x3 steel grate to the surface.\n\+
A low crawl over cobbles leads inward to the west." "You're below the grate.")
  make_cond_ins(OUT  unless_prop(GRATE  0)  R_OUTSIDE)  ditto(U) 
  make_ins(OUT remark(1))
  make_ins(CRAWL  R_COBBLES)  ditto(COBBLES)  ditto(IN)  ditto(W) 
  make_ins(PIT  R_SPIT) 
  make_ins(DEBRIS  R_DEBRIS) 

  make_loc(R_COBBLES  "You are crawling over cobbles in a low passage.  There is a dim light\n\+
at the east end of the passage." "You're in cobble crawl.") 
  make_ins(OUT R_INSIDE)  ditto(SURFACE)  ditto(E) 
  make_ins(IN R_DEBRIS)  ditto(DARK)  ditto(W)  ditto(DEBRIS) 
  make_ins(PIT  R_SPIT) 
  
  make_loc(R_DEBRIS  "You are in a debris room filled with stuff washed in from the surface.\n\+
A low wide passage with cobbles becomes plugged with mud and debris\n\+
here, but an awkward canyon leads upward and west.  A note on the wall\n\+
says \"MAGIC WORD XYZZY\"."  "You're in debris room.") 
  make_cond_ins(DEPRESSION  unless_prop(GRATE  0)  R_OUTSIDE) 
  make_ins(ENTRANCE  R_INSIDE) 
  make_ins(CRAWL  R_COBBLES)  ditto(COBBLES)  ditto(PASSAGE)  ditto(LOW)  ditto(E) 
  make_ins(CANYON  R_AWK)  ditto(IN)  ditto(U)  ditto(W) 
  make_ins(XYZZY  R_HOUSE) 
  make_ins(PIT  R_SPIT) 
  
  make_loc(R_AWK  "You are in an awkward sloping east/west canyon." null) 
  make_cond_ins(DEPRESSION  unless_prop(GRATE  0)  R_OUTSIDE) 
  make_ins(ENTRANCE  R_INSIDE) 
  make_ins(D R_DEBRIS)  ditto(E)  ditto(DEBRIS) 
  make_ins(IN R_BIRD)  ditto(U)  ditto(W) 
  make_ins(PIT  R_SPIT) 
 
  make_loc_hint(R_BIRD  "You are in a splendid chamber thirty feet high. The walls are frozen\n\+
rivers of orange stone. An awkward canyon and a good passage exit\n\+
from east and west sides of the chamber." "You're in bird chamber." F_BIRD_HINT) 
  make_cond_ins(DEPRESSION  unless_prop(GRATE  0)  R_OUTSIDE) 
  make_ins(ENTRANCE  R_INSIDE) 
  make_ins(DEBRIS  R_DEBRIS) 
  make_ins(CANYON  R_AWK)  ditto(E) 
  make_ins(PASSAGE  R_SPIT)  ditto(PIT)  ditto(W) 

  make_loc(R_SPIT  "At your feet is a small pit breathing traces of white mist. An east\n\+
passage ends here except for a small crack leading on." "You're at top of small pit.") 
  make_cond_ins(DEPRESSION  unless_prop(GRATE  0)  R_OUTSIDE) 
  make_ins(ENTRANCE  R_INSIDE) 
  make_ins(DEBRIS  R_DEBRIS) 
  make_ins(PASSAGE  R_BIRD)  ditto(E) 
  make_cond_ins(D  only_if_toting(GOLD)  R_NECK)  ditto(PIT)  ditto(STEPS) 
    \ good thing you weren't loaded down with GOLD
  make_ins(D R_EMIST) 
  make_ins(CRACK  remark(14)) ditto(W) 

  make_loc(R_EMIST  "You are at one end of a vast hall stretching forward out of sight to\n\+
the west.  There are openings to either side.  Nearby, a wide stone\n\+
staircase leads downward.  The hall is filled with wisps of white mist\n\+
swaying to and fro almost as if alive.  A cold wind blows up the\n\+
staircase.  There is a passage at the top of a dome behind you." "You're in Hall of Mists.") 
  make_ins(LEFT  R_NUGGET)  ditto(S) 
  make_ins(FORWARD  R_EFISS)  ditto(HALL)  ditto(W) 
  make_ins(STAIRS  R_HMK)  ditto(D)  ditto(N) 
  make_cond_ins(U  only_if_toting(GOLD)  remark(15)) ditto(PIT)  ditto(STEPS)  ditto(DOME)  ditto(PASSAGE)  ditto(E) 
  make_ins(U R_SPIT) 
  make_ins(Y2  R_JUMBLE) 

  make_loc(R_NUGGET  "This is a low room with a crude note on the wall. The note says,\n\+
\"You won't get it up the steps\"."  "You're in nugget of gold room.") 
  make_ins(HALL  R_EMIST)  ditto(OUT)  ditto(N) 
  
  make_loc(R_EFISS  "You are on the east bank of a fissure slicing clear across the hall.\n\+
The mist is quite thick here, and the fissure is too wide to jump."  "You're on east bank of fissure.") 
  make_ins(HALL  R_EMIST)  ditto(E) 
  make_cond_ins(JUMP  unless_prop(FISSURE  0)  remark(2))
  make_cond_ins(FORWARD  unless_prop(FISSURE  1)  R_LOSE) 
  make_cond_ins(OVER  unless_prop(FISSURE  1)  remark(3)) ditto(ACROSS)  ditto(W)  ditto(CROSS) 
  make_ins(OVER  R_WFISS) 
  
  make_loc(R_WFISS  "You are on the west side of the fissure in the Hall of Mists." null) 
  make_cond_ins(JUMP  unless_prop(FISSURE  0)  remark(2))
  make_cond_ins(FORWARD  unless_prop(FISSURE  1)  R_LOSE) 
  make_cond_ins(OVER  unless_prop(FISSURE  1)  remark(3)) ditto(ACROSS)  ditto(E)  ditto(CROSS) 
  make_ins(OVER  R_EFISS) 
  make_ins(W R_WMIST) 
  make_ins(N R_THRU) 

  make_loc(R_WMIST "You are at the west end of the Hall of Mists. A low wide crawl\n\+
continues west and another goes north. To the south is a little\n\+
passage 6 feet off the floor." "You're at west end of Hall of Mists.") 
  make_ins(S R_LIKE1)  ditto(U)  ditto(PASSAGE)  ditto(CLIMB) 
  make_ins(E R_WFISS) 
  make_ins(W R_ELONG)  ditto(CRAWL) 
  make_ins(N R_DUCK) 

  make_loc_hint(R_LIKE1  all_alike  null  F_TWIST_HINT) 
  make_ins(U R_WMIST) 
  make_ins(N R_LIKE1) 
  make_ins(E R_LIKE2) 
  make_ins(S R_LIKE4) 
  make_ins(W R_LIKE11) 
  
  make_loc_hint(R_LIKE2  all_alike  null  F_TWIST_HINT) 
  make_ins(W R_LIKE1) 
  make_ins(S R_LIKE3) 
  make_ins(E R_LIKE4) 

  make_loc_hint(R_LIKE3  all_alike  null  F_TWIST_HINT) 
  make_ins(E R_LIKE2) 
  make_ins(D R_DEAD5) 
  make_ins(S R_LIKE6) 
  make_ins(N R_DEAD9) 

  make_loc_hint(R_LIKE4  all_alike  null  F_TWIST_HINT) 
  make_ins(W R_LIKE1) 
  make_ins(N R_LIKE2) 
  make_ins(E R_DEAD3) 
  make_ins(S R_DEAD4) 
  make_ins(U R_LIKE14)  ditto(D) 

  make_loc_hint(R_LIKE5  all_alike  null  F_TWIST_HINT) 
  make_ins(E R_LIKE6) 
  make_ins(W R_LIKE7) 

  make_loc_hint(R_LIKE6  all_alike  null  F_TWIST_HINT) 
  make_ins(E R_LIKE3) 
  make_ins(W R_LIKE5) 
  make_ins(D R_LIKE7) 
  make_ins(S R_LIKE8) 

  make_loc_hint(R_LIKE7  all_alike  null  F_TWIST_HINT) 
  make_ins(W R_LIKE5) 
  make_ins(U R_LIKE6) 
  make_ins(E R_LIKE8) 
  make_ins(S R_LIKE9) 

  make_loc_hint(R_LIKE8  all_alike  null F_TWIST_HINT) 
  make_ins(W R_LIKE6) 
  make_ins(E R_LIKE7) 
  make_ins(S R_LIKE8) 
  make_ins(U R_LIKE9) 
  make_ins(N R_LIKE10) 
  make_ins(D R_DEAD11) 

  make_loc_hint(R_LIKE9  all_alike  null F_TWIST_HINT) 
  make_ins(W R_LIKE7) 
  make_ins(N R_LIKE8) 
  make_ins(S R_DEAD6) 

  make_loc_hint(R_LIKE10  all_alike  null F_TWIST_HINT) 
  make_ins(W R_LIKE8) 
  make_ins(N R_LIKE10) 
  make_ins(D R_DEAD7) 
  make_ins(E R_BRINK) 

  make_loc_hint(R_LIKE11  all_alike  null F_TWIST_HINT) 
  make_ins(N R_LIKE1) 
  make_ins(W R_LIKE11)  ditto(S) 
  make_ins(E R_DEAD1) 

  make_loc_hint(R_LIKE12  all_alike  null F_TWIST_HINT) 
  make_ins(S R_BRINK) 
  make_ins(E R_LIKE13) 
  make_ins(W R_DEAD10) 

  make_loc_hint(R_LIKE13  all_alike  null F_TWIST_HINT) 
  make_ins(N R_BRINK) 
  make_ins(W R_LIKE12) 
  make_ins(NW  R_PIRATES_NEST)   \ NW: a dirty trick!

  make_loc_hint(R_LIKE14  all_alike  null F_TWIST_HINT) 
  make_ins(U R_LIKE4)  ditto(D) 

  make_loc(R_BRINK "You are on the brink of a thirty-foot pit with a massive orange column\n\+
down one wall.  You could climb down here but you could not get back\n\+
up.  The maze continues at this level." "You're at brink of pit.") 
  make_ins(D R_BIRD)  ditto(CLIMB) 
  make_ins(W R_LIKE10) 
  make_ins(S R_DEAD8) 
  make_ins(N R_LIKE12) 
  make_ins(E R_LIKE13) 

  make_loc(R_ELONG "You are at the east end of a very long hall apparently without side\n\+
chambers.  To the east a low wide crawl slants up.  To the north a\n\+
round two-foot hole slants down." "You're at east end of long hall.") 
  make_ins(E R_WMIST)  ditto(U)  ditto(CRAWL) 
  make_ins(W R_WLONG) 
  make_ins(N R_CROSS)  ditto(D)  ditto(HOLE) 
  
  make_loc(R_WLONG  "You are at the west end of a very long featureless hall.  The hall\n\+
joins up with a narrow north/south passage." "You're at west end of long hall.") 
  make_ins(E R_ELONG) 
  make_ins(N R_CROSS) 
  make_cond_ins(S  100  R_DIFF0)   \ 100: Dwarves Not Permitted.

  twist(R_DIFF0 R_DIFF9 R_DIFF1 R_DIFF7 R_DIFF8 R_DIFF3 R_DIFF4 R_DIFF6 R_DIFF2 R_DIFF5 R_WLONG \+
        "You are in a maze of twisty little passages, all different.")
  twist(R_DIFF1 R_DIFF8 R_DIFF9 R_DIFF10 R_DIFF0 R_DIFF5 R_DIFF2 R_DIFF3 R_DIFF4 R_DIFF6 R_DIFF7 \+
        "You are in a maze of twisting little passages, all different.")
  twist(R_DIFF2 R_DIFF3 R_DIFF4 R_DIFF8 R_DIFF5 R_DIFF7 R_DIFF10 R_DIFF0 R_DIFF6 R_DIFF1 R_DIFF9 \+
        "You are in a little maze of twisty passages, all different.")
  twist(R_DIFF3 R_DIFF7 R_DIFF10 R_DIFF6 R_DIFF2 R_DIFF4 R_DIFF9 R_DIFF8 R_DIFF5 R_DIFF0 R_DIFF1 \+
        "You are in a twisting maze of little passages, all different.")
  twist(R_DIFF4 R_DIFF1 R_DIFF7 R_DIFF5 R_DIFF9 R_DIFF0 R_DIFF3 R_DIFF2 R_DIFF10 R_DIFF8 R_DIFF6 \+
        "You are in a twisting little maze of passages, all different.")
  twist(R_DIFF5 R_DIFF0 R_DIFF3 R_DIFF4 R_DIFF6 R_DIFF8 R_DIFF1 R_DIFF9 R_DIFF7 R_DIFF10 R_DIFF2 \+
        "You are in a twisty little maze of passages, all different.")
  twist(R_DIFF6 R_DIFF10 R_DIFF5 R_DIFF0 R_DIFF1 R_DIFF9 R_DIFF8 R_DIFF7 R_DIFF3 R_DIFF2 R_DIFF4 \+
        "You are in a twisty maze of little passages, all different.")
  twist(R_DIFF7 R_DIFF6 R_DIFF2 R_DIFF9 R_DIFF10 R_DIFF1 R_DIFF0 R_DIFF5 R_DIFF8 R_DIFF4 R_DIFF3 \+
        "You are in a little twisty maze of passages, all different.")
  twist(R_DIFF8 R_DIFF5 R_DIFF6 R_DIFF1 R_DIFF4 R_DIFF2 R_DIFF7 R_DIFF10 R_DIFF9 R_DIFF3 R_DIFF0 \+
        "You are in a maze of little twisting passages, all different.")
  twist(R_DIFF9 R_DIFF4 R_DIFF8 R_DIFF2 R_DIFF3 R_DIFF10 R_DIFF6 R_DIFF1 R_DIFF0 R_DIFF7 R_DIFF5 \+
        "You are in a maze of little twisty passages, all different.")
  twist(R_DIFF10 R_DIFF2 R_PONY R_DIFF3 R_DIFF7 R_DIFF6 R_DIFF5 R_DIFF4 R_DIFF1 R_DIFF9 R_DIFF8 \+
        "You are in a little maze of twisting passages, all different.")


  make_loc(R_PONY  dead_end  null) 
  make_ins(N R_DIFF10)  ditto(OUT) 
  make_loc(R_CROSS "You are at a crossover of a high N/S passage and a low E/W one." null) 
  make_ins(W R_ELONG) 
  make_ins(N R_DEAD0) 
  make_ins(E R_WEST) 
  make_ins(S R_WLONG) 

  make_loc_hint(R_HMK "You are in the Hall of the Mountain King, with passages off in all\n\+
directions."  "You're in Hall of Mt King."  F_SNAKE_HINT) 
  make_ins(STAIRS  R_EMIST)  ditto(U)  ditto(E) 
    \ I suppose our adventurer must be walking on the ceiling!
  make_cond_ins(N  unless_prop(SNAKE  0)  R_NS)  ditto(LEFT) 
  make_cond_ins(S  unless_prop(SNAKE  0)  R_SOUTH)  ditto(RIGHT) 
  make_cond_ins(W  unless_prop(SNAKE  0)  R_WEST)  ditto(FORWARD) 
  make_ins(N remark(16))
  make_cond_ins(SW  35  R_SECRET) 
  make_cond_ins(SW  only_if_here(SNAKE)  remark(16))
  make_ins(SECRET  R_SECRET) 
    
  make_loc(R_WEST "You are in the west side chamber of the Hall of the Mountain King.\n\+
A passage continues west and up here." "You're in west side chamber.") 
  make_ins(HALL  R_HMK)  ditto(OUT)  ditto(E) 
  make_ins(W R_CROSS)  ditto(U) 
    
  make_loc(R_SOUTH "You are in the south side chamber." null) 
  make_ins(HALL  R_HMK)  ditto(OUT)  ditto(N) 
    
  make_loc(R_NS "You are in a low N/S passage at a hole in the floor.  The hole goes\n\+
down to an E/W passage." "You're in N/S passage.") 
  make_ins(HALL  R_HMK)  ditto(OUT)  ditto(S) 
  make_ins(N R_Y2)  ditto(Y2) 
  make_ins(D R_DIRTY)  ditto(HOLE) 
    
  make_loc(R_Y2 "You are in a large room, with a passage to the south, a passage to the\n\+
west, and a wall of broken rock to the east.  There is a large \"Y2\" on\n\+
a rock in the room's center." "You're at \"Y2\".") 
  make_ins(PLUGH R_HOUSE) 
  make_ins(S R_NS) 
  make_ins(E R_JUMBLE)  ditto(WALL)  ditto(BROKEN) 
  make_ins(W R_WINDOE) 
  make_cond_ins(PLOVER  only_if_toting(EMERALD)  R_PDROP) 
  make_ins(PLOVER  R_PLOVER) 
    
  make_loc(R_JUMBLE "You are in a jumble of rock, with cracks everywhere." null) 
  make_ins(D R_Y2)  ditto(Y2) 
  make_ins(U R_EMIST) 
    
  make_loc(R_WINDOE "You're at a low window overlooking a huge pit, which extends up out of\n\+
sight.  A floor is indistinctly visible over 50 feet below.  Traces of\n\+
white mist cover the floor of the pit, becoming thicker to the right.\n\+
Marks in the dust around the window would seem to indicate that\n\+
someone has been here recently.  Directly across the pit from you and\n\+
25 feet away there is a similar window looking into a lighted room.\n\+
A shadowy figure can be seen there peering back at you." "You're at window on pit.") 
  make_ins(E R_Y2)  ditto(Y2) 
  make_ins(JUMP  R_NECK) 

  make_loc(R_DIRTY "You are in a dirty broken passage.  To the east is a crawl.  To the\n\+
west is a large passage.  Above you is a hole to another passage." "You're in dirty passage.") 
  make_ins(E R_CLEAN)  ditto(CRAWL) 
  make_ins(U R_NS)  ditto(HOLE) 
  make_ins(W R_DUSTY) 
  make_ins(BEDQUILT  R_BEDQUILT) 
    
  make_loc(R_CLEAN "You are on the brink of a small clean climbable pit.  A crawl leads\n\+
west." "You're by a clean pit.") 
  make_ins(W R_DIRTY)  ditto(CRAWL) 
  make_ins(D R_WET)  ditto(PIT)  ditto(CLIMB) 
    
  make_loc(R_WET "You are in the bottom of a small pit with a little stream, which\n\+
enters and exits through tiny slits."  "You're in pit by stream.") 
  make_ins(CLIMB  R_CLEAN)  ditto(U)  ditto(OUT) 
  make_ins(SLIT  remark(0)) ditto(STREAM)  ditto(D)  ditto(UPSTREAM)  ditto(DOWNSTREAM) 

  make_loc(R_DUSTY "You are in a large room full of dusty rocks.  There is a big hole in\n\+
the floor.  There are cracks everywhere, and a passage leading east." "You're in dusty rock room.")
  make_ins(E R_DIRTY)  ditto(PASSAGE) 
  make_ins(D R_COMPLEX)  ditto(HOLE)  ditto(FLOOR) 
  make_ins(BEDQUILT  R_BEDQUILT) 

  make_loc(R_COMPLEX "You are at a complex junction.  A low hands-and-knees passage from the\n\+
north joins a higher crawl from the east to make a walking passage\n\+
going west.  There is also a large room above.  The air is damp here." "You're at complex junction.") 
  make_ins(U R_DUSTY)  ditto(CLIMB)  ditto(ROOM) 
  make_ins(W R_BEDQUILT)  ditto(BEDQUILT) 
  make_ins(N R_SHELL)  ditto(SHELL) 
  make_ins(E R_ANTE) 

  make_loc(R_SHELL "You're in a large room carved out of sedimentary rock.  The floor\n\+
and walls are littered with bits of shells embedded in the stone.\n\+
A shallow passage proceeds downward, and a somewhat steeper one\n\+
leads up.  A low hands-and-knees passage enters from the south." "You're in Shell Room.") 
  make_ins(U R_ARCHED)  ditto(HALL) 
  make_ins(D R_RAGGED) 
  make_cond_ins(S  only_if_toting(CLAM)  remark(4))
  make_cond_ins(S  only_if_toting(OYSTER)  remark(5))
  make_ins(S R_COMPLEX) 

  make_loc(R_ARCHED "You are in an arched hall.  A coral passage once continued up and east\n\+
from here, but is now blocked by debris.  The air smells of sea water." "You're in arched hall.") 
  make_ins(D R_SHELL)  ditto(SHELL)  ditto(OUT) 

  make_loc(R_RAGGED "You are in a long sloping corridor with ragged sharp walls." null) 
  make_ins(U R_SHELL)  ditto(SHELL) 
  make_ins(D R_SAC) 

  make_loc(R_SAC "You are in a cul-de-sac about eight feet across." null) 
  make_ins(U R_RAGGED)  ditto(OUT) 
  make_ins(SHELL R_SHELL) 

  make_loc(R_ANTE "You are in an anteroom leading to a large passage to the east.  Small\n\+
passages go west and up.  The remnants of recent digging are evident.\n\+
A sign in midair here says \"CAVE UNDER CONSTRUCTION BEYOND THIS POINT.\n\+
PROCEED AT OWN RISK.  [WITT CONSTRUCTION COMPANY]\"" "You're in anteroom.") 
  make_ins(U R_COMPLEX) 
  make_ins(W R_BEDQUILT) 
  make_ins(E R_WITT) 

  make_loc_hint(R_WITT "You are at Witt's End.  Passages lead off in *ALL* directions." "You're at Witt's End." F_WITT_HINT) 
  make_cond_ins(E  95  remark(6)) ditto(N)  ditto(S) 
    ditto(NE)  ditto(SE)  ditto(SW)  ditto(NW)  ditto(U)  ditto(D) 
  make_ins(E R_ANTE) 
  make_ins(W remark(7))

  make_loc(R_BEDQUILT "You are in Bedquilt, a long east/west passage with holes everywhere.\n\+
To explore at random select north, south, up, or down." "You're in Bedquilt.") 
  make_ins(E R_COMPLEX) 
  make_ins(W R_SWISS) 
  make_cond_ins(S  80  remark(6))
  make_ins(SLAB  R_SLAB) 
  make_cond_ins(U  80  remark(6))
  make_cond_ins(U  50  R_ABOVEP) 
  make_ins(U R_DUSTY) 
  make_cond_ins(N  60  remark(6))
  make_cond_ins(N  75  R_LOW) 
  make_ins(N R_SJUNC) 
  make_cond_ins(D  80  remark(6))
  make_ins(D R_ANTE) 

  make_loc(R_SWISS "You are in a room whose walls resemble Swiss cheese.  Obvious passages\n\+
go west, east, NE, and NW.  Part of the room is occupied by a large\n\+
bedrock block." "You're in Swiss cheese room.") 
  make_ins(NE  R_BEDQUILT) 
  make_ins(W R_E2PIT) 
  make_cond_ins(S  80  remark(6))
  make_ins(CANYON  R_TALL) 
  make_ins(E R_SOFT) 
  make_cond_ins(NW  50  remark(6))
  make_ins(ORIENTAL  R_ORIENTAL) 

  make_loc(R_SOFT "You are in the Soft Room.  The walls are covered with heavy curtains,\n\+
the floor with a thick pile carpet.  Moss covers the ceiling." "You're in Soft Room.") 
  make_ins(W R_SWISS)  ditto(OUT) 

  make_loc(R_E2PIT "You are at the east end of the Twopit Room.  The floor here is\n\+
littered with thin rock slabs, which make it easy to descend the pits.\n\+
There is a path here bypassing the pits to connect passages from east\n\+
and west.  There are holes all over, but the only big one is on the\n\+
wall directly over the west pit where you can't get to it." "You're at east end of Twopit Room.") 
  make_ins(E R_SWISS) 
  make_ins(W R_W2PIT)  ditto(ACROSS) 
  make_ins(D R_EPIT)  ditto(PIT) 

  make_loc(R_W2PIT "You are at the west end of the Twopit Room.  There is a large hole in\n\+
the wall above the pit at this end of the room." "You're at west end of Twopit Room.") 
  make_ins(E R_E2PIT)  ditto(ACROSS) 
  make_ins(W R_SLAB)  ditto(SLAB) 
  make_ins(D R_WPIT)  ditto(PIT) 
  make_ins(HOLE  remark(8))

  make_loc(R_EPIT "You are at the bottom of the eastern pit in the Twopit Room.  There is\n\+
a small pool of oil in one corner of the pit." "You're in east pit.") 
  make_ins(U R_E2PIT)  ditto(OUT) 

  make_loc(R_WPIT "You are at the bottom of the western pit in the Twopit Room.  There is\n\+
a large hole in the wall about 25 feet above you." "You're in west pit.") 
  make_ins(U R_W2PIT)  ditto(OUT) 
  make_cond_ins(CLIMB  unless_prop(PLANT  2)  R_CHECK) 
  make_ins(CLIMB  R_CLIMB) 

  make_loc(R_NARROW "You are in a long, narrow corridor stretching out of sight to the\n\+
west.  At the eastern end is a hole through which you can see a\n\+
profusion of leaves." "You're in narrow corridor.") 
  make_ins(D R_WPIT)  ditto(CLIMB)  ditto(E) 
  make_ins(JUMP  R_NECK) 
  make_ins(W R_GIANT)  ditto(GIANT) 

  make_loc(R_GIANT "You are in the Giant Room.  The ceiling here is too high up for your\n\+
lamp to show it.  Cavernous passages lead east, north, and south.  On\n\+
the west wall is scrawled the inscription, \"FEE FIE FOE FOO\" [sic]." "You're in Giant Room.") 
  make_ins(S R_NARROW) 
  make_ins(E R_BLOCK) 
  make_ins(N R_IMMENSE) 

  make_loc(R_BLOCK "The passage here is blocked by a recent cave-in." null) 
  make_ins(S R_GIANT)  ditto(GIANT)  ditto(OUT) 

  make_loc(R_IMMENSE "You are at one end of an immense north/south passage." null) 
  make_ins(S R_GIANT)  ditto(GIANT)  ditto(PASSAGE) 
  make_cond_ins(N  unless_prop(RUSTY_DOOR  0)  R_FALLS)  ditto(ENTER)  ditto(CAVERN) 
  make_ins(N remark(9))

  make_loc(R_FALLS "You are in a magnificent cavern with a rushing stream, which cascades\n\+
over a sparkling waterfall into a roaring whirlpool that disappears\n\+
through a hole in the floor.  Passages exit to the south and west." "You're in cavern with waterfall.") 
  make_ins(S R_IMMENSE)  ditto(OUT) 
  make_ins(GIANT  R_GIANT) 
  make_ins(W R_INCLINE) 
    
  make_loc(R_INCLINE "You are at the top of a steep incline above a large room.  You could\n\+
climb down here, but you would not be able to climb up.  There is a\n\+
passage leading back to the north."  "You're at steep incline above large room.") 
  make_ins(N R_FALLS)  ditto(CAVERN)  ditto(PASSAGE) 
  make_ins(D R_LOW)  ditto(CLIMB) 
    
  make_loc(R_ABOVEP "You are in a secret N/S canyon above a sizable passage." null) 
  make_ins(N R_SJUNC) 
  make_ins(D R_BEDQUILT)  ditto(PASSAGE) 
  make_ins(S R_TITE) 
    
  make_loc(R_SJUNC "You are in a secret canyon at a junction of three canyons, bearing\n\+
north, south, and SE.  The north one is as tall as the other two\n\+
combined."  "You're at junction of three secret canyons.") 
    \ In Crowther's original, this was pretty much the edge of the cave. Going UP here
    \ would take you on a one-way trip back to the dusty rock room. Woods replaced
    \ that connection with a northerly passage to R_WINDOW.
  make_ins(SE  R_BEDQUILT) 
  make_ins(S R_ABOVEP) 
  make_ins(N R_WINDOW) 
    
  make_loc(R_TITE "A large stalactite extends from the roof and almost reaches the floor\n\+
below.  You could climb down it, and jump from it to the floor, but\n\+
having done so you would be unable to reach it to climb back up."  "You're at top of stalactite.") 
  make_ins(N R_ABOVEP) 
  make_cond_ins(D  40  R_LIKE6)  ditto(JUMP)  ditto(CLIMB) 
  make_cond_ins(D  50  R_LIKE9) 
  make_ins(D R_LIKE4) 

  make_loc(R_LOW "You are in a large low room.  Crawls lead north, SE, and SW." null) 
  make_ins(BEDQUILT  R_BEDQUILT) 
  make_ins(SW  R_SLOPING) 
  make_ins(N R_CRAWL) 
  make_ins(SE  R_ORIENTAL)  ditto(ORIENTAL) 

  make_loc(R_CRAWL "Dead end crawl." null) 
  make_ins(S R_LOW)  ditto(CRAWL)  ditto(OUT) 

  make_loc(R_WINDOW "You're at a low window overlooking a huge pit, which extends up out of\n\+
sight.  A floor is indistinctly visible over 50 feet below.  Traces of\n\+
white mist cover the floor of the pit, becoming thicker to the left.\n\+
Marks in the dust around the window would seem to indicate that\n\+
someone has been here recently.  Directly across the pit from you and\n\+
25 feet away there is a similar window looking into a lighted room.\n\+
A shadowy figure can be seen there peering back at you."  "You're at window on pit.") 
  make_ins(W R_SJUNC) 
  make_ins(JUMP  R_NECK) 

  make_loc(R_ORIENTAL "This is the Oriental Room.  Ancient oriental cave drawings cover the\n\+
walls.  A gently sloping passage leads upward to the north, another\n\+
passage leads SE, and a hands-and-knees crawl leads west." "You're in Oriental Room.") 
  make_ins(SE  R_SWISS) 
  make_ins(W R_LOW)  ditto(CRAWL) 
  make_ins(U R_MISTY)  ditto(N)  ditto(CAVERN) 

  make_loc(R_MISTY "You are following a wide path around the outer edge of a large cavern.\n\+
Far below, through a heavy white mist, strange splashing noises can be\n\+
heard.  The mist rises up through a fissure in the ceiling.  The path\n\+
exits to the south and west."  "You're in misty cavern.") 
  make_ins(S R_ORIENTAL)  ditto(ORIENTAL) 
  make_ins(W R_ALCOVE) 

  make_loc_hint(R_ALCOVE "You are in an alcove.  A small NW path seems to widen after a short\n\+
distance.  An extremely tight tunnel leads east.  It looks like a very\n\+
tight squeeze.  An eerie light can be seen at the other end."  "You're in alcove."  F_DARK_HINT) 
  make_ins(NW  R_MISTY)  ditto(CAVERN) 
  make_ins(E R_PPASS)  ditto(PASSAGE) 
  make_ins(E R_PLOVER)   \ never performed, but seen by "BACK"

  make_loc_hint(R_PLOVER "You're in a small chamber lit by an eerie green light.  An extremely\n\+
narrow tunnel exits to the west.  A dark corridor leads NE."  "You're in Plover Room."  F_DARK_HINT) 
  make_ins(W R_PPASS)  ditto(PASSAGE)  ditto(OUT) 
  make_ins(W R_ALCOVE)   \ never performed, but seen by "BACK"
  make_cond_ins(PLOVER  only_if_toting(EMERALD)  R_PDROP) 
  make_ins(PLOVER  R_Y2) 
  make_ins(NE  R_DARK)  ditto(DARK) 

  make_loc_hint(R_DARK "You're in the Dark-Room.  A corridor leading south is the only exit." \+
        "You're in Dark-Room."  F_DARK_HINT) 
  make_ins(S R_PLOVER)  ditto(PLOVER)  ditto(OUT) 

  make_loc(R_SLAB "You are in a large low circular chamber whose floor is an immense slab\n\+
fallen from the ceiling (Slab Room).  There once were large passages\n\+
to the east and west, but they are now filled with boulders.  Low\n\+
small passages go north and south, and the south one quickly bends\n\+
east around the boulders."  "You're in Slab Room.") 
  make_ins(S R_W2PIT) 
  make_ins(U R_ABOVER)  ditto(CLIMB) 
  make_ins(N R_BEDQUILT) 

  make_loc(R_ABOVER "You are in a secret N/S canyon above a large room." null) 
  make_ins(D R_SLAB)  ditto(SLAB) 
  make_cond_ins(S  unless_prop(DRAGON  0)  R_SCAN2) 
  make_ins(S R_SCAN1) 
  make_ins(N R_MIRROR) 
  make_ins(RESERVOIR  R_RES) 

  make_loc(R_MIRROR "You are in a north/south canyon about 25 feet across.  The floor is\n\+
covered by white mist seeping in from the north.  The walls extend\n\+
upward for well over 100 feet.  Suspended from some unseen point far\n\+
above you, an enormous two-sided mirror is hanging parallel to and\n\+
midway between the canyon walls.  (The mirror is obviously provided\n\+
for the use of the dwarves, who as you know are extremely vain.)\n\+
A small window can be seen in either wall, some fifty feet up."  "You're in mirror canyon.") 
  make_ins(S R_ABOVER) 
  make_ins(N R_RES)  ditto(RESERVOIR) 

  make_loc(R_RES "You are at the edge of a large underground reservoir.  An opaque cloud\n\+
of white mist fills the room and rises rapidly upward.  The lake is\n\+
fed by a stream, which tumbles out of a hole in the wall about 10 feet\n\+
overhead and splashes noisily into the water somewhere within the\n\+
mist.  The only passage goes back toward the south."  "You're at reservoir.") 
  make_ins(S R_MIRROR)  ditto(OUT) 

    \ R_SCAN1 and R_SCAN3 are the rooms the player sees when entering the
    \ secret canyon from the north and the east, respectively. The dragon
    \ blocks different exits in each room (and items dropped at one end of
    \ the canyon are not visible or accessible from the other end).
    \ Once the dragon has been vanquished, R_SCAN2 replaces both rooms.
  make_loc(R_SCAN1 "You are in a secret canyon that exits to the north and east." null) 
  make_ins(N R_ABOVER)  ditto(OUT) 
  make_ins(E remark(10)) ditto(FORWARD) 

  make_loc(R_SCAN2 "You are in a secret canyon that exits to the north and east." null) 
  make_ins(N R_ABOVER) 
  make_ins(E R_SECRET) 

  make_loc(R_SCAN3 "You are in a secret canyon that exits to the north and east." null) 
  make_ins(E R_SECRET)  ditto(OUT) 
  make_ins(N remark(10)) ditto(FORWARD) 

  make_loc(R_SECRET "You are in a secret canyon which here runs E/W.  It crosses over a\n\+
very tight canyon 15 feet below.  If you go down you may not be able\n\+
to get back up."  "You're in secret E/W canyon above tight canyon.") 
  make_ins(E R_HMK) 
  make_cond_ins(W  unless_prop(DRAGON  0)  R_SCAN2) 
  make_ins(W R_SCAN3) 
  make_ins(D R_WIDE) 

  make_loc(R_WIDE "You are at a wide place in a very tight N/S canyon." null) 
  make_ins(S R_TIGHT) 
  make_ins(N R_TALL) 

  make_loc(R_TIGHT "The canyon here becomes too tight to go further south." null) 
  make_ins(N R_WIDE)
    
  make_loc(R_TALL "You are in a tall E/W canyon.  A low tight crawl goes 3 feet north and\n\+
seems to open up." "You're in tall E/W canyon.") 
  make_ins(E R_WIDE) 
  make_ins(W R_BOULDERS) 
  make_ins(N R_SWISS)  ditto(CRAWL) 

  make_loc(R_BOULDERS "The canyon runs into a mass of boulders -- dead end." null) 
  make_ins(S R_TALL) 

  make_loc(R_SLOPING "You are in a long winding corridor sloping out of sight in both\n\+
directions."  "You're in sloping corridor.") 
  make_ins(D R_LOW) 
  make_ins(U R_SWSIDE) 

  make_loc(R_SWSIDE "You are on one side of a large, deep chasm.  A heavy white mist rising\n\+
up from below obscures all view of the far side.  A SW path leads away\n\+
from the chasm into a winding corridor."  "You're on SW side of chasm.") 
  make_ins(SW  R_SLOPING) 
  make_cond_ins(OVER  only_if_here(TROLL)  remark(11)) ditto(ACROSS)  ditto(CROSS)  ditto(NE) 
  make_cond_ins(OVER  unless_prop(CHASM  0)  remark(12))
  make_ins(OVER  R_TROLL) 
  make_cond_ins(JUMP  unless_prop(CHASM  0)  R_LOSE) 
  make_ins(JUMP  remark(2))

  make_loc(R_DEAD0  dead_end  null) 
  make_ins(S R_CROSS)  ditto(OUT) 

  make_loc_hint(R_DEAD1  dead_end  null F_TWIST_HINT) 
  make_ins(W R_LIKE11)  ditto(OUT) 

  make_loc(R_PIRATES_NEST  dead_end  null) 
  make_ins(SE  R_LIKE13) 

  make_loc_hint(R_DEAD3  dead_end  null F_TWIST_HINT) 
  make_ins(W R_LIKE4)  ditto(OUT) 

  make_loc_hint(R_DEAD4  dead_end  0  F_TWIST_HINT) 
  make_ins(E R_LIKE4)  ditto(OUT) 

  make_loc_hint(R_DEAD5  dead_end  0  F_TWIST_HINT) 
  make_ins(U R_LIKE3)  ditto(OUT) 

  make_loc_hint(R_DEAD6  dead_end  0  F_TWIST_HINT) 
  make_ins(W R_LIKE9)  ditto(OUT) 

  make_loc_hint(R_DEAD7  dead_end  0  F_TWIST_HINT) 
  make_ins(U R_LIKE10)  ditto(OUT) 

  make_loc(R_DEAD8  dead_end  0) 
  make_ins(E R_BRINK)  ditto(OUT) 

  make_loc_hint(R_DEAD9  dead_end  0  F_TWIST_HINT) 
  make_ins(S R_LIKE3)  ditto(OUT) 

  make_loc_hint(R_DEAD10  dead_end  0  F_TWIST_HINT) 
  make_ins(E R_LIKE12)  ditto(OUT) 

  make_loc_hint(R_DEAD11  dead_end  0  F_TWIST_HINT) 
  make_ins(U R_LIKE8)  ditto(OUT) 

  make_loc(R_NESIDE "You are on the far side of the chasm.  A NE path leads away from the\n\+
chasm on this side."  "You're on NE side of chasm.") 
  make_ins(NE  R_CORR) 
  make_cond_ins(OVER  only_if_here(TROLL)  remark(11)) ditto(ACROSS)  ditto(CROSS)  ditto(SW) 
  make_ins(OVER  R_TROLL) 
  make_ins(JUMP  remark(2))
  make_ins(FORK  R_FORK) 
  make_ins(VIEW  R_VIEW) 
  make_ins(BARREN  R_FBARR) 

  make_loc(R_CORR "You're in a long east/west corridor.  A faint rumbling noise can be\n\+
heard in the distance."  "You're in corridor.") 
  make_ins(W R_NESIDE) 
  make_ins(E R_FORK)  ditto(FORK) 
  make_ins(VIEW  R_VIEW) 
  make_ins(BARREN  R_FBARR) 

  make_loc(R_FORK "The path forks here.  The left fork leads northeast.  A dull rumbling\n\+
seems to get louder in that direction.  The right fork leads southeast\n\+
down a gentle slope.  The main corridor enters from the west."  "You're at fork in path.") 
  make_ins(W R_CORR) 
  make_ins(NE  R_WARM)  ditto(LEFT) 
  make_ins(SE  R_LIME)  ditto(RIGHT)  ditto(D) 
  make_ins(VIEW  R_VIEW) 
  make_ins(BARREN  R_FBARR) 

  make_loc(R_WARM "The walls are quite warm here.  From the north can be heard a steady\n\+
roar, so loud that the entire cave seems to be trembling.  Another\n\+
passage leads south, and a low crawl goes east."  "You're at junction with warm walls.") 
  make_ins(S R_FORK)  ditto(FORK) 
  make_ins(N R_VIEW)  ditto(VIEW) 
  make_ins(E R_CHAMBER)  ditto(CRAWL) 

  make_loc(R_VIEW "You are on the edge of a breath-taking view.  Far below you is an\n\+
active volcano, from which great gouts of molten lava come surging\n\+
out, cascading back down into the depths.  The glowing rock fills the\n\+
farthest reaches of the cavern with a blood-red glare, giving every-\n\+
thing an eerie, macabre appearance.  The air is filled with flickering\n\+
sparks of ash and a heavy smell of brimstone.  The walls are hot to\n\+
the touch, and the thundering of the volcano drowns out all other\n\+
sounds.  Embedded in the jagged roof far overhead are myriad twisted\n\+
formations, composed of pure white alabaster, which scatter the murky\n\+
light into sinister apparitions upon the walls.  To one side is a deep\n\+
 gorge, filled with a bizarre chaos of tortured rock that seems to have\n\+
 been crafted by the Devil himself.  An immense river of fire crashes\n\+
 out from the depths of the volcano, burns its way through the gorge,\n\+
 and plummets into a bottomless pit far off to your left.  To the\n\+
 right, an immense geyser of blistering steam erupts continuously\n\+
 from a barren island in the center of a sulfurous lake, which bubbles\n\+
 ominously.  The far right wall is aflame with an incandescence of its\n\+
 own, which lends an additional infernal splendor to the already\n\+
 hellish scene.  A dark, foreboding passage exits to the south."  "You're at breath-taking view.") 
  make_ins(S R_WARM)  ditto(PASSAGE)  ditto(OUT) 
  make_ins(FORK  R_FORK) 
  make_ins(D remark(13)) ditto(JUMP) 

  make_loc(R_CHAMBER "You are in a small chamber filled with large boulders.  The walls are\n\+
very warm, causing the air in the room to be almost stifling from the\n\+
heat.  The only exit is a crawl heading west, through which is coming\n\+
a low rumbling."  "You're in chamber of boulders.") 
  make_ins(W R_WARM)  ditto(OUT)  ditto(CRAWL) 
  make_ins(FORK  R_FORK) 
  make_ins(VIEW  R_VIEW) 

  make_loc(R_LIME "You are walking along a gently sloping north/south passage lined with\n\+
oddly shaped limestone formations."  "You're in limestone passage.") 
  make_ins(N R_FORK)  ditto(U)  ditto(FORK) 
  make_ins(S R_FBARR)  ditto(D)  ditto(BARREN) 
  make_ins(VIEW  R_VIEW) 

  make_loc(R_FBARR "You are standing at the entrance to a large, barren room.  A sign\n\+
posted above the entrance reads:  \"CAUTION!  BEAR IN ROOM!\""  "You're in front of barren room.") 
  make_ins(W R_LIME)  ditto(U) 
  make_ins(FORK  R_FORK) 
  make_ins(E R_BARR)  ditto(IN)  ditto(BARREN)  ditto(ENTER) 
  make_ins(VIEW  R_VIEW) 

  make_loc(R_BARR "You are inside a barren room.  The center of the room is completely\n\+
empty except for some dust.  Marks in the dust lead away toward the\n\+
far end of the room.  The only exit is the way you came in."  "You're in barren room.") 
  make_ins(W R_FBARR)  ditto(OUT) 
  make_ins(FORK  R_FORK) 
  make_ins(VIEW  R_VIEW) 

  \ The end-game repository.

  make_loc(R_NEEND "You are at the northeast end of an immense room, even larger than the\n\+
Giant Room.  It appears to be a repository for the \"Adventure\"\n\+
program.  Massive torches far overhead bathe the room with smoky\n\+
yellow light.  Scattered about you can be seen a pile of bottles (all\n\+
of them empty), a nursery of young beanstalks murmuring quietly, a bed\n\+
of oysters, a bundle of black rods with rusty stars on their ends, and\n\+
a collection of brass lanterns.  Off to one side a great many dwarves\n\+
are sleeping on the floor, snoring loudly.  A sign nearby reads: \"DO\n\+
NOT DISTURB THE DWARVES!\"  An immense mirror is hanging against one\n\+
wall, and stretches to the other end of the room, where various other\n\+
sundry objects can be glimpsed dimly in the distance." "You're at NE end.") 
  make_ins(SW  R_SWEND) 
  \ The following description has several minor differences from Woods' original.
  \ Woods' line breaks come after "A" on lines 4 and 5, "large" on line 6, and
  \ "vault" on line 7. Knuth's "that reads" corresponds to Woods' "which reads";
  \ presumably Knuth changed it to avoid ugly repetition, and I agree.
    
  make_loc(R_SWEND "You are at the southwest end of the repository.  To one side is a pit\n\+
full of fierce green snakes.  On the other side is a row of small\n\+
wicker cages, each of which contains a little sulking bird.  In one\n\+
corner is a bundle of black rods with rusty marks on their ends.\n\+
A large number of velvet pillows are scattered about on the floor.\n\+
A vast mirror stretches off to the northeast.  At your feet is a\n\+
large steel grate, next to which is a sign that reads, \"TREASURE\n\+
VAULT.  KEYS IN MAIN OFFICE.\""  "You're at SW end.") 
  make_ins(NE  R_NEEND) 
  make_ins(D remark(1))  \ You can't go through a locked steel grate!

  \ The following pseudo-locations have "forced" movement.
  \ In such cases we don't ask you for input; we assume that you have told
  \ us to force another instruction through. For example, if you try to
  \ JUMP across the fissure (R_EFISS), the instruction there sends you to
  \ R_LOSE, which prints the room description ("You didn't make it.") and
  \ immediately sends you to R_LIMBO, i.e., death.
  \ Crowther (and therefore Woods and therefore Knuth) implemented several
  \ responses as pseudo-locations; for example, "The dome is unclimbable"
  \ and "The crack is far too small for you to follow". For the ones that
  \ behave indistinguishably from remarks, I've converted them to remarks.

  make_loc(R_NECK "You are at the bottom of the pit with a broken neck." null) 
  make_ins(0  R_LIMBO) 
  make_loc(R_LOSE  "You didn't make it."  null) 
  make_ins(0  R_LIMBO) 
  make_loc(R_CLIMB  "You clamber up the plant and scurry through the hole at the top."  null) 
  make_ins(0  R_NARROW) 
    
  \ Typing CLIMB from the bottom of the west pit triggers a clever bit
  \ of gymnastics. We want to branch three ways on the state of the
  \ plant (too small to climb; "up the plant and out of the pit"; and
  \ all the way up the beanstalk). But the only operation available to
  \ us is "travel if objs(PLANT).prop is NOT x". So R_WPIT's instruction
  \ brings us to R_CHECK if objs(PLANT).prop != 2, and R_CHECK dispatches
  \ to one of the two non-narrow-corridor locations.
  make_loc(R_CHECK  null null) 
  make_cond_ins(0  unless_prop(PLANT  1)  R_UPNOUT) 
  make_ins(0  R_DIDIT) 
  make_loc(R_THRU "You have crawled through a very low wide passage parallel to and north\n\+
of the Hall of Mists."  null) 
  make_ins(0  R_WMIST) 
    
  make_loc(R_DUCK  game.places.get(R_THRU).long_desc.get  null) 
  make_ins(0  R_WFISS) 
    
  make_loc(R_UPNOUT "There is nothing here to climb.  Use \"up\" or \"out\" to leave the pit." null) 
  make_ins(0  R_WPIT) 
    
  make_loc(R_DIDIT  "You have climbed up the plant and out of the pit."  null) 
  make_ins(0  R_W2PIT) 

  \ The remaining "locations" R_PPASS, R_PDROP, and R_TROLL are special.
  __nextLoc game.start!(R_PPASS)
;

loaddone
