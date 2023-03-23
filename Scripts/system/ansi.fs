
: pre-ansi ;

requires assembler
requires forth_internals
requires forth_optype
requires extops
requires double
requires compatability
requires numberio

"Type 'regularMode' to exit ANSI Forth compatability mode\n" %s

: ansiMode kFFAnsi to features ;
: regularMode kFFRegular to features ;
ansiMode

autoforget ansi

: ansi ;

