//
// basic ops not included in the kernel defined in ARM assembler
//
requires asm_arm
requires forth_internals

autoforget extops

: extops ;

code _doDoesCode
  // RTOS is data ptr, RTOS+1 is IP, IP points to asm code for does action
  rip r0 mov,
  rrp ia! { r1 rip } ldm,	// r1 is data ptr
  r0 bx,
  endcode
  
#if( 0 )
// sample usage
: adder
  builds
    ,
  doescode
    // r1 is data ptr
    r1 ] r2 ldr,
    rsp ] r0 ldr,
    r0 r2 r2 add,
    rsp ] r2 str,
    next,

5 adder add5
#endif
    
code abs
  rsp ] r0 ldr,
  r0 r0 r0 orrs,
  lr ge bx,
  r0 r0 neg,
  rsp ] r0 str,
  next,
  
code +!
  rsp ia! { r0 r1 } ldm,	// r0 is ptr to dst ptr, r1 is amount to add
  r0 ] r2 ldr,
  r1 r2 r2 add,
  r0 ] r2 str,
  next,
  
code roll
  // TOS is number of entries to roll (1 means swap, 2 means rot)
  { r0 } ppop,
  r0 r0 r0 orrs,
  lr eq bx,					// early exit if #entries == 0
  //ne if,
  
    gt if,
      // positive roll
      rsp  r0 2 #lsl  r2 add,
      r2 ] r3 ldr,		// r3 = value to move to TOS
      begin,
        r2 db! { r1 } ldm,
        r2 4 #] r1 str,
        r0 1 # r0 subs,
      eq until,
      rsp ] r3 str,
      lr bx,
    endif,
    
    // negative roll
    r0 r0 neg,
    rsp r2 mov,
    r2 ] r3 ldr,	// r3 = value to move to bottom
    begin,
      r2 4 #] r1 ldr,
      r2 ia! { r1 } stm,
      r0 1 # r0 subs,
    eq until,
    r2 ] r3 str,
    
  //endif,
  next,
  
  
code 2dup
  rsp ia { r0 r1 } ldm,
  rsp db! { r0 r1 } stm,
  next,
  
code 2swap
  rsp ia { r0 r1 r2 } ldm,
  rsp ] r2 str,
  rsp 0xC #] r2 ldr,
  rsp 4 #] r2 str,
  rsp 8 #] r0 str,
  rsp 0xC #] r1 str,
  next,
  
code 2drop
  rsp 8 # rsp add,
  next,

code ndrop
  { r0 } ppop,
  rsp  r0 2 #lsl  rsp add,
  next,
  
code ndup
  // TOS is number of entries to dup
  { r0 } ppop,
  r0 r0 r0 orrs,
  lr eq bx,					// early exit if #entries == 0
  rsp  r0 2 #lsl  r2 add,
  begin,
    r2 4 # r2 sub,
    r2 ] r1 ldr,
    { r1 } ppush,
    r0 1 # r0 subs,
  eq until,
  next,
  
code 2over
  rsp 8 #] r0 ldr,
  rsp 0xC #] r1 ldr,
  rsp db! { r0 r1 } stm,
  next,
  
// 5 -> 1 4 -> 0 3 -> 5 2 -> 4 1 -> 3 0 -> 2
code 2rot
  rsp 0x14 #] r0 ldr,
  rsp 0xC #] r1 ldr,
  rsp 0x14 #] r1 str,
  rsp 4 #] r1 ldr,
  rsp 0xC #] r1 str,
  rsp 4 #] r0 str,
  rsp 0x10 #] r0 ldr,
  rsp 8 #] r1 ldr,
  rsp 0x10 #] r1 str,
  rsp ] r1 ldr,
  rsp 8 #] r1 str,
  rsp ] r0 str,
  next,
  
// 5 -> 3 4 -> 2 3 -> 1 2 -> 0 1 -> 5 0 -> 4
code -2rot
  rsp 0x14 #] r0 ldr,
  rsp 4 #] r1 ldr,
  rsp 0x14 #] r1 str,
  rsp 0xC #] r1 ldr,
  rsp 4 #] r1 str,
  rsp 0xC #] r0 str,
  rsp 0x10 #] r0 ldr,
  rsp ] r1 ldr,
  rsp 0x10 #] r1 str,
  rsp 8 #] r1 ldr,
  rsp ] r1 str,
  rsp 8 #] r0 str,
  next,

code 2nip
  rsp ia { r0 r1 } ldm,
  rsp 16 # rsp add,
  rsp db! { r0 r1 } stm,
  next,

code 2tuck
  { r0 r1 r2 r3 } ppop,
  rsp 8 # rsp sub,
  { r0 r1 r2 r3 } ppush,
  rsp 0x10 #] r0 str,
  rsp 0x14 #] r1 str,
  next,
  
code 2pick
  { r0 } ppop,
  rsp r0 3 #lsl r2 add,
  r2 ia { r0 r1 } ldm,
  { r0 r1 } ppush,
  next,
  
code 2roll
  // TOS is number of entries to roll (1 means swap, 2 means rot)
  { r3 } ppop,
  r3 r3 r3 orrs,
  lr eq bx,					// early exit if #entries == 0
  { rip rfp } push,		// we will use rip,rfp as temps
  
  gt if,
    
    // positive roll
    // save 2 longs at sp+(n * 8)
    // shift (2 * n) longs at sp+8 up by 2 longs
    // stuff 2 saved longs at sp
    rsp  r3 3 #lsl  r2 add,
    r2 ia { rip rfp } ldm,		// rip,rfp = value to move to TOS
    begin,
      r2 -8 #] r0 ldr,
      r2 -4 #] r1 ldr,
      r2 ia { r0 r1 } stm,
      r2 8 # r2 sub,
      r3 1 # r3 subs,
    eq until,
    rsp ia { rip rfp } stm,
      
  else,
    
    // negative roll
    // save 2 longs at sp
    // shift (2 * n) longs at sp+8 down by 2 longs
    // stuff 2 saved longs at sp+(n * 8)
    r3 r3 neg,			// make roll count positive
    
    rsp ia { rip rfp } ldm,		// rip,rfp = value to move to bottom
    rsp r2 mov,
    begin,
      r2 8 #] r0 ldr,
      r2 12 #] r1 ldr,
      r2 ia { r0 r1 } stm,
      r2 8 # r2 add,
      r3 1 # r3 subs,
    eq until,
    r2 ia { rip rfp } stm,

  endif,
    
  { rip rfp } pop,
  next,
  
code lnegate
  rsp ia { r0 r1 } ldm,
  r2 r2 r2 eor,
  r2 r1 r1 subs,
  r2 r0 r0 sbc,
  rsp ia { r0 r1 } stm,
  next,
  
code labs
  rsp ] r0 ldrd,  // r0 is hiword   r1 is lowword
  r0 r0 r0 orrs,
  lr ge bx,				// bail if not negative
  r2 r2 r2 eor,
  r2 r1 r1 subs,
  r2 r0 r0 sbc,
  rsp ] r0 strd,
  next,


// this requires that caller not have extra junk on return stack
code tailRecurse
  // tail recurse by popping the rstack and then moving the IP back one instruction
  { r0 } rpop,
  r0 4 # r0 sub,
  r0 rip mov,
  next,

code 2>r
  { r0 r1 } ppop,
  { r0 r1 } rpush,
  next,
  
code 2r>
  { r0 r1 } rpop,
  { r0 r1 } ppush,
  next,
  
code 2r@
  rrp ia { r0 r1 } ldm,
  { r0 r1 } ppush,
  next,
  
code compareMemory
  // returns null if numBytes memory blocks at block1 and block2 are the same
  //   else returns ptr to first non-matching byte in block1
  { r0 r1 r2 } ppop,
  // r2: numBytes
  // r1: block2
  // r0: block1
  r2 r2 r2 orrs,
  eq if,
    // block with size 0 always counts as a match
    { r2 } ppush,
    lr bx,
  endif,
  { r4 } push,
  r2 r0 r2 add,
  begin,
    r0 1 #] r3 ldrb,
    r1 1 #] r4 ldrb,
    r3 r4 cmp,
    ne if,
      r0 1 # r0 subs,
      { r0 } ppush,
      { r4 } pop,
      lr bx,
    endif,
    r2 r0 cmp,
  eq until,
  { r3 } ppush,
  { r4 } pop,
  next,
  
loaddone
  
  
code */
  next,
  
code */mod
  next,
  
code um/mod
  { r0 r1 r2 } ppop,
  // r0: numeratorLo  r1: numeratorHi     r2: denominator
  //    -> r0: quotient   r1: remainder
  0 # r3 mvn,
  begin,
    r0 r0 r0 adds,
    r1 r1 r1 adcs,
    r2 r1 r1 cc cmp,
    r2 r1 r1 cs sub,
    r0 1 # r0 cs orr,
    r3 1 #lsl r3 movs,
  eq until,
  { r0 r1 } ppush,
  next,
	
code sm/rem
  next,
  
// .fl for 64-bit, .fs for 32-bit

code dpi
  fldpi,
  8 # edx sub,
  .fl edx ] fstp,
  next,
  
code dsq
  edx ] .fl fld,
  fsqrt,
  edx ] .fl fstp,
  next,
  

// eax ebx ecx edx edi esi ebp
// r and d need to be 64-bit
// n and q could be as small as a byte

code l/
  // TOS: dlo edx   dhi edx+4   nlo edx+8   nhi edx+12
  
  ebx ebx xor,			// ebx holds sign flag
  
  // negate denominator if needed
  4 edx d] eax mov,
  eax eax or,
  0<, if,
    ebx not,
    edx ] esi mov,
    esi not,
    eax not,
    1 # esi add,
    0 # eax adc,
    esi edx ] mov,
    eax 4 edx d] mov,
  else,
    // bail if denominator is zero
    0=, if,
      edx ] eax mov,
      eax eax or,
      0=, if,
        // TODO: set divide-by-zero status
        16 # edx add,
        next,
      endif,
  endif,
  
  // negate numerator if needed
  12 edx d] eax mov,
  eax eax or,
  0<, if,
    ebx not,
    8 edx d] esi mov,
    esi not,
    eax not,
    1 # esi add,
    0 # eax adc,
    esi 8 edx d] mov,
    eax 12 edx d] mov,
  endif,
  
  edi push,
  ecx push,
  ebx push,			// sign flag is top of regular stack
  
  // N  edx[12]  edx[8]
  // D  edx[4]   edx[0]
  // R  ebx:eax  rhi:rlo
  // Q
  
  // Q := 0                 initialize quotient and remainder to zero
  // R := 0                     
  // for i = n-1...0 do     where n is number of bits
  //   R := R << 1          left-shift R by 1 bit    
  //   R(0) := N(i)         set the least-significant bit of R equal to bit i of the numerator
  //   if R >= D then
  //     R = R - D               
  //     Q(i) := 1
  //   end
  // loop  

  
  // alo(esi) * blo
  ebx ] eax mov,
  esi mul,				// edx is hipart, eax is final lopart
  edx edi mov,			// edi is hipart accumulator
  
  8 ebx d] esi mov,		// esi = alo
  eax 8 ebx d] mov,

  // alo * bhi
  4 ebx d] eax mov,		// eax = bhi
  esi mul,
  eax edi add,
  
  // ahi * blo
  12 ebx d] esi mov,
  ebx ] eax mov,
  esi mul,
  eax edi add,			// edi = hiResult
  
  // invert result if needed
  ebx pop,
  ebx ebx or,
  0<, if,
    8 ebx d] eax mov,		// eax = loResult
    eax not,
    edi not,
    1 # eax add,
    0 # edi adc,
    eax 8 ebx d] mov,
  endif,
  
  edi 12 ebx d] mov,
  
  8 # ebx add,
  ebx edx mov,
  ecx pop,
  edi pop,
  next,
  

code goo
  _TDPtr ebp d] eax mov,		// eax = this ptr
  4 eax d] ebx mov,			// ebx = first word of object data
  4 # edx sub,
  ebx edx ] mov,
  next,
  
class: boo

  int aa
  m: getaa
    goo
  ;m
  
  m: setaa
    -> aa
  ;m
  
;class

  
