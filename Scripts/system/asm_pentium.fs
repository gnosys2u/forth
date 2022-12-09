//         *** Assembler for the Intel i486 ***         07nov92py

// Copyright (C) 1992-2000 by Bernd Paysan

// Copyright (C) 2000 Free Software Foundation, Inc.

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
// mov  ax,bx                      .w bx ax mov
// mov  eax,[ebx]                  .d bx ) ax mov
// add  eax,4                      .d 4 # ax add
// 
// in .86 mode  .w is the default size, in .386 mode  .d is default
// .wa and .da change address size. .b, .w(a) and .d(a) are not
// switches like in my assem68k, they are prefixes.
// [A-D][L|H] implicitely set the .b size. So
// AH AL mov
// generates a byte move. Sure you need .b for memory operations
// like .b ax ) inc    which is  inc  BYTE PTR [eAX]

// 80486 Assembler Load Screen                          21apr00py

// base @ get-current ALSO ASSEMBLER DEFINITIONS also

// at this point, the search stack is: assembler assembler current (probably forth)
// this allows the top level of the stack to be overwritten by forth, so they
// can access forth words and assembler words, but swap the order at will

autoforget ASM_PENTIUM
: ASM_PENTIUM ;

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

int ModR/M               int ModR/M#
int SIB                  int SIB#
int disp	// displacement argument
int disp#	// sizeof displacement argument
int imm		// immediate argument
int imm#	// sizeof immediate argument
int Aimm?	// is immediate argument an address?
int Adisp?
int byte?	// is this a byte instruction?
int seg		// segment register
int .asize               int .anow
int .osize               int .onow


//: off 0 swap ! ;		: on -1 swap ! ;

: setOn -1 -> ;
: setOff 0 -> ;

: pre-
  setOff seg
  .asize -> .anow
  .osize -> .onow
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

: .w   setOff .onow ;
: .wa  setOff .anow ;

: .d   setOn .onow ;
: .da  setOn .anow ;


: _c,
  //"Assembled " %s dup 0xFF and %x " @ " %s here %x %nl
  c,
;

// Extra-Werte compilieren                              01may95py
: bytes,  // nr x n --
  ?dup if
    0 do
      over 0< if  swap 1+ swap  endif  dup _c,  0x8 rshift
    loop
  endif
  2drop
;

: opcode, // opcode -- 
  .asize .anow  <> if  0x67 _c,  endif
  .osize .onow  <> if  0x66 _c,  endif
  seg     if  seg _c,  endif
  _c,  pre-
;

: finish // opcode --
  opcode,
  ModR/M# if  ModR/M _c,  endif
  SIB#    if  SIB    _c,  endif
  Adisp?  disp disp# bytes,
  Aimm?   imm imm#  bytes,
  sclear
;

//
: finishb
  byte? xor  finish
;
: 0F,  0x0F opcode, ;

: finish0F // opcode --
  0F,  finish
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

: wadr builds c,  does c@  .wa ;  // reg --
: wadrs 1+ 0 do  dup wadr    11+  loop  drop ;  // mod n -- 

: dadr builds c,  does c@  .da ;  // reg --
: dadrs 1+ 0 do  dup dadr    11+  loop  drop ; // mod n -- 

   0 7 dadrs [ebx+esi] [ebx+edi] [ebp+esi] [ebp+edi] [esi] [edi] [ebp] [ebx]
   0 7 wadrs [bx+si] [bx+di] [bp+si] [bp+di] [si] [di] [bp] [bx]
 300 7 regs  eax ecx edx ebx esp ebp esi edi
 300 7 wregs ax cx dx bx sp bp si di
 300 7 bregs al cl dl bl ah ch dh bh
2300 5 regs eseg cseg sseg dseg fseg gseg

: .386  setOn .asize setOn .osize sclear ;  .386

: .86   setOff .asize setOff .osize sclear ;

: asize@  2 .anow if  2*  endif ;

: osize@  2 .onow if  2*  endif ;


// Address modes                                        01may95py
: #] // disp -- reg
  -> disp .anow if  55 4  else  66 2  endif  -> disp# ;
: *2   100 xor ;    : *4   200 xor ;    : *8   300 xor ;
: index 370 and swap 7 and or ;  // reg1 reg2 -- modr/m
: i] .anow 0= if "No Index!" error endif  // reg1 reg2 -- ireg
  *8  index  -> SIB 1 -> SIB# 44 ;
: i#] ebp swap i] swap #] drop ;  // disp32 reg -- ireg 
: seg]  // seg disp -- -1
  -> disp  asize@ -> disp#  -> imm 2 -> imm# -1 ;
: ]  dup esp = if dup i] else 77 and endif ;  // reg -- reg
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
: >>mod  // reg1 reg2 -- mod
  70 and swap 307 and or ;
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
: reg?  0xC0 -0x40 reg= ; // reg -- reg flag
: ?reg reg? 0= if "reg expected!" error endif ; // reg -- reg flag
: ?mem  dup 0xC0 < 0= if "mem expected!" error endif ; // mem -- mem
: ?ax  dup eax <> if "ax/al expected!" error endif ;  // reg -- reg
: cr? 0x100 -0x100 reg= ;  // reg -- reg flag
: dr? 0x200 -0x100 reg= ;  // reg -- reg flag
: tr? 0x300 -0x100 reg= ;  // reg -- reg flag
: sr? 0x400 -0x100 reg= ;  // reg -- reg flag
: st? dup 0x8 rshift 5 - ;  // reg -- reg flag
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

//################ ALERT!
//  forth uses

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
      imm# byte? + 1 > over eax = and
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
    2dup or sr? nip
    if    xr>mod  0x8C
    else  2dup or 0x8 rshift 1+ -3 and >r  xr>mod  0F,  r> 0x20 or
    endif  or  finish ;

// mov                                                  23jan93py

: assign#  byte? 0= if  osize@ -> imm#  else 1 -> imm# endif ;

: ?ofax // reg eax -- flag
  .anow if 55 else 66 endif eax d= ;

// r/m reg / reg r/m / reg --
: mov,
  // 2dup or 0> imm# and
  imm#
  if
    assign# reg?
    if
      7 and  0xB8 or byte? 3 lshift xor  setOff byte?
    else
      0 >mod  0xC7
    endif
  else
    2dup or 0xFF >
    if
      movxr exit
    endif
    2dup ?ofax
    if
      2drop 0xA1
    else
      2dup swap  ?ofax
      if
        2drop 0xA3
      else
        reg>mod 0x88 or
      endif
    endif
  endif
  finishb
;

// not neg mul imul div idiv                           29mar94py

: modf   -rot >mod finish   ; // r/m reg opcode --
: modfb  -rot >mod finishb  ; // r/m reg opcode --
: mod0F  -rot >mod finish0F ; // r/m reg opcode --
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
    c@ >r reg?  byte? 0=  and
    if    107 and r> 70 and or finish
    else  r> 0xFF modfb   endif ;
00 inc: inc,     11 inc: dec,

// test shld shrd                                       07feb93py

: test,  // reg1 reg2 / reg --  
  imm#
  if
    assign#  eax case?
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
  imm# 1 = if  0x6A finish exit  endif
  imm#     if  0x68 finish exit  endif
  reg?       if  7 and 0x50 or finish exit  endif
  sr?        if  0 pushs  exit  endif
  66 0xFF modf ;
: pop,   // reg --
  reg?       if  7 and 0x58 or finish exit  endif
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
    >r  here 2 + - ?brange
    -> disp  1 -> disp#  r> c@ finish ;
0xE0 sb: loopne,  0xE1 sb: loope,   0xE2 sb: loop,    0xE3 sb: jcxz,

// preceeding a ret, or retf, with "# N" will generate opcodes which will remove N bytes of arguments
// this will make 0xC3 and 0xCB opcodes become 0xC2 and 0xCA respectively
: pret // op --
  imm#  if  2 -> imm#  1-  endif  finish ;
: ret,  0xC3  pret ;
: retf, 0xCB  pret ;

// call jmp                                             22dec93py

: call,			// reg / disp --
  rel? if
    drop 0xE8 disp here 1+ asize@ + - -> disp
    finish exit
  endif
  22 0xFF modf
;

// the standard relative call:     SUBROUTINE_ADDR rcall,
: rcall, rel] call, ;

: callf,		// reg / seg --
  seg? if
    drop 0x9A
    finish exit
  endif
  33 0xFF modf
;

: jmp,   		// reg / disp --
  rel? if
    drop disp here 2 + - dup
    -0x80 0x80 within
    if
      -> disp 1 -> disp#  0xEB
    else
      3 - -> disp 0xE9
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
  55 0xFF modf
;




// : next ['] noop >code-address rel] jmp ;

// jump if                                              22dec93py

: cond: 0 do  i constant  loop ;

0x10 cond: vs, vc,   u<, u>=,  0=, 0<>,  u<=, u>,   0<, 0>=,  ps, pc,   <,  >=,   <=,  >,
0x10 cond: o,  no,   b,  nb,   z,  nz,    be,  nbe,  s,  ns,   pe, po,   l,  nl,   le,  nle,
: jmpIF  // addr cond --
  swap here 2 + - dup -0x80 0x80 within
  if            -> disp 0x70 1
  else  0F,  4 - -> disp 0x80 4  endif  -> disp# or finish ;
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
  over eax = if  swap  endif  reg?  0= if  swap  endif  ?reg
  byte? 0=  if eax case?
  if reg? if 7 and 0x90 or finish exit endif  eax  endif endif
  0x87 modfb ;

: movx 0F, modfb ; // r/m reg opcode -- 
: movsx, 0xBF movx ; // r/m reg --
: movzx, 0xB7 movx ; // r/m reg --

// misc                                                 16nov97py

: enter, 2 -> imm# 0xC8 finish c, ; // imm8 --
: arpl, swap 0x63 modf ; // reg r/m --
0x62 modf: BOUND // mem reg --

: mod0F:  builds c,  does c@ mod0F ;   // r/m reg -- 
0xBC mod0F: BSF
0xBD mod0F: BSR

0x06 bc0F: clts,
0x08 bc0F: invd,  0x09 bc0F: wbinvd,

: cmpxchg,  swap 0xA7 movx ; // reg r/m --
: cmpxchg8b,   0x8 0xC7 movx ; // r/m --
: bswap,       7 and 0xC8 or finish0F ; // reg --
: xadd,    0xC1 movx ; // r/m reg --

// misc                                                 20may93py

: imul, // r/m reg --
  imm# 0=
  if  dup eax =  if  drop pimul exit  endif
      0xAF mod0F exit  endif
  >mod imm# 1 = if  0x6B  else  0x69  endif  finish ;
: io  imm# if  1 -> imm#  else  0x8 +  endif finishb ; // oc --
: in,  0xE5 io ;
: out, 0xE7 io ;
: int, 1 -> imm# 0xCD finish ;
: 0F.0: builds c, does c@ 0x00 mod0F ; // r/m --
00 0F.0: sldt,   11 0F.0: str,    22 0F.0: lldt,   33 0F.0: ltr,
44 0F.0: verr,   55 0F.0: verw,
: 0F.1: builds c, does c@ 0x01 mod0F ; // r/m --
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
    st? dup 0< 0= if  swap r> >mod 2* 0xD8 + finish exit  endif
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
  dup eax = if  44  else  ?mem 77  endif  0xDF modf ;
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

// MMX opcodes                                          02mar97py

300 7 regs mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7

: mmxs
  dup if
    do i mod0F:  loop
  endif
;
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

// MMX opcodes                                          02mar97py

0xD5 mod0F: pmullw,               0xE5 mod0F: pmulhw,
0xF5 mod0F: pmaddwd,
0xDB mod0F: pand,                 0xDF mod0F: pandn,
0xEB mod0F: por,                  0xEF mod0F: pxor,
: pshift // mmx imm/m mod op --
  imm# if  1 -> imm#  else  + 0x50 +  endif  mod0F ;
: psrlw,   020 0x71 pshift ;  // mmx imm/m --
: psrld,   020 0x72 pshift ;  // mmx imm/m --
: psrlq,   020 0x73 pshift ;  // mmx imm/m --
: psraw,   040 0x71 pshift ;  // mmx imm/m --
: psrad,   040 0x72 pshift ;  // mmx imm/m --
: psllw,   060 0x71 pshift ;  // mmx imm/m --
: pslld,   060 0x72 pshift ;  // mmx imm/m --
: psllq,   060 0x73 pshift ;  // mmx imm/m --

// MMX opcodes                                         27jun99beu

// mmxreg --> mmxreg move
0x6F mod0F: movq,

// memory/reg32 --> mmxreg load
0x6F mod0F: pldq,  // Intel: MOVQ mm,m64
0x6E mod0F: pldd,  // Intel: MOVD mm,m32/r

// mmxreg --> memory/reg32
: pstq, swap  0x7F mod0F ; // mm m64   --   // Intel: MOVQ m64,mm
: pstd, swap  0x7E mod0F ; // mm m32/r --  // Intel: MOVD m32/r,mm

// 3Dnow! opcodes (K6)                                  21apr00py
: mod0F#  # 1 -> imm mod0F ;   // code imm --
: 3Dnow:  builds c,  does c@ mod0F# ;   // imm --
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

: femms,  0x0E finish0F ;
: prefetch,  000 0x0D mod0F ;    : prefetchw,  010 0x0D mod0F ;

// 3Dnow!+MMX opcodes (Athlon)                          21apr00py

0xF7 mod0F: maskmovq,             0xE7 mod0F: movntq,
0xE0 mod0F: pavgb,                0xE3 mod0F: pavgw,
0xC5 mod0F: pextrw,               0xC4 mod0F: pinsrw,
0xEE mod0F: pmaxsw,               0xDE mod0F: pmaxub,
0xEA mod0F: pminsw,               0xDA mod0F: pminub,
0xD7 mod0F: pmovmskb,             0xE4 mod0F: pmulhuw,
0xF6 mod0F: psadbw,               0x70 mod0F: pshufw,

0x0C 3Dnow: pi2fw,                0x1C 3Dnow: pf2iw,
0x8A 3Dnow: pfnacc,               0x8E 3Dnow: pfpnacc,
0xBB 3Dnow: pswabd,               : sfence,   0xAE 0x07 mod0F# ;
: prefetchnta,  000 0x18 mod0F ;  : prefetcht0,  010 0x18 mod0F ;
: prefetcht1,   020 0x18 mod0F ;  : prefetcht2,  030 0x18 mod0F ;

// Assembler Conditionals                               22dec93py
// cond -- ~cond
: ~cond         1 xor ;
// start dest --- offbyte
: >offset       swap  2 + -  ?brange ;
// cond -- here 
: if,           here dup 2 + rot  ~cond  jmpIF ;
: endif,        dup here >offset swap 1+ c! ;
alias then, endif,
: _ahead        here dup 2 + rel] jmp, ;
: else,         _ahead swap endif, ;
: begin,        here ;
: do,           here ;
: while,        if, swap ;
: again,        rel] jmp, ;
: repeat,       again, endif, ;
: until,        ~cond jmpIF ;
: ?do,          here dup 2 + dup jcxz, ;
: but,          swap ;
: yet,          dup ;
: makeflag      ~cond al swap setIF  1 # eax and,  eax dec, ;

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
  edi jmp,
  previous				// pop assembler vocab off search stack
;

// use endcode to end subroutines and "code" forthops which don't end with "next,"
: endcode
  previous				// pop assembler vocab off search stack
;


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


enum: eForthCore
  0x00 _optypeAction
  0x04 _numBuiltinOps
  0x08 _ops
  0x0C _numOps
  0x10 _maxOps
  0x14 _pEngine
  0x18 _IP
  0x1C _SP
  0x20 _RP
  0x24 _FP
  0x28 _TPM
  0x2C _TPD
  0x30 _varMode
  0x34 _state
  0x38 _error
  0x3C _SB
  0x40 _ST
  0x44 _SLen
  0x48 _RB
  0x4C _RT
  0x50 _RLen
  0x54 _pThread
  0x58 _pDictionary
  0x5C _pFileFuncs
  0x60 _innerLoop
  0x64 _innerExecute
  0x68 _consoleOutStream
  0x70 _base
  0x74 _signedPrintMode
  0x78 _traceFlags
  0x7C _scratch
;enum

// pop IP off rstack using eax
: rpop,
  _RP ebp d] eax mov,
  eax ] esi mov,
  4 # eax add,
  eax _RP ebp d] mov,
;

// push IP onto rstack using eax
: rpush,
  _RP ebp d] eax mov,
  4 # eax sub,
  esi eax ] mov,
  eax _RP ebp d] mov,
;

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
	