//	ax		free
//	bx		free
//	cx		IP
//	dx		SP
//	si		free
//	di		inner interp PC (constant)
//	bp		core ptr (constant)
//	sp		unused

requires asm_pentium
autoforget _inlineAsm
also assembler


//
// inline assembly stuff will be put in asm_pentium when done testing
//

code _inlineAsm
  esi eax mov,
  eax ] esi mov,
  4 # eax add,
  eax jmp,
endcode

int _asmPatch

: asm[
  "_inlineAsm @ " %s here %x %nl
  lit _inlineAsm ,
  here -> _asmPatch
  0 ,
  also assembler
  0 state !
;

precedence asm[

: ]asm
  next,
  "patching " %s _asmPatch %x " with " %s here %x %nl
  here _asmPatch !
  1 state !
  previous
;

: goo
  1 3 5
  "Before\n" %s ds
  asm[
    7 # eax mov,
    4 # edx sub,
    eax edx ] mov,
  ]asm
  "After\n" %s ds
  9 13 15
;

previous
loaddone


asm[ does:

  compile _inlineAsm opcode
  push DP on rstack
  compile 0 (will be filled in by ]ASM)
  push assembler vocab on top of search stack
  set state to interpret

]asm does:

  compile jmp DI instruction
  round DP up to nearest longword
  pop address off rstack, store DP there
  set state to compile
  pop search stack

_inlineAsm

  on entry, IP points to longword with IP after the inline code
  set inlineAddress = IP + 4
  set IP = @IP
  non-turbo version will have to save & setup registers with SP, IP, inner interp re-entry point
  jump to inlineAddress

