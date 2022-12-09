requires compatability

autoforget asm_arm

: asm_arm ;

vocabulary assembler

: there here ;
: t@ @ ;
: t! ! ;
: t,
  //"compile " %s dup %x " at " %s here %x %nl
  ,
;
: 2, , , ;  // or should there be a swap first?
: uu %s %bl %bl ds ; // debug print

base @

//
// Simple ARM7 RPN Assembler
//
// Copyright (C) David Kühling 2007-2009
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

// mov mvn : fewer ops!
// s-flag for TEQ? always set !?
// mula -> mla

hex  // EVERYTHING BELOW IS IN HEXADECIMAL!

also assembler definitions

// operand types
enum: eARM // 0      1           2      3    4        5       6           7
  70000000 register shifted #immediate psr cxsf-mask offset multimode register-list 
  80000000 forward backward
;enum

: nand invert and ;		// x1 -- x2
: ?register   // n --
   register <> abort" Invalid operand, need a register r0..r15" ;
: ?psr   // n --
   psr <> abort" Invalid operand, need special register SPSR or CSPR" ;

// Registers
: regs:  10 0 do  i register 2constant loop ;
regs: r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 r15

000000 psr 2constant CPSR	400000 psr 2constant SPSR

// Bit masks
: bit:  /* n "name" -- */   1 swap lshift constant ;
: bits // 0 bit1 ... bitN -- x
   0  begin over or  swap 0= until ;

19 bit: %I   18 bit: %P   17 bit: %U   16 bit: %B   15 bit: %W

// Basic instruction creation, condition codes
int instruction
int had-cc
: encode  /* x1 -- */   instruction  or  -> instruction ;
: ?can-cc /* -- */
   had-cc abort" Attempt to specify condition code twice."
   instruction 1C rshift 0<>
   abort" Condition code not allowed for instruction." ;
: cc:  // x "name" --
  builds ,
  does @  // x --
   ?can-cc  1C lshift encode   true -> had-cc ;

00 cc: eq	01 cc: ne	02 cc: cs	03 cc: cc
04 cc: mi	05 cc: pl	06 cc: vs	07 cc: vc
08 cc: hi	09 cc: ls	0a cc: ge	0b cc: lt
0c cc: gt	0d cc: le	0e cc: alw	0f cc: nv
02 cc: hs	03 cc: lo

: <instruction  // x --
   dup  0F0000000 and if
      had-cc abort" Condition code not allowed for instruction"
   else  had-cc 0= if alw then   then
   encode ;
: instruction>  // -- x
   instruction   0 -> instruction  false -> had-cc ;

// Simple register operands
: register-operand:  // bit-offs "name" --
   builds ,
  does @  // n-reg 'register' n-bit -- mask
   >r ?register r> lshift   encode ;

10 register-operand: Rn,	0C register-operand: Rd,
10 register-operand: RdHi,	0C register-operand: RdLo,
8 register-operand: Rs,		 0 register-operand: Rm,

// PSR register operands
: psr,  ?psr  encode ;

// Field mask (for MSR)
: cxsf-mask:  /* #bit "name" -- */  1 swap lshift  cxsf-mask 2constant ;
: cxsf,  begin  dup cxsf-mask = while  drop encode  repeat ;
10 cxsf-mask: C   11 cxsf-mask: X   12 cxsf-mask: S   13 cxsf-mask: F

// Right-hand side operands
: lrotate32  // x1 n -- x2
   2dup lshift 0FFFFFFFF and >r    20 swap - rshift r> or ;
: #  // n -- x
   10 0 ?do
      dup 0 100 within if
	 i 8 lshift or  %I or  #immediate unloop exit
      then
      2 lrotate32
   loop
   abort" Immediate operand cannot be expressed as shifted 8 bit value" ;

: ?shift // x1 -- x1
   dup 1 20 within 0= abort" Invalid shift value" ;
: #shift: // mask "name" --
  builds ,
  does @  // n-reg 'register' shift mask --  operand 'shifted'
   >r   ?shift 7 lshift >r  ?register  r> or  r> or   shifted  ;
: rshift:  // mask "name" -- 
  builds ,
  does @  // n-reg 'register' mask --  operand 'shifted'
    >r   ?register 8 lshift >r  ?register   r> or  r> or   010 or shifted ;
: rrx  // n-reg 'register' -- operand 'shifted'
   ?register  060 or  shifted ;
   
000 dup #shift: #lsl  rshift: lsl	020 dup #shift: #lsr  rshift: lsr
040 dup #shift: #asr  rshift: asr	060 dup #shift: #ror  rshift: ror

: ?rhs  // 'shifted'|'register'|'#immediate' --
   >r r@ shifted <>  r@ #immediate <> and  r> register <> and
   abort" Need a (shifted) register or immediate value as operand" ;
: ?#shifted-register // x 'shifted'|'register' -- x
   >r r@ shifted <>  r> register <> and  abort" Need a (shifted) register here"
   dup 010 and  abort" Shift by register not allowed here" ;
: rhs,  // x 'r-shifted'|`#-shifted' --
   ?rhs  encode ;
: rhs',  // x 'r-shifted'|`#-shifted' --
   dup shifted = abort" Shifted registers not allowed here."  rhs, ;

// Addressing modes
: offset:  /* 0 bit1 ... bitN "name" -- */  bits   offset 2constant ;

0 %P %I %U	offset: +]
0 %P %I		offset: -]
0 %P %I %U %W	offset: +]!
0 %P %I    %W	offset: -]!
0    %I %U	offset: ]+
0    %I		offset: ]-
0 %P 		offset: #]
0 %P       %W	offset: #]!
0    		offset: ]#

: ]   0 #] ;
: [#]  // addr -- r15 offs 'offset'     generate PC-relative address
   >r r15  r> there 8 +  -   #] ;

: multimode:  /* 0 bit1 ... bitN "name" -- */  bits  multimode 2constant ;
 
0 		multimode: da     
0    %U		multimode: ia 
0 %P		multimode: db 
0 %P %U		multimode: ib 
0 	%W	multimode: da!    
0    %U	%W	multimode: ia!
0 %P	%W	multimode: db!
0 %P %U	%W	multimode: ib!

: ?offset  // 'offset' --
   offset <> abort" Invalid operand, need an address offset e.g ' Rn ] ' " ;
: ?multimode  // 'offset' --
   multimode <> abort" Need an address mode for load/store multiple: DA etc." ;
: ?upwards  // n1 -- n2
   dup 0< if  negate else %U encode then ;
: ?post-offset  // x 'offset' -- x
   ?offset  dup %P and 0=
   abort" Only post-indexed addressing, ]#, ]+ or ]- , allowed here" ;
: ?0#]  // 0 'offset' --
   ?offset    0 #] drop d<>
   abort" Only addresses without offset, e.g r0 ] allowed here" ;
: #offset12,  // n --
   ?upwards  dup 000 1000 within 0= abort" Offset out of range"  encode ;
: #offset8,  // n --
   ?upwards  dup 000 100 within 0= abort" Offset out of range"
   %B encode	// %B replaces (inverted) %I-bit for  8-bit offsets!
   dup 0F and /* low nibble*/ encode  0F0 and 4 lshift  /* high nibble*/ encode ;
: R#shifted-offset,  // n  'register'|'shifted-reg' --
   ?#shifted-register encode  ;
: R-offset,  // n  'register'|'shifted-reg' --
   ?register encode  ;
: offs12,  // x1..xn 'offset' --
   ?offset dup encode
   %I and 0= if  #offset12, else R#shifted-offset, then ;
: offsP,  // x1..xn 'offset' --
   2dup ?post-offset drop  offs12, ;
: offs8,  // x1..xn 'offset' --      limited addressing for halword load etc.
   ?offset dup %I nand encode
   %I and 0= if  #offset8, else R-offset, then ;
: mmode,  // x 'multimode' --
   ?multimode encode ;

// Branch offsets
: ?branch-offset  // offset -- offset
   dup -2000000 2000000 within 0= abort" Branch destination out of range"
   dup 3 and 0<> abort" Branch destination not 4 byte-aligned" ;
: branch-addr>offset  /* src dest -- offset */   swap 8 +  -   ?branch-offset ;
: branch-offset>bits  /* offset -- x */  2 rshift 0FFFFFF and ;
: branch-addr,  // addr -- x
   there swap branch-addr>offset  branch-offset>bits  encode ;
: a<mark  /* -- addr 'backward' */  there backward ;
: a<resolve  // addr 'backward' -- addr
   backward <> abort" Expect assembler backward reference on stack" ;
: a>mark  /* -- addr 'forward' addr */  there forward   over ;
: a>resolve  // addr 'forward' --
   forward <> abort" Expect assembler forward reference on stack"
   dup  there branch-addr>offset  branch-offset>bits
   over t@ 0FF000000 and  or   swap t! ;

// "Comment" fields (SVC/SWI)
: ?comment  // x -- x
   dup 0 01000000 within 0= abort" Comment field is limited to 24 bit values" ;
: comment,  // x --
   ?comment encode ;

// Register lists //for LDM and STM*/
: {  /* -- mark */  77777777 ;
: }  // mark reg1 .. regN -- reglist
   0 begin over 77777777 <> while
	 swap ?register   1 rot lshift or
   repeat  nip register-list ;
: r-r  // reg1 regN -- reg1 reg2... regN
   ?register  swap ?register  1+ ?do  i register loop ;
: ?register-list  // 'register-list' --
   register-list <> abort" Need a register list { .. } as operand" ;
: reg-list,  // x 'register-list' --
   ?register-list encode ;
   
// Mnemonics
//: instruction-class:  /* xt "name" -- */  create ,
//  DOES> @  /* mask xt "name" -- */  create 2,
//  DOES> 2@   /* mask xt -- */  >r   <instruction r> execute instruction> t, ;
: _instclass @  // mask xt "name" --
  builds 2,
  does 2@   /* mask xt -- */  >r   <instruction r> execute instruction> t, ;
: instruction-class:  // xt "name" --
  builds ,
  does _instclass ;

:noname  Rd,	rhs,	Rn, ;		instruction-class: data-op:
:noname  rhs,	Rn, ;			instruction-class: cmp-op:
:noname  Rd,	rhs, ;			instruction-class: mov-op:
:noname  Rd,	psr, ;			instruction-class: mrs-op:
:noname  cxsf, psr, rhs', ;	instruction-class: msr-op:
:noname  Rd,	offs12, Rn, ;		instruction-class: mem-op:
:noname  Rd,	offsP,  Rn, ;		instruction-class: memT-op:
:noname  Rd,	offs8,  Rn, ;		instruction-class: memH-op:
:noname  Rd,	Rm,	?0#] Rn, ;	instruction-class: memS-op:
:noname  reg-list, mmode, Rn, ;		instruction-class: mmem-op:
:noname  branch-addr, ;			instruction-class: branch-op:
:noname  Rn,	Rs,	Rm, ;		instruction-class: RRR-op:
:noname  Rd,	Rn,	Rs,	Rm, ;	instruction-class: RRRR-op:
:noname  RdHi, RdLo,	Rs,	Rm, ;	instruction-class: RRQ-op:
:noname  comment, ;			instruction-class: comment-op:
:noname  Rm, ;				instruction-class: branchR-op:
: mmem-op2x:  /* x "name1" "name2" -- */  dup mmem-op: mmem-op: ;

00000000 data-op: and,         00100000 data-op: ands,
00200000 data-op: eor,		   00300000 data-op: eors,
00400000 data-op: sub,		   00500000 data-op: subs,
00600000 data-op: rsb,		   00700000 data-op: rsbs,
00800000 data-op: add,		   00900000 data-op: adds,
00a00000 data-op: adc,		   00b00000 data-op: adcs,
00c00000 data-op: sbc,		   00d00000 data-op: sbcs,
00e00000 data-op: rsc,		   00f00000 data-op: rscs,
01100000 cmp-op:  tst,		   0110f000 cmp-op:  tstp,
01300000 cmp-op:  teq,		   0130f000 cmp-op:  teqp,
01500000 cmp-op:  cmp,		   0150f000 cmp-op:  cmpp,
01700000 cmp-op:  cmn,		   0170f000 cmp-op:  cmnp,
01800000 data-op: orr,		   01900000 data-op: orrs,
01a00000 mov-op:  mov,		   01b00000 mov-op:  movs,
01c00000 data-op: bic,		   01d00000 data-op: bics,
01e00000 mov-op:  mvn,		   01f00000 mov-op:  mvns,

04000000 mem-op:  str,		   04100000 mem-op:  ldr,             
04400000 mem-op:  strb,        04500000 mem-op:  ldrb,
04200000 memT-op: strt,		   04300000 memT-op: ldrt, 
04600000 memT-op: strbt,	   04700000 memT-op: ldrbt,
000000b0 memH-op: strh,		   001000b0 memH-op: ldrh, 
001000f0 memH-op: ldrsh,	   001000d0 memH-op: ldrsb,

000000f0 memH-op: strd,		   000000d0 memH-op: ldrd, 

01000090 memS-op: swp,		   01400090 memS-op: swpb,

08000000 mmem-op:  stm,		   08100000 mmem-op: ldm,
08400000 mmem-op:  ^stm,	   08500000 mmem-op: ^ldm,

010f0000 mrs-op:  mrs,		   


0a000000 branch-op:  b,		   0b000000 branch-op:  bl,
//0a120010 branchR-op: bx,
12fff10 branchR-op: bx,
0f000000 comment-op: swi,	   0f000000 comment-op: svc,

00000090 RRR-op:  mul,	 	   00100090 RRR-op:  muls, 
00200090 RRRR-op: mla,	 	   00300090 RRRR-op: mlas, 
00800090 RRQ-op:  umull,	   00900090 RRQ-op:  umulls,
00A00090 RRQ-op:  umlal,	   00B00090 RRQ-op:  umlals,
00C00090 RRQ-op:  smull,	   00D00090 RRQ-op:  smulls,
000E0090 RRQ-op:  smlal,	   00F00090 RRQ-op:  smlals,

// floating point ops
// n m d vsub,		d=n-m
// single/double is defined by register used (rsXX or rdXX)

enum: _fpRegs
  rs0  rs1  rs2  rs3  rs4  rs5  rs6  rs7  rs8  rs9  rs10 rs11 rs12 rs13 rs14 rs15
  rs16 rs17 rs18 rs19 rs20 rs21 rs22 rs23 rs24 rs25 rs26 rs27 rs28 rs29 rs30 rs31
  0x40
  rd0  rd1  rd2  rd3  rd4  rd5  rd6  rd7  rd8  rd9  rd10 rd11 rd12 rd13 rd14 rd15
  rd16 rd17 rd18 rd19 rd20 rd21 rd22 rd23 rd24 rd25 rd26 rd27 rd28 rd29 rd30 rd31
;enum

: isFPRegister
  -> int fpReg
  or( within( fpReg rs0 rs31 1+ ) within( fpReg rd0 rd31 1+ ) )
;

: ?fpregister isFPRegister not abort" invalid fp register" ;

: encodeFPReg
  -> int regExtraBit
  -> int regShift
  -> int isDouble
  -> int fpReg
  
  fpReg rd0 >= isDouble <> abort" mixed single and double precision registers"
  ?fpregister( fpReg )
  fpReg 0x3F and -> fpReg
  if( isDouble )
    lshift( and( fpReg 0xF ) regShift )
    if( fpReg 0xF > )
      regExtraBit or
    endif
  else
    lshift( fpReg 2/ regShift )
    if( and( fpReg 1 ) )
      regExtraBit or
    endif
  endif
  encode
;

: fp-three-reg-op:
  builds ,
  does
    // instructionBits rDst rM rN
    // rDst = rN OP rM
  
    @ <instruction
  
    -> int rDst
    -> int rM			
    -> int rN

    rDst rd0 >= -> int isDouble
  
    encodeFPReg( rDst isDouble 0xC 0x400000 )
    encodeFPReg( rN isDouble 0x10 0x80 )
    encodeFPReg( rM isDouble 0 0x20 )
    if( isDouble )
      encode( 0x100 )
    endif
    instruction> t,
;

0xEE000A00 fp-three-reg-op: vmla,		0xEE000A40 fp-three-reg-op: vmls,
0xEE100A40 fp-three-reg-op: vnmla,		0xEE100A00 fp-three-reg-op: vnmls,
0xEE200A00 fp-three-reg-op: vmul,		0xEE200A40 fp-three-reg-op: vnmul,
0xEE300A00 fp-three-reg-op: vadd,		0xEE300A40 fp-three-reg-op: vsub,
0xEE800A00 fp-three-reg-op: vdiv,
0xEE900A00 fp-three-reg-op: vfnma,		0xEE900A40 fp-three-reg-op: vfnms,
0xEEA00A00 fp-three-reg-op: vfma,		0xEEA00A40 fp-three-reg-op: vfms,

: fp-two-reg-op:
  builds ,
  does
    // instructionBits rDst rM
    // rDst = OP(rM)
  
    @ <instruction
  
    -> int rDst
    -> int rM			

    rDst rd0 >= -> int isDouble
  
    encodeFPReg( rDst isDouble 0xC 0x400000 )
    encodeFPReg( rM isDouble 0 0x20 )
    if( isDouble )
      encode( 0x100 )
    endif
    instruction> t,
;

0xEEB00AC0 fp-two-reg-op: vabs,		0xEEB10A40 fp-two-reg-op: vneg,
0xEEB10AC0 fp-two-reg-op: vsqrt,
0xEEB40A40 fp-two-reg-op: vcmp,		0xEEB40AC0 fp-two-reg-op: vcmpe,

: fp-one-reg-op:
  builds ,
  does
    // instructionBits rDst
    // compare rDst to 0
  
    @ <instruction
  
    -> int rDst

    rDst rd0 >= -> int isDouble
  
    encodeFPReg( rDst isDouble 0xC 0x400000 )
    if( isDouble )
      encode( 0x100 )
    endif
    instruction> t,
;
0xEEB50A40 fp-one-reg-op: vcmpz,		0xEEB50AC0 fp-one-reg-op: vcmpze,

: fp-mem-op:
  builds ,
  does
    @ <instruction
    // rDst offsetMarker 1000000? #offset regMarker #reg
    -> int rDst
    rDst rd0 >= -> int isDouble
    
    ?offset drop
    -> int rawOffset
    
    ?register
    -> int srcReg
    
    not( within( rawOffset -400 400 ) ) abort" offset out of range"
    rawOffset 4/ dup
    if( 0< )
      negate
    else
      or( 0x800000 )
    endif
    or( lshift( srcReg 0x10 ) )
    if( isDouble )
      or( 0x100 )
    endif
    encode
    
    encodeFPReg( rDst isDouble 0xC 0x400000 )
    instruction> t,
;
0xED100A00 fp-mem-op: vldr,  		0xED000A00 fp-mem-op: vstr,

// 1110 110p udw1  rn   vd  101s immediate8		vldm rn	{regs}	0xEC100A00	immediate8 is register count (double for 64-bit regs)
// 1110 110p udw0  rn   vd  101s immediate8		vstm rn	{regs}	0xEC000A00	immediate8 is register count
// puw control autoincrement mode just like int vdm/ldm instructions
// REG ia/ia!/db! BASE_FPREG LAST_REG vldm, 

: fp-multireg-op:
  builds ,
  does
    @ <instruction
    
    2dup ?fpregister ?fpregister
    over - 1+
    -> int nRegs
    -> int baseReg
    baseReg rd0 >= -> int isDouble
  
    not( within( nRegs 0 32 ) ) abort" bad fp register list"
    
    mmode, Rn,
  
    encodeFPReg( baseReg isDouble 0xC 0x400000 )
    encode( nRegs if( isDouble ) 2* endif )
    if( isDouble )
      encode( 0x100 )
    endif
    
    instruction> t,
;
0xEC100A00 fp-multireg-op: vldm,	0xEC000A00 fp-multireg-op: vstm,


// rA rsB vmov,         rsA rB vmov,         0xEE000A10  move between one int reg and single-precision fp reg
// rA rB rdC vmov,      rdA rB rC vmov,      0xEC400B10  move between two int regs and double-precision fp reg
// rsA rsB rC rD vmov,  rA rB rsC rsD vmov,  0xEC400A10  move between two int regs and two consecutive single-precision fp regs
: vmov,
  int regLo
  int regHi
  int fpReg
  0 -> int fromFP
  false -> int isDouble
  false -> int twoRegs

  if( dup register = )
  
    // dst is int register
    drop -> regHi
    if( dup register = )
      // dst is int register pair
      drop -> regLo
      true -> twoRegs
    endif
    -> fpReg
    ?fpregister( fpReg )
    fpReg rd0 >= -> isDouble
    if( isDouble )
      // rdA rB rC case
      not( twoRegs ) abort" vmov missing second single-precision reg"
    else
      if( twoRegs )
        // rsA rsB rC rD case
        if( isFPRegister( dup ) )
          1 ->- fpReg
          fpReg <> abort" vmov single-precision regs not contiguous"
        else
          drop true abort" vmov missing second single-precision reg"
        endif
      endif
    endif
    0x100000 -> fromFP
    
  else
  
    // dst is fp register
    -> fpReg
    fpReg rd0 >= -> isDouble
    if( isDouble )
      // rA rB rdC case
      ?register -> regHi
      ?register -> regLo
      true -> twoRegs
    else
      if( isFPRegister( dup ) )
        // rA rB rsC rsD case
        1 ->- fpReg
        fpReg <> abort" vmov single-precision regs not contiguous"
        ?register -> regHi
        ?register -> regLo
        true -> twoRegs
      else
        // rA rsB case
        ?register -> regHi
      endif
    endif
    
  endif
  
  if( twoRegs )
    if( isDouble ) 0xEC400B10 else 0xEC400A10 endif <instruction
    encodeFPReg( fpReg isDouble 0 0x20 )
    encode( lshift( regLo 0xC ) )
    encode( lshift( regHi 0x10 ) )
  else
    0xEE000A10 <instruction
    encodeFPReg( fpReg false 0x10 0x80 )
    encode( lshift( regHi 0xC ) )
  endif
  
  instruction>
  or( fromFP )
  t,
;

//
// Labels and branch resolving
//
: label  there constant ;
: if-not,  a>mark b, ;
: ahead,  alw if-not, ;
: then,  a>resolve ;
: else,  a>mark alw b,  2swap then, ;
: begin,  a<mark ;
: until-not,  a<resolve b, ;
: again,  alw until-not, ;
: while-not,  if-not, ;
: repeat,  2swap  again,  then, ;
: repeat-until-not,  2swap  until-not,  then, ;
          
: invertConditionCodes
  instruction 10000000 xor -> instruction  // invert bit 28 to flip sense of flags
;
: if, invertConditionCodes if-not, ;
: endif,  a>resolve ;
: until,  invertConditionCodes a<resolve b, ;
: while, if, ;
: repeat-until,  2swap  until,  then, ;

//
// Register aliases
//
r15 2constant pc
r14 2constant lr
r13 2constant sp
r11 2constant raction	// optype action table
r10 2constant rnumops	// number of entries in rops
r9  2constant rops	// opcode table
r8  2constant rfp	// local variable fram pointer
r7  2constant rrp	// forth return stack pointer
r6  2constant rsp	// forth stack pointer
r5  2constant rip	// forth instruction pointer
r4  2constant rcore	// forth code base pointer

// pseudo-ops
: pop, -> long reglist sp ia! reglist ldm, ;
: push, -> long reglist sp db! reglist stm, ;
: ppop, -> long reglist rsp ia! reglist ldm, ;
: ppush, -> long reglist rsp db! reglist stm, ;
: rpop, -> long reglist rrp ia! reglist ldm, ;
: rpush, -> long reglist rrp db! reglist stm, ;
: neg, -> long dstReg 0 # dstReg rsb, ;

: next,
  lr bx,
  previous
;

// wrap the existing definition of "code" with op which pushes assembler vocab on search stack
also forth definitions
: code
  code
  also assembler		// push assembler vocab on top of search stack
;

// subroutines must have a "endcode" after its return instruction to pop assembler stack
: subroutine
  create
  also assembler		// push assembler vocab on top of search stack
;

// use endcode to end subroutines and "code" forthops which don't end with "next,"
: endcode
  previous				// pop assembler vocab off search stack
;

code testAsm
  rs1 rs2 rs3 vadd,
  rs4 rs5 rs6 vsub,
  rs7 rs8 rs9 vmul,
  rs10 rs11 rs12 vdiv,
  rs1 rs2 rs3 vmla,
  rs4 rs5 rs6 vmls,
  rs1 rs2 rs3 vnmla,
  rs4 rs5 rs6 vnmls,
  rs4 rs5 rs6 vnmul,
  rs0 rs1 vabs,
  rs2 rs3 vneg,
  rs4 rs5 vsqrt,
  rd1 rd2 rd3 vadd,
  rd4 rd5 rd6 vsub,
  rd7 rd8 rd9 vmul,
  rd10 rd11 rd12 vdiv,
  rd1 rd2 rd3 vmla,
  rd4 rd5 rd6 vmls,
  rd1 rd2 rd3 vnmla,
  rd4 rd5 rd6 vnmls,
  rd4 rd5 rd6 vnmul,
  rd0 rd1 vabs,
  rd2 rd3 vneg,
  rd4 rd5 vsqrt,
  rsp 4 #] rs3 vldr,
  rsp -4 #] rs3 vstr,
  rsp -4 #] rd3 vldr,
  rsp 4 #] rd3 vstr,
  r0 rs2 vmov,
  rs2 r0 vmov,
  r1 r2 rd2 vmov,
  rd2 r1 r2 vmov,
  rs8 rs9 r3 r4 vmov,
  r3 r4 rs8 rs9 vmov,
  rrp ia! rd3 rd8 vldm,
  rsp db! rs4 rs13 vstm,
  rs1 rs8 vcmp,
  rd7 rd4 vcmpe,
  rs9 vcmpze,
  rd14 vcmpz,
  next,
  
enum: eForthCore
  0x00 _optypeAction
  0x04 _builtinOps
  0x08 _numBuiltinOps
  0x0C _numAsmBuiltinOps
  0x10 _userOps
  0x14 _numUserOps
  0x18 _maxUserOps
  0x1C _pEngine
  0x20 _IPtr
  0x24 _SPtr
  0x28 _RPtr
  0x2C _FPtr
  0x30 _TMPtr
  0x34 _TDPtr
  0x38 _varMode
  0x3C _state
  0x40 _error
  0x44 _SBPtr
  0x48 _STPtr
  0x4C _SLen
  0x50 _RBPtr
  0x54 _RTPtr
  0x58 _RLen
  0x5C _pThread
  0x60 _pDictionary
  0x64 _fileFuncs
;enum

previous definitions
: ]ASM   also assembler ; 
: ASM[   previous ;

: [ASM]   ]ASM ; precedence [ASM]
: [END-ASM]   ASM[ ; precedence [END-ASM]

base !

loaddone

vsub d,n,m		becomes			n m d vsub,		d=n-m


// fpreg encoding is broken into a single bit and a 4-bit chunk
// in double precision, the single bit is on top, in single precision it is on bottom

// 1110 1110 0d11  vn   vd  101s n0m0  vm		vadd				0xEE300A00
// 1110 1110 0d11  vn   vd  101s n0m0  vm		vsub				0xEE300A40
// 1110 1110 0d11  vn   vd  101s n0m0  vm		vmul				0xEE200A00
// 1110 1110 1d00  vn   vd  101s n0m0  vm		vdiv				0xEE800A00
// 1110 1110 0d00  vn   vd  101s npm0  vm		vmla/vmls			0xEE000A00/0xEE000A40			p=1 vmls, p=0 vmla
// 1110 1110 0d01  vn   vd  101s npm0  vm		vnmla/vnmls			0xEE100A00/0xEE100A40			p=1 vmls, p=0 vmla
// 1110 1110 0d10  vn   vd  101s n1m0  vm		vnmul				0xEE200A40
// 1110 1110 1d11 0000  vd  101s 11m0  vm		vabs sd, sm			0xEEB00AC0
// 1110 1110 1d11 0001  vd  101s 01m0  vm		vneg sd, sm			0xEEB10A40
// 1110 1110 1d11 0001  vd  101s 11m0  vm		vsqrt sd, sm		0xEEB10AC0
// 1110 1101 ud01  rn   vd  101s immediate8		vldr				u=1 means add immediate8, u=0 means subtract immediate8
// 1110 1101 ud00  rn   vd  101s immediate8		vstr
// 1110 1110 1d11 0100  vd  101s 01m0  vm		vcmp sd, sm			0xEEB40A40
// 1110 1110 1d11 0100  vd  101s 11m0  vm		vcmpe sd, sm		0xEEB40AC0
// 1110 1110 1d11 0101  vd  101s 0100 0000		vcmpz sd			0xEEB50A40
// 1110 1110 1d11 0101  vd  101s 1100 0000		vcmpze sd			0xEEB50AC0
// 1110 110p udw1  rn   vd  101s immediate8		vldm rn	{regs}		0xEC100A00	immediate8 is register count
// 1110 1110 000p  vn   rt  101s n001 0000      vmov rt, sn     	0xEE000A10  single register, p is fromFP
// 1110 1100 010p  rt2  rt  1010 00m1  vm       vmov rt,rt2,dm  	0xEC400A10  2 int registers and single registers
// 1110 1100 010p  rt2  rt  1011 00m1  vm       vmov rt,rt2,sm,sm+1	0xEC400B10  2 int registers and double register
// 1110 1110 1d11 1222  vd  101s p1m0  vm		vcvt 


rsp rs11 ] vldr,		0xed505600		ldcl 6, cr5, [r0, #-0] according to gdb
vldr
cond 1101 ud01  rn   vd  101s imm8
1110 1101 1101 0110 0111 1010 0000 0000		edd67a00	vldr s15,[r6]
1110 1101 0101 0000 0101 0110 0000 0000
0xED100600
vstr
cond 1101 ud00  rn   vd  101s imm8
1110 1101 1100 0110 0111 1010 0000 0000		edc67a00	vstr s15,[r6]

vmov 32-bit
cond 1110 000p  vn   rt  1010 n001 0000
p=1 means to arm register, p=0 means to fp register

vmov 64-bit
cond 1100 010p rt2  rt  1010 00m1  vm
p=1 means to arm register, p=0 means to fp register

r1 rs3 vmov,		ee111a90
rs3 r1 vmov,		ee011a90


general layout of fp data manipulation instructions:
	111t 1110 <opc1> <opc2> ???? 101s <opc3>?0 opc4
opc1	opc2	opc3
0x00	-		-		vmla, vmls		multuply accumulate or subtract
0x01	-		-		vnmla, vnmls	negate multuply accumulate or subtract
0x10	-		x0		vmul			multiply
0x10	-		x1		vnmul			negate multiply
0x11	-		x0		vadd
0x11	-		x1		vsub
1x00	-		x0		vdiv
1x11	-		x0		vmov immediate
1x11	0000	01		vmov register
1x11	0000	11		vabs
1x11	0001	01		vneg
1x11	0001	11		vsqrt


-----------------------
Data operations can take 3 operands.
REGISTER_A OPERAND_B DESTINATION_REGISTER op,
OPERAND_B 
r1 r2 r3 sub, -> r3 = r2 - r1

The first operand and destination operand must be registers.
The second operand can be a register, an immediate value, or a shifted register
  register
    just a plain old register
  shifted
    REG LITERAL_NUMBER #lsl   (or #lsr | #asr | #ror)
    REG REG_SHIFT #lsl        (or #lsr | #asr | #ror)
    REG rrx					rotate right including carry bit
  #immediate
    an immediate value which is made of an 8-bit literal and a 4-bit shift code
    use syntax LITERAL_NUMBER # to encode this
-----------------------
http://www.complang.tuwien.ac.at/forth/gforth/Docs-html/ARM-Assembler.html

5.26.8 ARM Assembler

The ARM assembler included in Gforth was written from scratch by David Kuehling.

The assembler includes all instruction of ARM architecture version 4, but does not (yet) have support for Thumb instructions. It also lacks support for any co-processors.

The assembler uses a postfix syntax with the target operand specified last. For load/store instructions the last operand will be the register(s) to be loaded from/stored to.

Registers are specified by their names r0 through r15, with the aliases pc, lr, sp, ip and fp provided for convenience. Note that ip means intra procedure call scratch register (r12) and does not refer to the instruction pointer.

Condition codes can be specified anywhere in the instruction, but will be most readable if specified just in front of the mnemonic. The 'S' flag is not a separate word, but encoded into instruction mnemonics, ie. just use adds, instead of add, if you want the status register to be updated.

The following table lists the syntax of operands for general instructions:

     Gforth          normal assembler      description
     123 #           #123                  immediate
     r12             r12                   register
     r12 4 #LSL      r12, LSL #4           shift left by immediate
     r12 r1 #LSL     r12, LSL r1           shift left by register
     r12 4 #LSR      r12, LSR #4           shift right by immediate
     r12 r1 #LSR     r12, LSR r1           shift right by register
     r12 4 #ASR      r12, ASR #4           arithmetic shift right
     r12 r1 #ASR     r12, ASR r1           ... by register
     r12 4 #ROR      r12, ROR #4           rotate right by immediate
     r12 r1 #ROR     r12, ROR r1           ... by register
     r12 RRX         r12, RRX              rotate right with extend by 1

Memory operand syntax is listed in this table:

     Gforth            normal assembler      description
     r4 ]              [r4]                  register
     r4 4 #]           [r4, #+4]             register with immediate offset
     r4 -4 #]          [r4, #-4]             with negative offset
     r4 r1 +]          [r4, +r1]             register with register offset
     r4 r1 -]          [r4, -r1]             with negated register offset
     r4 r1 2 #LSL -]   [r4, -r1, LSL #2]     with negated and shifted offset
     r4 4 #]!          [r4, #+4]!            immediate preincrement
     r4 r1 +]!         [r4, +r1]!            register preincrement
     r4 r1 -]!         [r4, +r1]!            register predecrement
     r4 r1 2 #LSL +]!  [r4, +r1, LSL #2]!    shifted preincrement
     r4 -4 ]#          [r4], #-4             immediate postdecrement
     r4 r1 ]+          [r4], r1              register postincrement
     r4 r1 ]-          [r4], -r1             register postdecrement
     r4 r1 2 #LSL ]-   [r4], -r1, LSL #2     shifted postdecrement
     ' xyz >body [#]   xyz                   PC-relative addressing

Register lists for load/store multiple instructions are started and terminated by
using the words { and } respectivly. Between braces, register names can be listed
one by one, or register ranges can be formed by using the postfix operator r-r.
The ^ flag is not encoded in the register list operand, but instead directly
encoded into the instruction mnemonic, ie. use ^ldm, and ^stm,.

Addressing modes for load/store multiple are not encoded as instruction suffixes,
but instead specified after the register that supplies the address.
Use one of DA, IA, DB, IB, DA!, IA!, DB! or IB!.

The following table gives some examples:

     Gforth                           normal assembler
     { r0 r7 r8 }  r4 ia  stm,        stmia    {r0,r7,r8}, r4
     { r0 r7 r8 }  r4 db!  ldm,       ldmdb    {r0,r7,r8}, r4!
     { r0 r15 r-r }  sp ia!  ^ldm,    ldmfd    {r0-r15}^, sp!

Conditions for control structure words are specified in front of a word:

     r1 r2 cmp,    // compare r1 and r2
     eq if,        // equal?
        ...          // code executed if r1 == r2
     then,

Here is an example of a code word (assumes that the stack pointer is in r9, and that r2 and r3 can be clobbered):

     code my+ ( n1 n2 --  n3 )
        r9 IA!       { r2 r3 } ldm,  // pop r2 = n2, r3 = n1
        r2   r3      r3        add,  // r3 = n2+n1
        r9 -4 #]!    r3        str,  // push r3
        next,
     end-code

Look at arch/arm/asm-example.fs for more examples. 
