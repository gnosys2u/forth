
: pre-ansi ;

requires assembler
requires forth_internals
requires extops
requires double
requires compatability
requires numberio
requires floating

." Type 'setNonAnsiMode' to exit ANSI Forth compatability mode\n"

setAnsiMode

autoforget ansi

: ansi ;

