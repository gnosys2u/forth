
: pre-ansi ;

requires assembler
requires forth_internals
requires forth_optype
requires extops
requires compatability
requires numberio

"Type 'kFFRegular to features' to exit ANSI Forth compatability mode\n" %s

kFFAnsi to features

autoforget ansi

: ansi ;

