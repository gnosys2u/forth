
: hints ;

//========== Hints. =======================================================
// This section corresponds to sections 80 and 193--196 in Knuth.

: addHint
  new Hint -> Hint newHint
  new String -> newHint.prompt
  new String -> newHint.hint
  newHint.hint.set
  newHint.prompt.set
  -> newHint.cost
  -> newHint.thresh
  game.addHint(newHint)
  oclear newHint
;

: build_hints
  // hints - threshold cost prompt hint
  0  5  "Welcome to Adventure!!  Would you like instructions? "
  "Somewhere nearby is Colossal Cave  where others have found fortunes in\n\+
treasure and gold  though it is rumored that some who enter are never\n\+
seen again.  Magic is said to work in the cave.  I will be your eyes\n\+
and hands.  Direct me with commands of 1 or 2 words.  I should warn\n\+
you that I look at only the first five letters of each word  so you'll\n\+
have to enter \"NORTHEAST\" as \"NE\" to distinguish it from \"NORTH\".\n\+
(Should you get stuck  type \"HELP\" for some general hints.  For information\n\+
on how to end your adventure  etc.  type \"INFO\".)\n\+
                             -  -  -\n \+
The first Adventure program was developed by Willie Crowther.\n\+
Most of the features of the current program were added by Don Woods.\n\+
This particular program was translated from Fortran to CWEB by\n\+
Don Knuth  and then from CWEB to ANSI C by Arthur O'Dwyer."  addHint

  0  10  "Hmmm  this looks like a clue  which means it'll cost you 10 points to \+
read it.  Should I go ahead and read it anyway? "
"It says  \"There is something strange about this place  such that one \+
of the words I've always known now has a new effect.\""  addHint

  4  2  "Are you trying to get into the cave? "
"The grate is very solid and has a hardened steel lock.  You cannot \+
enter without a key  and there are no keys in sight.  I would recommend \+
looking elsewhere for the keys."  addHint

  5  2  "Are you trying to catch the bird? "
"Something seems to be frightening the bird just now and you cannot \+
catch it no matter what you try. Perhaps you might try later."  addHint

8  2  "Are you trying to deal somehow with the snake? "
"You can't kill the snake  or drive it away  or avoid it  or anything \+
like that.  There is a way to get by  but you don't have the necessary \+
resources right now."  addHint

  75  4  "Do you need help getting out of the maze? "
"You can make the passages look less alike by dropping things." addHint

  25  5  "Are you trying to explore beyond the Plover Room? "
"There is a way to explore that region without having to worry about \+
falling into a pit.  None of the objects available is immediately \+
useful in discovering the secret."  addHint

  20  3  "Do you need help getting out of here? "
"Don't go west." addHint

;

loaddone
