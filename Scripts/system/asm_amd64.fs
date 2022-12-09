//         *** Assembler for the Athlon64 ***           17jul04py

// Copyright (C) 1992-2000 by Bernd Paysan (486 assemlber)

// Copyright (C) 2000,2001,2003,2004,2007,2010 Free Software Foundation, Inc.

// This file is part of Gforth.

// Gforth is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS 0 do A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
// 
// The syntax is reverse polish. Source and destination are
// reversed. Size prefixes are used instead of AX/EAX. Example:
// Intel                           gives
// mov  ax,bx                      .w rbx rax mov
// mov  eax,[ebx]                  .d rbx ) rax mov
// add  eax,4                      .d 4 # rax add
// add  rax,8                      .q 8 # rax add
// 
// in .86 mode  .w is the default size, in .386 mode  .d is default
// in .amd64 mode .q is default size.
// .wa, .da, and .qa change address size. .b, .w(a) and .d(a) are not
// switches like in my assem68k, they are prefixes.
// [A-D][L|H] implicitely set the .b size. So
// AH AL mov
// generates a byte move. Sure you need .b for memory operations
// like .b ax ) inc    which is  inc  BYTE PTR [eAX]

// athlon64 Assembler Load Screen                       21apr00py

// base @ get-current ALSO ASSEMBLER DEFINITIONS also

// at this point, the search stack is: assembler assembler current (probably forth)
// this allows the top level of the stack to be overwritten by forth, so they
// can access forth words and assembler words, but swap the order at will

autoforget asm_amd64
: asm_amd64 ;

vocabulary assembler
also assembler definitions

// allow inline comments using (...) to allow easier porting from GForth
features
base @
kFFParenIsComment ->+ features
octal

// Assembler Forth words                                11mar00py

: case? 		// n1 n2 -- t / n1 f
  over = if
    drop true
  else
    false
  endif
;

// Stack-Buffer fr Extra-Werte                         22dec93py

long ModR/M
long ModR/M#
long SIB
long SIB#
long disp	    // displacement argument
long disp#      // sizeof displacement argument
long imm        // immediate argument
long imm#       // sizeof immediate argument
long Aimm?      // is immediate argument an address?
long Adisp?
long byte?      // is this a byte instruction?
long seg        // segment register
long .asize               long .anow
long .osize               long .onow
long .64size              long .64now
long .rex                 long .64bit
long .arel                long media
long dquad?

//: off 0 swap ! ;		: on -1 swap ! ;

: setOn -1 -> ;
: setOff 0 -> ;

: pre-
  setOff seg
  .asize -> .anow
  .osize -> .onow
  setOff .rex
  setOff .arel
  .64size -> .64now
  setOff media
  setOff dquad?
;

: sclear
  pre-
  setOff Aimm?
  setOff Adisp?
  setOff ModR/M#
  setOff SIB#
  setOff disp#
  setOff imm#
  setOff byte?
;

: .b
  1 -> byte?
  imm# 1 min -> imm#
;

: .w   setOff .onow   setOff .64now ;    : .wa  setOff .anow ;
: .d   setOn .onow   setOff .64now ;     : .da  setOn .anow ;
: .q   setOn .64now ;                    : .qa  setOff .anow ;
: -rex setOff .64now ;                   : -size  .osize -> .onow ;
: .dq  setOn dquad? ;

: _c,
  //"Assembled " %s dup 0xFF and %x " @ " %s here %x %nl
  c,
;

: +rel ;

// Extra-Werte compilieren                              01may95py
: bytes,  // nr x n --
  ?dup if
    0 do
      over 0< if  +rel swap 1+ swap  endif  dup _c,  0x8 rshift
    loop
  endif
  2drop
;

: rbytes, ( nr x n -- )
    >r here r@ + - r> bytes,
;
  
: opcode, // opcode -- 
  .asize .anow  <> if  0x67 _c,  endif
  media if
    media _c,
  else
    .osize .onow  <> if  0x66 _c,  endif
  endif
  seg     if  seg _c,  endif
  .64now  if  .rex 0x08 or -> .rex endif
  .rex    if  .rex 0xF and 0x40 or _c, endif
  _c,  pre-
;

: finish // opcode --
  opcode,
  ModR/M# if  ModR/M _c,  endif
  SIB#    if  SIB    _c,  endif
  Adisp?  disp disp# .arel if  rbytes,  else  bytes,  endif
  Aimm?   imm imm#  bytes,
  sclear
;

//
: finishb
  byte? xor  finish
;
: 0F,  0x0F opcode, ;

// Possible bug: '.64now off' does not have any effect _after_ 0F,
: finish0F // opcode --
  0F,  setOff .64now finish
;
: finishxx0f  // ( $xx0fyy -- )  \ xx 0f yy opcodes  (xx=66,f2,f3 or 00 if none)
   dup 0x10 rshift -> media  -size  0xff and finish0F
;


// Register                                             29mar94py

: regs  // mod n --
  1+ 0 do  dup constant 11+  loop  drop
;

: breg  // reg --
  builds c,
  does c@  .b
;

: bregs // mod n --
  1+ 0 do  dup breg     11+  loop  drop
;

: wreg  // reg --
  builds c,
  does c@  .w
;

: wregs // mod n --
  1+ 0 do  dup wreg     11+  loop  drop
;

: wadr: builds c,  does c@  .wa ;  // reg --
: wadrs 1+ 0 do  dup wadr:    11+  loop  drop ;  // mod n -- 

: xmmreg  // reg --
  builds ,
  does @  .w
;

: xmmregs // mod n --
  1+ 0 do  dup xmmreg     11+  loop  drop
;

: dadr builds c,  does c@  .da ;  // reg --
: dadrs 1+ 0 do  dup dadr    11+  loop  drop ; // mod n -- 

      0 7 wadrs [rbx+rsi] [rbx+rdi] [rbp+rsi] [rbp+rdi] [rsi] [rdi] [rbp] [rbx]
    300 7 regs rax rcx rdx rbx rsp rbp rsi rdi
0200300 7 regs r8 r9 r10 r11 r12 r13 r14 r15
    300 7 bregs al cl dl bl ah ch dh bh
2000300 3 bregs spl bpl sil dil
0200300 7 bregs r8l r9l r10l r11l r12l r13l r14l r15l
   2300 5 regs eseg cseg sseg dseg fseg gseg

: .386  setOff .64size setOff .64bit setOn .asize setOn .osize sclear ;

: .86   setOff .64size setOff .64bit setOff .asize setOff .osize sclear ;

: .amd64  .386  setOn .64size  setOff .asize setOn .64bit sclear ;  .amd64

: asize@  2 .anow if  2*  endif  .64bit if drop 4 endif ;

: osize@  2 .onow if  2*  endif ;


// Address modes                                        01may95py
: index  // breg1 ireg2 -- ireg
    dup 0x10000 and >r 370 and swap dup 0x10000 and >r 7 and or
    -> SIB 1 -> SIB# r> r> 2* or 44 or ;
: #] // disp -- reg
    .64bit if  -> disp   44 55 index 4 -> disp#  exit endif
    -> disp .anow if  55 4  else  66 2  endif  -> disp# ;
: r#]  // disp -- reg
    .64bit 0= if "RIP address only in 64 bit mode" error endif
    -> disp 4 -> disp# setOn .arel  55 ;
    
: *2   100 xor ;    : *4   200 xor ;    : *8   300 xor ;


: i] .anow .64bit or 0= if "No Index!" error endif  // reg1 reg2 -- ireg
  *8  index ;

: i#] rbp swap i] swap #] drop ;  // disp32 reg -- ireg 
: seg]  // seg disp -- -1
  -> disp  asize@ -> disp#  -> imm 2 -> imm# -1 ;
: ]  dup rsp = if dup i] else 200077 and endif ;  // reg -- reg
: d] ] >r dup -> disp  0x80 -0x80 within  // disp reg -- reg
  Adisp? or if  200 asize@  else  100 1  endif -> disp# r> or ;
: di] i] d] ;  // disp reg1 reg2 -- ireg
: A: setOn Adisp? ;
: A:: -2 -> Adisp? ;
: a#] A: #] ;
: aseg] A: seg] ;

// # A# rel] CR DR TR ST <ST STP                        01jan98py
: # // imm -- 
 dup -> imm  -0x80 0x80 within  byte? or
  if  1  else  osize@  endif  -> imm# ;
: L#  // imm -- 
  -> imm  osize@ -> imm# ;
: A#  // imm -- 
  setOn Aimm?  L# ;
: rel]  // addr -- -2 
  -> disp asize@ -> disp# -2 ;
: l] // disp reg -- reg 
 ] >r -> disp 200 asize@ -> disp# r> or ;
: li] // disp reg1 reg2 -- reg 
  i] l] ;

: >>mod // reg1 reg2 -- mod
  // bug?  This can only add $10 to .rex.  However only bits 0..3 of .rex are
  // used
  2dup or 0x80000 and 0x0F rshift ->+ .rex
  dup 0x10000 and 0x0E rshift ->+ .rex  70 and swap
  dup 0x30000 and 0x10 rshift ->+ .rex  307 and or ;
: >reg // reg -- reg'
  dup 0x10000 and 0x10 rshift ->+ .rex 7 and ;
: >mod // reg1 reg2 --
  >>mod -> ModR/M  1 -> ModR/M# ;


: CR   7 and 11 *  0x1C0 or ;  // n ---
0 CR constant CR0
: DR   7 and 11 *  0x2C0 or ;  // n ---
: TR   7 and 11 *  0x3C0 or ;  // n ---
: ST   7 and       0x5C0 or ;  // n ---
: <ST  7 and       0x7C0 or ;  // n ---
: STP  7 and       0x8C0 or ;  // n ---

// reg?                                                 10apr93py
: reg= 2 pick and = ; // reg flag mask -- flag 
: reg?  0xC0 0x0FFC0 reg= ; // reg -- reg flag
: ?reg reg? 0= if "reg expected!" error endif ; // reg -- reg flag
: ?mem  dup 0xC0 < 0= if "mem expected!" error endif ; // mem -- mem
: ?ax  dup rax <> if "ax/al expected!" error endif ;  // reg -- reg
: cr? 0x100 0xFF00 reg= ;  // reg -- reg flag
: dr? 0x200 0xFF00 reg= ;  // reg -- reg flag
: tr? 0x300 0xFF00 reg= ;  // reg -- reg flag
: sr? 0x400 0xFF00 reg= ;  // reg -- reg flag
: st? dup 0x8 rshift 5- ;  // reg -- reg flag
: ?st st? 0< if "st expected!" error endif ;  // reg -- reg
: xr? dup 0xFF > ;  // reg -- reg flag
: ?xr  xr? 0= if "xr expected!" error endif ;  // reg -- reg
: rel? dup -2 = ; // reg -- reg flag
: seg? dup -1 = ; // reg -- reg flag

// Single Byte instruction                              27mar94py

: bc:    builds c, does c@ _c,        ; // opcode --
: bc.b:  builds c, does c@ finishb  ; // opcode --
: bc0F:  builds c, does c@ finish0F ; // opcode --

: seg:   builds c, does c@ -> seg ; // opcode --

0x26 seg: eseg:    0x2E seg: cseg:    0x36 seg: sseg:    0x3E seg: dseg:
0x64 seg: fseg:    0x65 seg: gseg:

// arithmetics                                          07nov92py

: reg>mod // reg1 reg2 -- 1 / 3
  reg? if
    >mod 3
  else
    swap ?reg >mod 1
  endif
;
    
// n --
: ari:
  builds
    c,
  does 			// reg1 reg2 / reg -- 
    c@ >r imm#
    if
      imm# byte? + 1 > over rax = and
      if
        drop 0x05 r> 70 and or
      else
        r> >mod 0x81 imm# 1 byte? + =
        if 2+
        endif
      endif
    else
      reg>mod  r> 70 and or
    endif
    finishb
;

00 ari: add,     11 ari: or,      22 ari: adc,     33 ari: sbb,
44 ari: and,     55 ari: sub,     66 ari: xor,     77 ari: cmp,

// bit shifts    strings                                07nov92py

// n --
: shift:
  builds
    c,
  does			// r/m -- 
    c@ >mod  imm#
    if
        imm 1 =
        if
          0xD1 0
        else
          0xC1 1
        endif
        -> imm#
    else
      0xD3
    endif
    finishb
;

00 shift: rol,   11 shift: ror,   22 shift: rcl,   33 shift: rcr,
44 shift: shl,   55 shift: shr,   66 shift: sal,   77 shift: sar,

0x6D bc.b: ins,   0x6F bc.b: outs,
0xA5 bc.b: movs,  0xA7 bc.b: cmps,
0xAB bc.b: stos,  0xAD bc.b: lods,  0xAF bc.b: scas,

// movxr                                                07feb93py

: xr>mod  // reg1 reg2 -- 0 / 2
    xr?  if  >mod  2  else  swap ?xr >mod  0  endif  ;

: movxr  // reg1 reg2 --
    -rex 2dup or sr? nip
    if    xr>mod  0x8C or finish
    else  2dup or 0x8 rshift 1+ -3 and >r  xr>mod  r> 0x20 or or finish0F
    endif ;

// mov                                                  23jan93py

: assign#  byte? 0= if  osize@ -> imm#  else 1 -> imm# endif ;
: ?64off  .64bit .anow 0= and if  10 -> disp# endif 0 -> SIB# ;
: ?ofax // reg eax -- flag
  .64bit if  44  else  .anow if 55 else 66 endif endif rax
  rot = -rot = and  ;

// r/m reg / reg r/m / reg --
: mov,
  2dup or 0> imm# and
  if
    assign# reg?
    if
      >reg  0xB8 or byte? 3 lshift xor  setOff byte?
      .64now if 10 -> imm# endif
    else
      0 >mod  0xC7
    endif
  else
    2dup or 0xFFFF and 0xFF >
    if
      movxr exit
    endif
    2dup ?ofax
    if
      2drop 0xA1 ?64off
    else
      2dup swap  ?ofax
      if
        2drop 0xA3 ?64off
      else
        reg>mod 0x88 or
      endif
    endif
  endif
  finishb
;

// not neg mul imul div idiv                           29mar94py

: modf   -rot >mod finish   ;      // r/m reg opcode --
: modfb  -rot >mod finishb  ;      // r/m reg opcode --
: mod0F  -rot >mod finish0F ;      // r/m reg opcode --
: modxx0f -rot >mod finishxx0f ;  // r/m reg opcode --
: modf:  builds  c,  does  c@ modf ;
: not: // mode --
  builds c,
  does // r/m -- 
    c@ 0xF7 modfb ;

00 not: test#                   22 not: not,     33 not: neg,
44 not: mul,     55 not: pimul 66 not: div,     77 not: idiv,

: inc: // mode --
  builds c,
  does  // r/m -- 
    c@ >r reg?  byte? 0=  and .64bit 0= and
    if    107 and r> 70 and or finish
    else  r> 0xFF modfb   endif ;
00 inc: inc,     11 inc: dec,

// test shld shrd                                       07feb93py

: test,  // reg1 reg2 / reg --  
  imm#
  if
    assign#  rax case?
    if  0xA9  else  test#  exit  endif
  else
    ?reg >mod  0x85
  endif
  finishb
;

: shd // r/m reg opcode --
    imm# if  1 -> imm# 1-  endif  mod0F ;
: shld,  swap 245 shd ;          : shrd,  swap 255 shd ;

: btx: // r/m reg/# code --
  builds c,
  does c@ >r imm#
    if    1 -> imm#  r> 0xBA
    else  swap 203 r> >>mod  endif  mod0F ;
44 btx: bt,      55 btx: bts,     66 btx: btr,     77 btx: btc,

// push pop                                             05jun92py

: pushs   swap  fseg case?  if  0xA0 or finish0F exit  endif
                  gseg case?  if  0xA8 or finish0F exit  endif
    30 and 6 or or finish ;

: push,  // reg --
  -rex
  imm# 1 = if  0x6A finish exit  endif
  imm#     if  0x68 finish exit  endif
  reg?       if  >reg 0x50 or finish exit  endif
  sr?        if  0 pushs  exit  endif
  66 0xFF modf ;
: pop,   // reg --
  -rex
  reg?       if  >reg 0x58 or finish exit  endif
  sr?        if  1 pushs  exit  endif
  06 0x8F modf ;

// Ascii Arithmetics                                    22may93py

0x27 bc: daa,     0x2F bc: das,     0x37 bc: aaa,     0x3F bc: aas,

: aa:
  builds c,
  does // -- 
    c@ imm# 0= if  0x0A -> imm  endif  1 -> imm# finish ;
0xD4 aa: aam,     0xD5 aa: aad,     0xD6 bc: salc,    0xD7 bc: xlat,

0x60 bc: pusha,   0x61 bc: popa,
0x90 bc: nop,
0x98 bc: cbw,     0x99 bc: cwd,                      0x9B bc: fwait,
0x9C bc: pushf,   0x9D bc: popf,   0x9E bc: sahf,    0x9F bc: lahf,
                0xC9 bc: leave,
0xCC bc: int3,                     0xCE bc: into,    0xCF bc: iret,
//' fwait Alias wait

// one byte opcodes                                     25dec92py

0xF0 bc: lock,                      0xF2 bc: rep,     0xF3 bc: repe,
0xF4 bc: hlt,     0xF5 bc: cmc,
0xF8 bc: clc,     0xF9 bc: stc,     0xFA bc: cli,     0xFB bc: sti,
0xFC bc: cld,     0xFD bc: std,

: ?brange // offword --- offbyte
  dup 0x80 -0x80 within
  if "branch offset out of 1-byte range" %s endif ;
: sb: // opcode -- 
  builds c,
  does   // addr -- 
    >r  here 2+ - ?brange
    -> disp  1 -> disp#  r> c@ -rex finish ;
0xE0 sb: loopne,  0xE1 sb: loope,   0xE2 sb: loop,    0xE3 sb: jcxz,

// preceeding a ret, or retf, with "# N" will generate opcodes which will remove N bytes of arguments
// this will make 0xC3 and 0xCB opcodes become 0xC2 and 0xCA respectively
: pret // op --
  imm#  if  2 -> imm#  1-  endif  -rex finish ;
: ret,  0xC3  pret ;
: retf, 0xCB  pret ;

// call jmp                                             22dec93py

: call,			// reg / disp --
  rel? if
    drop 0xE8 disp here 1+ asize@ + - -> disp
    finish exit
  endif
  22 0xFF -rex modf
;

// the standard relative call:     SUBROUTINE_ADDR rcall,
: rcall, rel] call, ;

: callf,		// reg / seg --
  seg? if
    drop 0x9A
    finish exit
  endif
  33 0xFF -rex modf
;

: jmp,   		// reg / disp --
  -rex
  rel? if
    drop disp here 2+ - dup
    -0x80 0x80 within
    if
      -> disp 1 -> disp#  0xEB
    else
      3- -> disp 0xE9
    endif
    finish
    exit
  endif
  44 0xFF modf
;

: jmpf,		// reg / seg --
  seg? if
    drop 0xEA
    finish exit
  endif
  55 0xFF -rex modf
;

// : next .d ['] noop >code-address rel] jmp ;

// jump if                                              22dec93py

: cond: 0 do  i constant  loop ;

0x10 cond: vs, vc,   u<, u>=,  0=, 0<>,  u<=, u>,   0<, 0>=,  ps, pc,   <,  >=,   <=,  >,
0x10 cond: o,  no,   b,  nb,   z,  nz,    be,  nbe,  s,  ns,   pe, po,   l,  nl,   le,  nle,
: jmpIF  // addr cond --
  swap here 2+ - dup -0x80 0x80 within
  if            -> disp 0x70 1
  else  0F,  4- -> disp 0x80 4  endif  -> disp# or -rex finish ;
: jmp:  builds c,  does c@ jmpIF ;
: jmps  0 do  i jmp:  loop ;
0x10 jmps jo, jno, jb, jnb, jz, jnz, jbe, jnbe, js, jns, jpe, jpo, jl, jnl, jle, jnle,

// xchg                                                 22dec93py

: setIF // r/m cond -- 
  0 swap 0x90 or mod0F ;
: set: // cond -- 
  builds c,
  does  c@ setIF ;
// n -- 
: sets:   0 do  i set:  loop ;
0x10 sets: seto, setno, setb, setnb, sete, setne, setna, seta, sets, setns, setpe, setpo, setl, setge, setle, setg,
: xchg // r/m reg / reg r/m --
  over rax = if  swap  endif  reg?  0= if  swap  endif  ?reg
  byte? 0=  if rax case?
  if reg? if >reg 0x90 or finish exit endif  rax  endif endif
  0x87 modfb ;

: movx 0F, modfb ; // r/m reg opcode -- 
: movsx, 0xBF movx ; // r/m reg --
: movzx, 0xB7 movx ; // r/m reg --

// misc                                                 16nov97py

: enter, 2 -> imm# 0xC8 -rex finish c, ; // imm8 --
: arpl, swap 0x63 modf ; // reg r/m --
0x62 modf: BOUND // mem reg --

: mod0F:  builds c,  does c@ mod0F ;   // r/m reg -- 
0xBC mod0F: BSF
0xBD mod0F: BSR

0x06 bc0F: clts,
0x08 bc0F: invd,  0x09 bc0F: wbinvd,

: cmpxchg,  swap 0xA7 movx ; // reg r/m --
: cmpxchg8b,   0x8 0xC7 movx ; // r/m --
: bswap,       >reg 0xC8 or finish0F ; // reg --
: xadd,    0xC1 movx ; // r/m reg --

// misc                                                 20may93py

: imul, // r/m reg --
  imm# 0=
  if  dup rax =  if  drop pimul exit  endif
      0xAF mod0F exit  endif
  >mod imm# 1 = if  0x6B  else  0x69  endif  finish ;
: io  imm# if  1 -> imm#  else  0x8+  endif finishb ; // oc --
: in,  0xE5 io ;
: out, 0xE7 io ;
: int, 1 -> imm# 0xCD finish ;
: 0F.0: builds c, does c@ 0x00 -rex mod0F ; // r/m --
00 0F.0: sldt,   11 0F.0: str,    22 0F.0: lldt,   33 0F.0: ltr,
44 0F.0: verr,   55 0F.0: verw,
: 0F.1: builds c, does c@ 0x01 -rex mod0F ; // r/m --
00 0F.1: sgdt,   11 0F.1: sidt,   22 0F.1: lgdt,   33 0F.1: lidt,
44 0F.1: smsw,                    66 0F.1: lmsw,   77 0F.1: invlpg,

// misc                                                 29mar94py

0x02 mod0F: lar, // r/m reg -- )
0x8D modf:  lea, // m reg -- )
0xC4 modf:  les, // m reg -- )
0xC5 modf:  lds, // m reg -- )
0xB2 mod0F: lss, // m reg -- )
0xB4 mod0F: lfs, // m reg -- )
0xB5 mod0F: lgs, // m reg -- )
// Pentium/AMD K5 codes
: cpuid, 0F, 0xA2 _c, ;
: cmpchx8b, 0 0xC7 mod0F ; // m -- )
: rdtsc, 0F, 0x31 _c, ;
: rdmsr, 0F, 0x32 _c, ;
: wrmsr, 0F, 0x30 _c, ;
: rsm, 0F, 0xAA _c, ;

// Floating point instructions                          22dec93py

0xD8 bc: D8,   0xD9 bc: D9,   0xDA bc: DA,   0xDB bc: DB,
0xDC bc: DC,   0xDD bc: DD,   0xDE bc: DE,   0xDF bc: DF,

: D9: builds c, does D9, c@ finish ;

variable fsize
: .fs   0 fsize ! ;  : .fl   4 fsize ! ;  : .fx   3 fsize ! ;
: .fw   6 fsize ! ;  : .fd   2 fsize ! ;  : .fq   7 fsize ! ;
.fx
: fop:  builds c,  does  // fr/m -- )
    c@ >r
    st? dup 0< 0= if  swap r> >mod 2* 0xD8+ finish exit  endif
    drop ?mem r> >mod 0xD8 fsize @ dup 1 and dup 2* + - +
    finish ;
: f@!:
  builds c,
  does  // fm -- 
   c@ 0xD9 modf ;

// Floating point instructions                          08jun92py

0xD0 D9: fnop,

0xE0 D9: fchs,    0xE1 D9: fabs,
0xE4 D9: ftst,    0xE5 D9: fxam,
0xE8 D9: fld1,    0xE9 D9: fldl2t,  0xEA D9: fldl2e,  0xEB D9: fldpi,
0xEC D9: fldlg2,  0xED D9: fldln2,  0xEE D9: fldz,
0xF0 D9: f2xm1,   0xF1 D9: fyl2x,   0xF2 D9: fptan,   0xF3 D9: fpatan,
0xF4 D9: fxtract, 0xF5 D9: fprem1,  0xF6 D9: fdecstp, 0xF7 D9: fincstp,
0xF8 D9: fprem,   0xF9 D9: fyl2xp1, 0xFA D9: fsqrt,   0xFB D9: fsincos,
0xFC D9: frndint, 0xFD D9: fscale,  0xFE D9: fsin,    0xFF D9: fcos,

// Floating point instructions                          23jan94py

00 fop: fadd,    11 fop: fmul,    22 fop: fcom,    33 fop: fcomp,
44 fop: fsub,    55 fop: fsubr,   66 fop: fdiv,    77 fop: fdivr,

: fcompp, 1 STP fcomp, ;
: fbld,   44 0xD8 modf ; // fm -- )
: fbstp,  66 0xDF modf ; // fm -- )
: ffree,  00 0xDD modf ; // st -- )
: fsave,  66 0xDD modf ; // fm -- )
: frstor, 44 0xDD modf ; // fm -- )
: finit,  DB, 0xE3 _c, ; // -- )
: fxch,   11 0xD9 modf ; // st -- )

44 f@!: fldenv,  55 f@!: fldcw,   66 f@!: fstenv,  77 f@!: fstcw,

// fild fst fstsw fucom                                 22may93py
: fucom, ?st st? if 77 else 66 endif 0xDD modf ; // st -- )
: fucompp, DA, 0xE9 _c, ;
: fnclex,  DB, 0xE2 _c, ;
: fclex,   fwait, fnclex, ;
: fstsw, // r/m -- )
  dup rax = if  44  else  ?mem 77  endif  0xDF modf ;
: f@!,  fsize @ 1 and if  drop  else  nip  endif
    fsize @ 0xD9 or modf ;
: fx@!, // mem/st l x -- 
  rot  st? 0=
    if  swap 0xDD modf drop exit  endif  ?mem -rot
    fsize @ 3 = if drop 0xDB modf exit endif  f@!, ;
: fst,  // st/m -- 
  st?  0=
  if  22 0xDD modf exit  endif  ?mem 77 22 f@!, ;
: fld,  st? 0= if 0 0xD9 modf exit endif 55 0 fx@!, ; // st/m -- )
: fstp, 77 33 fx@!, ; // st/m -- )

// PPro instructions                                    28feb97py


: cmovIF 0x40 or mod0F ;  // r/m r flag -- )
: cmov:  builds c, does c@ cmovIF ;
: cmovs:  0 do  i cmov:  loop ;
0x10 cmovs: cmovo,  cmovno,   cmovb,   cmovnb,   cmovz,  cmovnz,  cmovbe,  cmovnbe,   cmovs,  cmovns,   cmovpe,  cmovpo,   cmovl,  cmovnl,   cmovle,  cmovnle,

// MMX/SSE2 opcodes                            02mar97py/14aug10dk

    300 7 regs    mm0   mm1   mm2   mm3   mm4   mm5   mm6   mm7
    300 7 xmmregs xmm0  xmm1  xmm2  xmm3  xmm4  xmm5  xmm6  xmm7
0200300 7 xmmregs xmm8  xmm9  xmm10 xmm11 xmm12 xmm13 xmm14 xmm15
: mmx  -rex  dquad?  0x660000 and or modxx0f ;
: mmx:  builds c,  does c@  mmx ;   // code -- 
: mmxs ?do  i mmx:  loop ;

0x64 0x60 mmxs punpcklbw, punpcklwd, punockldq, packusdw,
0x68 0x64 mmxs pcmpgtb, pcmpgtw, pcmpgtd, packsswb,
0x6C 0x68 mmxs punpckhbw, punpckhwd, punpckhdq, packssdw,
0x78 0x74 mmxs pcmpeqb, pcmpeqw, pcmpeqd, emms,
0xDA 0xD8 mmxs psubusb, psubusw,
0xEA 0xE8 mmxs psubsb, psubsw,
0xFB 0xF8 mmxs psubb, psubw, psubd,
0xDE 0xDC mmxs paddusb, paddusw,
0xEE 0xEC mmxs paddsb, paddsw,
0xFF 0xFC mmxs paddb, paddw, paddd,

// MMX/SSE2 opcodes                            02mar97py/14aug10dk

0xD5 mmx:   pmullw,               0xE5 mmx:   pmulhw,
0xF5 mmx:   pmaddwd,
0xDB mmx:   pand,                 0xDF mmx:   pandn,
0xEB mmx:   por,                  0xEF mmx:   pxor,
: pshift // mmx imm/m mod op --
  imm# if  1 -> imm#  else  + 0x50+  endif  mmx ;
: dqshift  // xmm imm mod op --
   dquad? imm# and 0= if "Usage  xmm<NN> imm # <opcode>" error exit endif
   pshift ;
: psrlw,   020 0x71 pshift ;  // mmx imm/m --
: psrld,   020 0x72 pshift ;  // mmx imm/m --
: psrlq,   020 0x73 pshift ;  // mmx imm/m --
: psraw,   040 0x71 pshift ;  // mmx imm/m --
: psrad,   040 0x72 pshift ;  // mmx imm/m --
: psllw,   060 0x71 pshift ;  // mmx imm/m --
: pslld,   060 0x72 pshift ;  // mmx imm/m --
: psllq,   060 0x73 pshift ;  // mmx imm/m --
: psrldq,  030 0x73 dqshift ; // xmm imm --   shifts by bytes, not bits!
: pslldq,  070 0x73 dqshift ; // xmm imm --

// MMX opcodes                                         27jun99beu

// mmxreg --> mmxreg move
// (dk)this misses the reg->mem move, redefined in the SSE part below
// 0x6F mod0F: movq,

// memory/reg32 --> mmxreg load
0x6F mmx: pldq,  // Intel: MOVQ mm,m64
0x6E mmx: pldd,  // Intel: MOVD mm,m32/r

// mmxreg --> memory/reg32
: pstq, swap  0x7F mod0F ; // mm m64   --   // Intel: MOVQ m64,mm
: pstd, swap  0x7E mod0F ; // mm m32/r --  // Intel: MOVD m32/r,mm

// 3Dnow! opcodes (K6)                                  21apr00py
: ib!  assembler:# 1 -> imm# ; // code --
: mod0F#  ib! mod0F ;   // code imm --
: 3Dnow:  builds c,  does -rex 0x0f swap c@ mod0F# ;   // imm --
0x0D 3Dnow: pi2fd,                0x1D 3Dnow: pf2id,
0x90 3Dnow: pfcmpge,              0xA0 3Dnow: pfcmpgt,
0x94 3Dnow: pfmin,                0xA4 3Dnow: pfmax,
0x96 3Dnow: pfrcp,                0xA6 3Dnow: pfrcpit1,
0x97 3Dnow: pfrsqrt,              0xA7 3Dnow: pfrsqit1,
0x9A 3Dnow: pfsub,                0xAA 3Dnow: pfsubr,
0x9E 3Dnow: pfadd,                0xAE 3Dnow: pfacc,
0xB0 3Dnow: pfcmpeq,              0xB4 3Dnow: pfmul,
0xB6 3Dnow: pfrcpit2,             0xB7 3Dnow: pmulhrw,
0xBF 3Dnow: pavgusb,

: femms,  -rex 0x0E finish0F ;
: prefetch,  -rex 000 0x0D mod0F ;    : prefetchw,  -rex 010 0x0D mod0F ;

// 3Dnow!+MMX/SSE2 opcodes (Athlon/Athlon64)   21apr00py/14aug10dk

0xF7 mmx:   maskmovq,             0xE7 mmx:   movntq,
0xE0 mmx:   pavgb,                0xE3 mmx:   pavgw,
0xC5 mmx:   pextrw,               0xC4 mmx:   pinsrw,
0xEE mmx:   pmaxsw,               0xDE mmx:   pmaxub,
0xEA mmx:   pminsw,               0xDA mmx:   pminub,
0xD7 mmx:   pmovmskb,             0xE4 mmx:   pmulhuw,
0xF6 mmx:   psadbw,               0x70 mmx:   pshufw,

0x0C 3Dnow: pi2fw,                0x1C 3Dnow: pf2iw,
0x8A 3Dnow: pfnacc,               0x8E 3Dnow: pfpnacc,
0xBB 3Dnow: pswabd,               : sfence,   .d 0xae 0xf8 ib! finish0F ;
: prefetchnta,  .d 000 0x18 mod0F ;  : prefetcht0,  .d 010 0x18 mod0F ;
: prefetcht1,   .d 020 0x18 mod0F ;  : prefetcht2,  .d 030 0x18 mod0F ;

// MMX/SSE/SSE2 moves                                     12aug10dk

: movxx  // mod opcode1-regdst opcode2-memdst rex&mmx? --
   >r >r >r
   reg>mod 3 = if r> r> drop else  r> r>  nip then
   r@ 1 and if dquad? 0x660000 and or then
   r> 2 and 0= if -rex then
   finishxx0f ;
   
: movxx:  // opcode1-regdst opcode2-memdst rex&mmx? --
  builds , 2,
  does  // xmm1 xmm2/mem | xmm1/mem xmm2   a-addr --
   dup @  swap cell+ 2@  rot  movxx ;

0x0f10   0x0f11   0 movxx: movups,   0x660f10 0x660f11 0 movxx: movupd,
0x0f12   0x0f13   0 movxx: movlps,   0x660f12 0x660f13 0 movxx: movlpd,
0x0f16   0x0f17   0 movxx: movhps,   0x660f16 0x660f17 0 movxx: movhpd,
0x0f28   0x0f29   0 movxx: movaps,   0x660f28 0x660f29 0 movxx: movapd,
0xf30f10 0xf30f11 0 movxx: movss,    0xf20f10 0xf20f11 0 movxx: movsd,   
0x660f6f 0x660f7f 0 movxx: movdqa,   0xf30f6f 0xf30f7f 0 movxx: movdqu,
0x0f6e   0x0f7e   3 movxx: movd,   // use .d/.q prefix to select operand size!

: maskmovdqu,  .dq maskmovq, ;
: movq,  ( xmm/mmx mmx/xmm/mem64 | mmx/xmm/mem64 xmm/mmx )
   dquad? if  0xf30f7e 0x660fd6
   else           0x0f6f   0x0f7f then  0 movxx ;

// SSE floating point arithmetic                          12aug10dk
: sse:  // opc1 rex? "name" --
  builds swap , c,
   does  // xmm1 xmm2/mem a-addr --
   dup @  swap cell+ c@ 0= if -rex then   modxx0f ;
: sses  // prefixN..prefix1 opc n "name1" ... "nameN"  --
   0 do   swap 0x10 lshift over or  0 sse:  loop  drop ;
: sse2xa 0x66 0x00 rot 2 sses ; // opc "name1" "name2-66"  --
: sse2xb 0xf2 0xf3 rot 2 sses ; // opc "name1-f3" "name2-f2"  --
: sse2xc 0x66 0xf2 rot 2 sses ; // opc "name1-f3" "name2-f2"  --
: sse4x  dup sse2xa sse2xb ;    // opc "name1" "name4"  --

0x0f58 sse4x addps, addpd, addss, addsd,
0x0f5c sse4x subps, subpd, subss, subsd, 
0x0f5f sse4x maxps, maxpd, maxss, maxsd,
0x0f5d sse4x minps, minpd, minss, minsd,
0x0f59 sse4x mulps, mulpd, mulss, mulsd,
0x0f5e sse4x divps, divpd, divss, divsd,
0x0f54 sse2xa andps, andpd,
0x0f55 sse2xa andnps, andnpd,
0x0f56 sse2xa orps, orpd,
0x0f57 sse2xa xorps, xorpd,
0x0f2e sse2xa ucomiss, ucomisd,
0x0f2f sse2xa comiss, comisd,
0x0f5b sse2xa cvtdq2ps, cvtps2dq,   0xf30f5b 0 sse: cvttps2dq,
0x0fe6 sse2xb cvtdq2pd, cvtpd2dq,   0x660f5b 0 sse: cvttpd2dq, 
0x0f2d sse2xa cvtps2pi, cvtpd2pi,   
0x0f2a sse2xa cvtpi2ps, cvtpi2pd,   
// these take .d/.q size prefix:
0xf30f2d 1 sse: cvtss2si,
0xf20f2d 1 sse: cvtsd2si,
0xf20f2a 1 sse: cvtsi2sd,
0xf30f2a 1 sse: cvtsi2ss,

// 0x0f5e sse2xa divps divpd \ already defined above
0x0f7c sse2xc haddps, haddpd,
0x0f7d sse2xc hsubps, hsubpd,
0x0fd0 sse2xc addsubps, addsubpd,

: cmp: // opc #cmp "name" --
   builds swap , c,
   does  // xmm1/mem xmm2 a-addr --
   dup @ swap cell+ c@ ib! -rex modxx0f ;
: cmps:  0x8 0 do  dup i cmp: loop  drop ;    // opc "name1" ... "name8" --
  0x0fc2 cmps: cmpeqps, cmpltps, cmpleps, cmpunordps, cmpneqps, cmpnltps, cmpnleps, cmpordps,
0x660fc2 cmps: cmpeqpd, cmpltpd, cmplepd, cmpunordpd, cmpneqpd, cmpnltpd, cmpnlepd, cmpordpd,
0xf30fc2 cmps: cmpeqss, cmpltss, cmpless, cmpunordss, cmpneqss, cmpnltss, cmpnless, cmpordss,
0xf20fc2 cmps: cmpeqsd, cmpltsd, cmplesd, cmpunordsd, cmpneqsd, cmpnltsd, cmpnlesd, cmpordsd,

// Assembler Conditionals                               22dec93py
// cond -- ~cond
: ~cond         1 xor ;
// start dest --- offbyte
: >offset       swap  2+ -  ?brange ;
// cond -- here 
: if,           here dup 2+ rot  ~cond  jmpIF ;
: endif,        dup here >offset swap 1+ c! ;
alias then, endif,
: _ahead        here dup 2+ rel] jmp, ;
: else,         _ahead swap endif, ;
: begin,        here ;
: do,           here ;
: while,        if, swap ;
: again,        rel] jmp, ;
: repeat,       again, endif, ;
: until,        ~cond jmpIF ;
: ?do,          here dup 2+ dup jcxz, ;
: but,          swap ;
: yet,          dup ;
: makeflag      ~cond al swap setIF  1 # rax and,  rax dec, ;

alias rip rsi
alias rnext rdi
alias roptab r9
alias rnumops r10
alias racttab r11
alias rcore r12
alias rfp r13
alias rpsp r14
alias rrp r15

// wrap the existing definition of "code" with op which pushes assembler vocab on search stack
also forth definitions
: code
  code
  also assembler		// push assembler vocab on top of search stack
  sclear
;

// subroutines must have a "endcode" after its return instruction to pop assembler stack
: subroutine
  create
  also assembler		// push assembler vocab on top of search stack
  sclear
;

previous definitions

// exit from a forthop defined by "code"
: next,
  rnext jmp,
  previous				// pop assembler vocab off search stack
;

// use endcode to end subroutines and "code" forthops which don't end with "next,"
: endcode
  previous				// pop assembler vocab off search stack
;


0 #if
// inline assembly support

code _inlineAsm
  esi eax mov,
  eax ] esi mov,
  4 # eax add,
  eax jmp,
endcode

int _asmPatch

: asm[
  "_inlineAsm @ " %s here %x %nl
  compile _inlineAsm
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
#endif

enum: eForthCore
  0x00 _optypeAction
  0x08 _numBuiltinOps
  0x10 _ops
  0x18 _numOps
  0x20 _maxOps
  0x28 _pEngine
  0x30 _IP
  0x38 _SP
  0x40 _RP
  0x48 _FP
  0x50 _TPM
  0x50 _varMode
  0x60 _state
  0x68 _error
  0x70 _SB
  0x78 _ST
  0x80 _SLen
  0x88 _RB
  0x90 _RT
  0x98 _RLen
  0xa0 _pThread
  0xa8 _pDictionary
  0xb0 _pFileFuncs
  0xb8 _innerLoop
  0xc0 _innerExecute
  0xc8 _consoleOutStream
  0xd0 _base
  0xd8 _signedPrintMode
  0xe0 _traceFlags
  0xe8 _pExceptionFrame
  0xf0 _scratch
;enum

base !
-> features

previous definitions



loaddone



The 386 assembler included in Gforth was written by Bernd Paysan, it's
available under GPL, and originally part of bigFORTH.

   The 386 disassembler included in Gforth was written by Andrew McKewan
and is in the public domain.

   The disassembler displays code in an Intel-like prefix syntax.

   The assembler uses a postfix syntax with reversed parameters.

   The assembler includes all instruction of the Athlon, i.e. 486 core
instructions, Pentium and PPro extensions, floating point, MMX, 3Dnow!,
but not ISSE. It's an integrated 16- and 32-bit assembler. Default is 32
bit, you can switch to 16 bit with .86 and back to 32 bit with .386.

   There are several prefixes to switch between different operation
sizes, `.b' for byte accesses, `.w' for word accesses, `.d' for
double-word accesses. Addressing modes can be switched with `.wa' for
16 bit addresses, and `.da' for 32 bit addresses. You don't need a
prefix for byte register names (`AL' et al).

   For floating point operations, the prefixes are `.fs' (IEEE single),
`.fl' (IEEE double), `.fx' (extended), `.fw' (word), `.fd'
(double-word), and `.fq' (quad-word).

   The MMX opcodes don't have size prefixes, they are spelled out like
in the Intel assembler. Instead of move from and to memory, there are
PLDQ/PLDD and PSTQ/PSTD.

  Immediate values are indicated by postfixing them with `#', e.g.,
`3 #'.  Here are some examples of addressing modes in various syntaxes:

 Gforth             Intel (NASM)     AT&T (gas)      Name
  .w eax             ax               %ax             register (16 bit)
  eax                eax              %eax            register (32 bit)
  3 #                offset 3         $3              immediate
  1000 #]            byte ptr 1000    1000            displacement
  ebx ]              [ebx]            (%ebx)          base
  100 edi d]         100[edi]         100(%edi)       base+displacement
  20 eax *4 i#]      20[eax*4]        20(,%eax,4)     (index*scale)+displacement
  edi eax *4 i]      [edi][eax*4]     (%edi,%eax,4)   base+(index*scale)
  4 ebx ecx di]      4[ebx][ecx]      4(%ebx,%ecx)    base+index+displacement
  12 esp eax *2 di]  12[esp][eax*2]   12(%esp,%eax,2) base+(index*scale)+displacement

   You can use `L)' and `LI)' instead of `D)' and `DI)' to enforce
32-bit displacement fields (useful for later patching).

   Some example of instructions are:

     eax ebx mov,             \ move ebx,eax
     3 # ax mov,              \ mov eax,3
     100 edi d] ax mov,       \ mov eax,100[edi]
     4 ebx ecx di] ax mov,    \ mov eax,4[ebx][ecx]
     .w eax ebx mov,          \ mov bx,ax

   The following forms are supported for binary instructions:

     <reg> <reg> <inst>
     <n> # <reg> <inst>
     <mem> <reg> <inst>
     <reg> <mem> <inst>
     <n> # <mem> <inst>

   The shift/rotate syntax is:

     <reg/mem> 1 # shl, \ shortens to shift without immediate
     <reg/mem> 4 # shl,
     <reg/mem> cl shl,

   Precede string instructions (`movs' etc.) with `.b' to get the byte
version.

   The control structure words `IF' `UNTIL' etc. must be preceded by
one of these conditions: `vs vc u< u>= 0= 0<> u<= u> 0< 0>= ps pc < >=
<= >'. (Note that most of these words shadow some Forth words when
`assembler' is in front of `forth' in the search path, e.g., in `code'
words).  Currently the control structure words use one stack item, so
you have to use `roll' instead of `cs-roll' to shuffle them (you can
also use `swap' etc.).

   Here is an example of a `code' word (assumes that the stack pointer
is in esi and the TOS is in ebx):

     code my+ ( n1 n2 -- n )
         4 esi D] ebx add,
         4 # esi add,
         Next
     end-code

	0x80 # ebx mov,
	0x80 # .w ebx mov,
	0x80 # .w esi mov,
	0x80 # si mov,
	0x80 # di mov,
	0x80 # bp mov,
	0x80 # sp mov,
	0x80 # .d ebx mov,
	0x80 # .b ebx mov,
	// 
	[ebx+esi] eax mov,
	[ebx+edi] eax mov,
	[ebp+esi] eax mov,
	[ebp+edi]  eax mov,
	[esi] eax mov,
	[edi] eax mov,
	[ebp] eax mov,
	[ebx] eax mov,

jump instructions from http://unixwiz.net/techtips/x86-jumps.html

Instruction  Jump if ...           Signed?   Flags
-----------  -----------           --------  -----
JO           overflow                        OF=1
JNO          not overflow                    OF=0
JS           sign                            SF=1
JNS          not sign                        SF=0
JE/JZ        equal
             zero                            ZF=1
JNE/JNZ      not-equal
             not-zero                        ZF=0
JB/JNAE/JC   below
             not-above-or-equal
             carry                 unsigned  CF=1
JNB/JAE/JNC  not-below
             above-or-equal
             no-carry              unsigned  CF=0
JBE/JNA      below-or-equal
             not-above             unsigned  CF=1 or ZF=1
JA/JNBE      above
             not-below-or-equal    unsigned  CF=0 and ZF=0
JL/JNGE      less
             not-greater-or-equal  signed    SF<>OF
JGE/JNL      greater-or-equal
             not-less              signed    SF=OF
JLE/JNG      less-or-equal
             not-greater           signed    ZF=1 or SF<>OF
JG/JNLE      greater
             not-less-or-equal     signed    ZF=0 and SF=OF
JP/JPE       parity
             parity-even                     PF=1
JNP/JPO      not-parity
             parity-odd                      PF=0
JCXZ/JECXZ   CX register is zero
             ECX register is zero
	