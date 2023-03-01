\         *** Assembler for the Intel i486 ***         07nov92py

\ Copyright (C) 1992-2000 by Bernd Paysan

\ Copyright (C) 2000 Free Software Foundation, Inc.

\ This file is part of Gforth.

\ Gforth is free software; you can redistribute it and/or
\ modify it under the terms of the GNU General Public License
\ as published by the Free Software Foundation; either version 2
\ of the License, or (at your option) any later version.

\ This program is distributed in the hope that it will be useful,
\ but WITHOUT ANY WARRANTY; without even the implied warranty of
\ MERCHANTABILITY or FITNESS 0 do A PARTICULAR PURPOSE.  See the
\ GNU General Public License for more details.

\ You should have received a copy of the GNU General Public License
\ along with this program; if not, write to the Free Software
\ Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
\ 
\ The syntax is reverse polish. Source and destination are
\ reversed. Size prefixes are used instead of AX/EAX. Example:
\ Intel                           gives
\ mov  ax,bx                      .w bx ax mov
\ mov  eax,[ebx]                  .d bx ) ax mov
\ add  eax,4                      .d 4 # ax add
\ 
\ in .86 mode  .w is the default size, in .386 mode  .d is default
\ .wa and .da change address size. .b, .w(a) and .d(a) are not
\ switches like in my assem68k, they are prefixes.
\ [A-D][L|H] implicitely set the .b size. So
\ AH AL mov
\ generates a byte move. Sure you need .b for memory operations
\ like .b ax ) inc    which is  inc  BYTE PTR [eAX]

\ 80486 Assembler Load Screen                          21apr00py

\ base @ get-current ALSO ASSEMBLER DEFINITIONS also

\ at this point, the search stack is: assembler assembler current (probably forth)
\ this allows the top level of the stack to be overwritten by forth, so they
\ can access forth words and assembler words, but swap the order at will

autoforget ASM_PENTIUM
: ASM_PENTIUM ;

vocabulary assembler
also assembler definitions

\ allow inline comments using (...) to allow easier porting from GForth
features
base @
kFFParenIsComment ->+ features
octal

\ Assembler Forth words                                11mar00py

: case? 		\ n1 n2 -- t / n1 f
  over = if
    drop true
  else
    false
  endif
;

\ Stack-Buffer fr Extra-Werte                         22dec93py

int ModR/M               int ModR/M#
int SIB                  int SIB#
int disp	\ displacement argument
int disp#	\ sizeof displacement argument
int imm		\ immediate argument
int imm#	\ sizeof immediate argument
int Aimm?	\ is immediate argument an address?
int Adisp?
int byte?	\ is this a byte instruction?
int seg		\ segment register
int .asize               int .anow
int .osize               int .onow


\ : off 0 swap ! ;		: on -1 swap ! ;

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
  \ "Assembled " %s dup $FF and %x " @ " %s here %x %nl
  c,
;

\ Extra-Werte compilieren                              01may95py
: bytes,  \ nr x n --
  ?dup if
    0 do
      over 0< if  swap 1+ swap  endif  dup _c,  $8 rshift
    loop
  endif
  2drop
;

: opcode, \ opcode -- 
  .asize .anow  <> if  $67 _c,  endif
  .osize .onow  <> if  $66 _c,  endif
  seg     if  seg _c,  endif
  _c,  pre-
;

: finish \ opcode --
  opcode,
  ModR/M# if  ModR/M _c,  endif
  SIB#    if  SIB    _c,  endif
  Adisp?  disp disp# bytes,
  Aimm?   imm imm#  bytes,
  sclear
;

\ 
: finishb
  byte? xor  finish
;
: 0F,  $0F opcode, ;

: finish0F \ opcode --
  0F,  finish
;

\ Register                                             29mar94py

: regs  \ mod n --
  1+ 0 do  dup constant 11+  loop  drop
;

: breg  \ reg --
  builds c,
  does c@  .b
;

: bregs \ mod n --
  1+ 0 do  dup breg     11+  loop  drop
;

: wreg  \ reg --
  builds c,
  does c@  .w
;

: wregs \ mod n --
  1+ 0 do  dup wreg     11+  loop  drop
;

: wadr builds c,  does c@  .wa ;  \ reg --
: wadrs 1+ 0 do  dup wadr    11+  loop  drop ;  \ mod n -- 

: dadr builds c,  does c@  .da ;  \ reg --
: dadrs 1+ 0 do  dup dadr    11+  loop  drop ; \ mod n -- 

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


\ Address modes                                        01may95py
: #] \ disp -- reg
  -> disp .anow if  55 4  else  66 2  endif  -> disp# ;
: *2   100 xor ;    : *4   200 xor ;    : *8   300 xor ;
: index 370 and swap 7 and or ;  \ reg1 reg2 -- modr/m
: i] .anow 0= if "No Index!" error endif  \ reg1 reg2 -- ireg
  *8  index  -> SIB 1 -> SIB# 44 ;
: i#] ebp swap i] swap #] drop ;  \ disp32 reg -- ireg 
: seg]  \ seg disp -- -1
  -> disp  asize@ -> disp#  -> imm 2 -> imm# -1 ;
: ]  dup esp = if dup i] else 77 and endif ;  \ reg -- reg
: d] ] >r dup -> disp  $80 -$80 within  \ disp reg -- reg
  Adisp? or if  200 asize@  else  100 1  endif -> disp# r> or ;
: di] i] d] ;  \ disp reg1 reg2 -- ireg
: A: setOn Adisp? ;
: A:: -2 -> Adisp? ;
: a#] A: #] ;
: aseg] A: seg] ;

\ # A# rel] CR DR TR ST <ST STP                        01jan98py
: # \ imm -- 
 dup -> imm  -$80 $80 within  byte? or
  if  1  else  osize@  endif  -> imm# ;
: L#  \ imm -- 
  -> imm  osize@ -> imm# ;
: A#  \ imm -- 
  setOn Aimm?  L# ;
: rel]  \ addr -- -2 
  -> disp asize@ -> disp# -2 ;
: l] \ disp reg -- reg 
 ] >r -> disp 200 asize@ -> disp# r> or ;
: li] \ disp reg1 reg2 -- reg 
  i] l] ;
: >>mod  \ reg1 reg2 -- mod
  70 and swap 307 and or ;
: >mod \ reg1 reg2 --
  >>mod -> ModR/M  1 -> ModR/M# ;


: CR   7 and 11 *  $1C0 or ;  \ n ---
0 CR constant CR0
: DR   7 and 11 *  $2C0 or ;  \ n ---
: TR   7 and 11 *  $3C0 or ;  \ n ---
: ST   7 and       $5C0 or ;  \ n ---
: <ST  7 and       $7C0 or ;  \ n ---
: STP  7 and       $8C0 or ;  \ n ---

\ reg?                                                 10apr93py
: reg= 2 pick and = ; \ reg flag mask -- flag 
: reg?  $C0 -$40 reg= ; \ reg -- reg flag
: ?reg reg? 0= if "reg expected!" error endif ; \ reg -- reg flag
: ?mem  dup $C0 < 0= if "mem expected!" error endif ; \ mem -- mem
: ?ax  dup eax <> if "ax/al expected!" error endif ;  \ reg -- reg
: cr? $100 -$100 reg= ;  \ reg -- reg flag
: dr? $200 -$100 reg= ;  \ reg -- reg flag
: tr? $300 -$100 reg= ;  \ reg -- reg flag
: sr? $400 -$100 reg= ;  \ reg -- reg flag
: st? dup $8 rshift 5 - ;  \ reg -- reg flag
: ?st st? 0< if "st expected!" error endif ;  \ reg -- reg
: xr? dup $FF > ;  \ reg -- reg flag
: ?xr  xr? 0= if "xr expected!" error endif ;  \ reg -- reg
: rel? dup -2 = ; \ reg -- reg flag
: seg? dup -1 = ; \ reg -- reg flag

\ Single Byte instruction                              27mar94py

: bc:    builds c, does c@ _c,        ; \ opcode --
: bc.b:  builds c, does c@ finishb  ; \ opcode --
: bc0F:  builds c, does c@ finish0F ; \ opcode --

: seg:   builds c, does c@ -> seg ; \ opcode --

$26 seg: eseg:    $2E seg: cseg:    $36 seg: sseg:    $3E seg: dseg:
$64 seg: fseg:    $65 seg: gseg:

\ ################ ALERT!
\  forth uses

\ arithmetics                                          07nov92py

: reg>mod \ reg1 reg2 -- 1 / 3
  reg? if
    >mod 3
  else
    swap ?reg >mod 1
  endif
;
    
\ n --
: ari:
  builds
    c,
  does 			\ reg1 reg2 / reg -- 
    c@ >r imm#
    if
      imm# byte? + 1 > over eax = and
      if
        drop $05 r> 70 and or
      else
        r> >mod $81 imm# 1 byte? + =
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

\ bit shifts    strings                                07nov92py

\ n --
: shift:
  builds
    c,
  does			\ r/m -- 
    c@ >mod  imm#
    if
        imm 1 =
        if
          $D1 0
        else
          $C1 1
        endif
        -> imm#
    else
      $D3
    endif
    finishb
;

00 shift: rol,   11 shift: ror,   22 shift: rcl,   33 shift: rcr,
44 shift: shl,   55 shift: shr,   66 shift: sal,   77 shift: sar,

$6D bc.b: ins,   $6F bc.b: outs,
$A5 bc.b: movs,  $A7 bc.b: cmps,
$AB bc.b: stos,  $AD bc.b: lods,  $AF bc.b: scas,

\ movxr                                                07feb93py

: xr>mod  \ reg1 reg2 -- 0 / 2
    xr?  if  >mod  2  else  swap ?xr >mod  0  endif  ;

: movxr  \ reg1 reg2 --
    2dup or sr? nip
    if    xr>mod  $8C
    else  2dup or $8 rshift 1+ -3 and >r  xr>mod  0F,  r> $20 or
    endif  or  finish ;

\ mov                                                  23jan93py

: assign#  byte? 0= if  osize@ -> imm#  else 1 -> imm# endif ;

: ?ofax \ reg eax -- flag
  .anow if 55 else 66 endif eax d= ;

\ r/m reg / reg r/m / reg --
: mov,
  \ 2dup or 0> imm# and
  imm#
  if
    assign# reg?
    if
      7 and  $B8 or byte? 3 lshift xor  setOff byte?
    else
      0 >mod  $C7
    endif
  else
    2dup or $FF >
    if
      movxr exit
    endif
    2dup ?ofax
    if
      2drop $A1
    else
      2dup swap  ?ofax
      if
        2drop $A3
      else
        reg>mod $88 or
      endif
    endif
  endif
  finishb
;

\ not neg mul imul div idiv                           29mar94py

: modf   -rot >mod finish   ; \ r/m reg opcode --
: modfb  -rot >mod finishb  ; \ r/m reg opcode --
: mod0F  -rot >mod finish0F ; \ r/m reg opcode --
: modf:  builds  c,  does  c@ modf ;
: not: \ mode --
  builds c,
  does \ r/m -- 
    c@ $F7 modfb ;

00 not: test#                   22 not: not,     33 not: neg,
44 not: mul,     55 not: pimul 66 not: div,     77 not: idiv,

: inc: \ mode --
  builds c,
  does  \ r/m -- 
    c@ >r reg?  byte? 0=  and
    if    107 and r> 70 and or finish
    else  r> $FF modfb   endif ;
00 inc: inc,     11 inc: dec,

\ test shld shrd                                       07feb93py

: test,  \ reg1 reg2 / reg --  
  imm#
  if
    assign#  eax case?
    if  $A9  else  test#  exit  endif
  else
    ?reg >mod  $85
  endif
  finishb
;

: shd \ r/m reg opcode --
    imm# if  1 -> imm# 1-  endif  mod0F ;
: shld,  swap 245 shd ;          : shrd,  swap 255 shd ;

: btx: \ r/m reg/# code --
  builds c,
  does c@ >r imm#
    if    1 -> imm#  r> $BA
    else  swap 203 r> >>mod  endif  mod0F ;
44 btx: bt,      55 btx: bts,     66 btx: btr,     77 btx: btc,

\ push pop                                             05jun92py

: pushs   swap  fseg case?  if  $A0 or finish0F exit  endif
                  gseg case?  if  $A8 or finish0F exit  endif
    30 and 6 or or finish ;

: push,  \ reg --
  imm# 1 = if  $6A finish exit  endif
  imm#     if  $68 finish exit  endif
  reg?       if  7 and $50 or finish exit  endif
  sr?        if  0 pushs  exit  endif
  66 $FF modf ;
: pop,   \ reg --
  reg?       if  7 and $58 or finish exit  endif
  sr?        if  1 pushs  exit  endif
  06 $8F modf ;

\ Ascii Arithmetics                                    22may93py

$27 bc: daa,     $2F bc: das,     $37 bc: aaa,     $3F bc: aas,

: aa:
  builds c,
  does \ -- 
    c@ imm# 0= if  $0A -> imm  endif  1 -> imm# finish ;
$D4 aa: aam,     $D5 aa: aad,     $D6 bc: salc,    $D7 bc: xlat,

$60 bc: pusha,   $61 bc: popa,
$90 bc: nop,
$98 bc: cbw,     $99 bc: cwd,                      $9B bc: fwait,
$9C bc: pushf,   $9D bc: popf,   $9E bc: sahf,    $9F bc: lahf,
                $C9 bc: leave,
$CC bc: int3,                     $CE bc: into,    $CF bc: iret,
\ ' fwait Alias wait

\ one byte opcodes                                     25dec92py

$F0 bc: lock,                      $F2 bc: rep,     $F3 bc: repe,
$F4 bc: hlt,     $F5 bc: cmc,
$F8 bc: clc,     $F9 bc: stc,     $FA bc: cli,     $FB bc: sti,
$FC bc: cld,     $FD bc: std,

: ?brange \ offword --- offbyte
  dup $80 -$80 within
  if "branch offset out of 1-byte range" %s endif ;
: sb: \ opcode -- 
  builds c,
  does   \ addr -- 
    >r  here 2 + - ?brange
    -> disp  1 -> disp#  r> c@ finish ;
$E0 sb: loopne,  $E1 sb: loope,   $E2 sb: loop,    $E3 sb: jcxz,

\ preceeding a ret, or retf, with "# N" will generate opcodes which will remove N bytes of arguments
\ this will make $C3 and $CB opcodes become $C2 and $CA respectively
: pret \ op --
  imm#  if  2 -> imm#  1-  endif  finish ;
: ret,  $C3  pret ;
: retf, $CB  pret ;

\ call jmp                                             22dec93py

: call,			\ reg / disp --
  rel? if
    drop $E8 disp here 1+ asize@ + - -> disp
    finish exit
  endif
  22 $FF modf
;

\ the standard relative call:     SUBROUTINE_ADDR rcall,
: rcall, rel] call, ;

: callf,		\ reg / seg --
  seg? if
    drop $9A
    finish exit
  endif
  33 $FF modf
;

: jmp,   		\ reg / disp --
  rel? if
    drop disp here 2 + - dup
    -$80 $80 within
    if
      -> disp 1 -> disp#  $EB
    else
      3 - -> disp $E9
    endif
    finish
    exit
  endif
  44 $FF modf
;

: jmpf,		\ reg / seg --
  seg? if
    drop $EA
    finish exit
  endif
  55 $FF modf
;




\ : next ['] noop >code-address rel] jmp ;

\ jump if                                              22dec93py

: cond: 0 do  i constant  loop ;

$10 cond: vs, vc,   u<, u>=,  0=, 0<>,  u<=, u>,   0<, 0>=,  ps, pc,   <,  >=,   <=,  >,
$10 cond: o,  no,   b,  nb,   z,  nz,    be,  nbe,  s,  ns,   pe, po,   l,  nl,   le,  nle,
: jmpIF  \ addr cond --
  swap here 2 + - dup -$80 $80 within
  if            -> disp $70 1
  else  0F,  4 - -> disp $80 4  endif  -> disp# or finish ;
: jmp:  builds c,  does c@ jmpIF ;
: jmps  0 do  i jmp:  loop ;
$10 jmps jo, jno, jb, jnb, jz, jnz, jbe, jnbe, js, jns, jpe, jpo, jl, jnl, jle, jnle,

\ xchg                                                 22dec93py

: setIF \ r/m cond -- 
  0 swap $90 or mod0F ;
: set: \ cond -- 
  builds c,
  does  c@ setIF ;
\ n -- 
: sets:   0 do  i set:  loop ;
$10 sets: seto, setno, setb, setnb, sete, setne, setna, seta, sets, setns, setpe, setpo, setl, setge, setle, setg,
: xchg \ r/m reg / reg r/m --
  over eax = if  swap  endif  reg?  0= if  swap  endif  ?reg
  byte? 0=  if eax case?
  if reg? if 7 and $90 or finish exit endif  eax  endif endif
  $87 modfb ;

: movx 0F, modfb ; \ r/m reg opcode -- 
: movsx, $BF movx ; \ r/m reg --
: movzx, $B7 movx ; \ r/m reg --

\ misc                                                 16nov97py

: enter, 2 -> imm# $C8 finish c, ; \ imm8 --
: arpl, swap $63 modf ; \ reg r/m --
$62 modf: BOUND \ mem reg --

: mod0F:  builds c,  does c@ mod0F ;   \ r/m reg -- 
$BC mod0F: BSF
$BD mod0F: BSR

$06 bc0F: clts,
$08 bc0F: invd,  $09 bc0F: wbinvd,

: cmpxchg,  swap $A7 movx ; \ reg r/m --
: cmpxchg8b,   $8 $C7 movx ; \ r/m --
: bswap,       7 and $C8 or finish0F ; \ reg --
: xadd,    $C1 movx ; \ r/m reg --

\ misc                                                 20may93py

: imul, \ r/m reg --
  imm# 0=
  if  dup eax =  if  drop pimul exit  endif
      $AF mod0F exit  endif
  >mod imm# 1 = if  $6B  else  $69  endif  finish ;
: io  imm# if  1 -> imm#  else  $8 +  endif finishb ; \ oc --
: in,  $E5 io ;
: out, $E7 io ;
: int, 1 -> imm# $CD finish ;
: 0F.0: builds c, does c@ $00 mod0F ; \ r/m --
00 0F.0: sldt,   11 0F.0: str,    22 0F.0: lldt,   33 0F.0: ltr,
44 0F.0: verr,   55 0F.0: verw,
: 0F.1: builds c, does c@ $01 mod0F ; \ r/m --
00 0F.1: sgdt,   11 0F.1: sidt,   22 0F.1: lgdt,   33 0F.1: lidt,
44 0F.1: smsw,                    66 0F.1: lmsw,   77 0F.1: invlpg,

\ misc                                                 29mar94py

$02 mod0F: lar, \ r/m reg -- )
$8D modf:  lea, \ m reg -- )
$C4 modf:  les, \ m reg -- )
$C5 modf:  lds, \ m reg -- )
$B2 mod0F: lss, \ m reg -- )
$B4 mod0F: lfs, \ m reg -- )
$B5 mod0F: lgs, \ m reg -- )
\ Pentium/AMD K5 codes
: cpuid, 0F, $A2 _c, ;
: cmpchx8b, 0 $C7 mod0F ; \ m -- )
: rdtsc, 0F, $31 _c, ;
: rdmsr, 0F, $32 _c, ;
: wrmsr, 0F, $30 _c, ;
: rsm, 0F, $AA _c, ;

\ Floating point instructions                          22dec93py

$D8 bc: D8,   $D9 bc: D9,   $DA bc: DA,   $DB bc: DB,
$DC bc: DC,   $DD bc: DD,   $DE bc: DE,   $DF bc: DF,

: D9: builds c, does D9, c@ finish ;

variable fsize
: .fs   0 fsize ! ;  : .fl   4 fsize ! ;  : .fx   3 fsize ! ;
: .fw   6 fsize ! ;  : .fd   2 fsize ! ;  : .fq   7 fsize ! ;
.fx
: fop:  builds c,  does  \ fr/m -- )
    c@ >r
    st? dup 0< 0= if  swap r> >mod 2* $D8 + finish exit  endif
    drop ?mem r> >mod $D8 fsize @ dup 1 and dup 2* + - +
    finish ;
: f@!:
  builds c,
  does  \ fm -- 
   c@ $D9 modf ;

\ Floating point instructions                          08jun92py

$D0 D9: fnop,

$E0 D9: fchs,    $E1 D9: fabs,
$E4 D9: ftst,    $E5 D9: fxam,
$E8 D9: fld1,    $E9 D9: fldl2t,  $EA D9: fldl2e,  $EB D9: fldpi,
$EC D9: fldlg2,  $ED D9: fldln2,  $EE D9: fldz,
$F0 D9: f2xm1,   $F1 D9: fyl2x,   $F2 D9: fptan,   $F3 D9: fpatan,
$F4 D9: fxtract, $F5 D9: fprem1,  $F6 D9: fdecstp, $F7 D9: fincstp,
$F8 D9: fprem,   $F9 D9: fyl2xp1, $FA D9: fsqrt,   $FB D9: fsincos,
$FC D9: frndint, $FD D9: fscale,  $FE D9: fsin,    $FF D9: fcos,

\ Floating point instructions                          23jan94py

00 fop: fadd,    11 fop: fmul,    22 fop: fcom,    33 fop: fcomp,
44 fop: fsub,    55 fop: fsubr,   66 fop: fdiv,    77 fop: fdivr,

: fcompp, 1 STP fcomp, ;
: fbld,   44 $D8 modf ; \ fm -- )
: fbstp,  66 $DF modf ; \ fm -- )
: ffree,  00 $DD modf ; \ st -- )
: fsave,  66 $DD modf ; \ fm -- )
: frstor, 44 $DD modf ; \ fm -- )
: finit,  DB, $E3 _c, ; \ -- )
: fxch,   11 $D9 modf ; \ st -- )

44 f@!: fldenv,  55 f@!: fldcw,   66 f@!: fstenv,  77 f@!: fstcw,

\ fild fst fstsw fucom                                 22may93py
: fucom, ?st st? if 77 else 66 endif $DD modf ; \ st -- )
: fucompp, DA, $E9 _c, ;
: fnclex,  DB, $E2 _c, ;
: fclex,   fwait, fnclex, ;
: fstsw, \ r/m -- )
  dup eax = if  44  else  ?mem 77  endif  $DF modf ;
: f@!,  fsize @ 1 and if  drop  else  nip  endif
    fsize @ $D9 or modf ;
: fx@!, \ mem/st l x -- 
  rot  st? 0=
    if  swap $DD modf drop exit  endif  ?mem -rot
    fsize @ 3 = if drop $DB modf exit endif  f@!, ;
: fst,  \ st/m -- 
  st?  0=
  if  22 $DD modf exit  endif  ?mem 77 22 f@!, ;
: fld,  st? 0= if 0 $D9 modf exit endif 55 0 fx@!, ; \ st/m -- )
: fstp, 77 33 fx@!, ; \ st/m -- )

\ PPro instructions                                    28feb97py


: cmovIF $40 or mod0F ;  \ r/m r flag -- )
: cmov:  builds c, does c@ cmovIF ;
: cmovs:  0 do  i cmov:  loop ;
$10 cmovs: cmovo,  cmovno,   cmovb,   cmovnb,   cmovz,  cmovnz,  cmovbe,  cmovnbe,   cmovs,  cmovns,   cmovpe,  cmovpo,   cmovl,  cmovnl,   cmovle,  cmovnle,

\ MMX opcodes                                          02mar97py

300 7 regs mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7

: mmxs
  dup if
    do i mod0F:  loop
  endif
;
$64 $60 mmxs punpcklbw, punpcklwd, punockldq, packusdw,
$68 $64 mmxs pcmpgtb, pcmpgtw, pcmpgtd, packsswb,
$6C $68 mmxs punpckhbw, punpckhwd, punpckhdq, packssdw,
$78 $74 mmxs pcmpeqb, pcmpeqw, pcmpeqd, emms,
$DA $D8 mmxs psubusb, psubusw,
$EA $E8 mmxs psubsb, psubsw,
$FB $F8 mmxs psubb, psubw, psubd,
$DE $DC mmxs paddusb, paddusw,
$EE $EC mmxs paddsb, paddsw,
$FF $FC mmxs paddb, paddw, paddd,

\ MMX opcodes                                          02mar97py

$D5 mod0F: pmullw,               $E5 mod0F: pmulhw,
$F5 mod0F: pmaddwd,
$DB mod0F: pand,                 $DF mod0F: pandn,
$EB mod0F: por,                  $EF mod0F: pxor,
: pshift \ mmx imm/m mod op --
  imm# if  1 -> imm#  else  + $50 +  endif  mod0F ;
: psrlw,   020 $71 pshift ;  \ mmx imm/m --
: psrld,   020 $72 pshift ;  \ mmx imm/m --
: psrlq,   020 $73 pshift ;  \ mmx imm/m --
: psraw,   040 $71 pshift ;  \ mmx imm/m --
: psrad,   040 $72 pshift ;  \ mmx imm/m --
: psllw,   060 $71 pshift ;  \ mmx imm/m --
: pslld,   060 $72 pshift ;  \ mmx imm/m --
: psllq,   060 $73 pshift ;  \ mmx imm/m --

\ MMX opcodes                                         27jun99beu

\ mmxreg --> mmxreg move
$6F mod0F: movq,

\ memory/reg32 --> mmxreg load
$6F mod0F: pldq,  \ Intel: MOVQ mm,m64
$6E mod0F: pldd,  \ Intel: MOVD mm,m32/r

\ mmxreg --> memory/reg32
: pstq, swap  $7F mod0F ; \ mm m64   --   \ Intel: MOVQ m64,mm
: pstd, swap  $7E mod0F ; \ mm m32/r --  \ Intel: MOVD m32/r,mm

\ 3Dnow! opcodes (K6)                                  21apr00py
: mod0F#  # 1 -> imm mod0F ;   \ code imm --
: 3Dnow:  builds c,  does c@ mod0F# ;   \ imm --
$0D 3Dnow: pi2fd,                $1D 3Dnow: pf2id,
$90 3Dnow: pfcmpge,              $A0 3Dnow: pfcmpgt,
$94 3Dnow: pfmin,                $A4 3Dnow: pfmax,
$96 3Dnow: pfrcp,                $A6 3Dnow: pfrcpit1,
$97 3Dnow: pfrsqrt,              $A7 3Dnow: pfrsqit1,
$9A 3Dnow: pfsub,                $AA 3Dnow: pfsubr,
$9E 3Dnow: pfadd,                $AE 3Dnow: pfacc,
$B0 3Dnow: pfcmpeq,              $B4 3Dnow: pfmul,
$B6 3Dnow: pfrcpit2,             $B7 3Dnow: pmulhrw,
$BF 3Dnow: pavgusb,

: femms,  $0E finish0F ;
: prefetch,  000 $0D mod0F ;    : prefetchw,  010 $0D mod0F ;

\ 3Dnow!+MMX opcodes (Athlon)                          21apr00py

$F7 mod0F: maskmovq,             $E7 mod0F: movntq,
$E0 mod0F: pavgb,                $E3 mod0F: pavgw,
$C5 mod0F: pextrw,               $C4 mod0F: pinsrw,
$EE mod0F: pmaxsw,               $DE mod0F: pmaxub,
$EA mod0F: pminsw,               $DA mod0F: pminub,
$D7 mod0F: pmovmskb,             $E4 mod0F: pmulhuw,
$F6 mod0F: psadbw,               $70 mod0F: pshufw,

$0C 3Dnow: pi2fw,                $1C 3Dnow: pf2iw,
$8A 3Dnow: pfnacc,               $8E 3Dnow: pfpnacc,
$BB 3Dnow: pswabd,               : sfence,   $AE $07 mod0F# ;
: prefetchnta,  000 $18 mod0F ;  : prefetcht0,  010 $18 mod0F ;
: prefetcht1,   020 $18 mod0F ;  : prefetcht2,  030 $18 mod0F ;

\ Assembler Conditionals                               22dec93py
\ cond -- ~cond
: ~cond         1 xor ;
\ start dest --- offbyte
: >offset       swap  2 + -  ?brange ;
\ cond -- here 
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

\ wrap the existing definition of "code" with op which pushes assembler vocab on search stack
also forth definitions
: code
  code
  also assembler		\ push assembler vocab on top of search stack
  sclear
;

\ subroutines must have a "endcode" after its return instruction to pop assembler stack
: subroutine
  create
  also assembler		\ push assembler vocab on top of search stack
  sclear
;

previous definitions

\ exit from a forthop defined by "code"
: next,
  edi jmp,
  previous				\ pop assembler vocab off search stack
;

\ use endcode to end subroutines and "code" forthops which don't end with "next,"
: endcode
  previous				\ pop assembler vocab off search stack
;


\ inline assembly support

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
  $00 _optypeAction
  $04 _numBuiltinOps
  $08 _ops
  $0C _numOps
  $10 _maxOps
  $14 _pEngine
  $18 _IP
  $1C _SP
  $20 _RP
  $24 _FP
  $28 _TPM
  $2C _TPD
  $30 _varMode
  $34 _state
  $38 _error
  $3C _SB
  $40 _ST
  $44 _SLen
  $48 _RB
  $4C _RT
  $50 _RLen
  $54 _pThread
  $58 _pDictionary
  $5C _pFileFuncs
  $60 _innerLoop
  $64 _innerExecute
  $68 _consoleOutStream
  $70 _base
  $74 _signedPrintMode
  $78 _traceFlags
  $7C _scratch
;enum

\ pop IP off rstack using eax
: rpop,
  _RP ebp d] eax mov,
  eax ] esi mov,
  4 # eax add,
  eax _RP ebp d] mov,
;

\ push IP onto rstack using eax
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

	$80 # ebx mov,
	$80 # .w ebx mov,
	$80 # .w esi mov,
	$80 # si mov,
	$80 # di mov,
	$80 # bp mov,
	$80 # sp mov,
	$80 # .d ebx mov,
	$80 # .b ebx mov,
	\ 
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
	