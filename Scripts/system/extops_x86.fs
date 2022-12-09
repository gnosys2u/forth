//
// basic ops not included in the kernel defined in x86 assembler
//

autoforget extops

requires asm_pentium
requires forth_internals

: extops ;

code _doDoesCode
  // RTOS is data ptr, RTOS+1 is IP, esi (IP) points to asm code for does action
  esi ecx mov,				// ecx is does action code
  _RP ebp d] ebx mov,
  ebx ] eax mov,			// eax is data ptr
  4 ebx d] esi mov,
  8 # ebx add,				// cleanup rstack
  ebx _RP ebp d] mov,
  ecx jmp,
  endcode
  
#if( 0 )
// sample usage
: adder
  builds
    ,
  doescode
    // eax is data ptr
    eax ] ebx mov,
    edx ] ebx add,
    ebx edx ] mov,
    next,

5 adder add5
#endif
    
code abs
  edx ] eax mov,
  eax eax or,
  0<, if,
    eax neg,
    eax edx ] mov,
  endif,
  next,
  
code +!
  edx ] eax mov,
  4 edx d] ebx mov,
  eax ] ebx add,
  ebx eax ] mov,
  8 # edx add,
  next,
  
code 1+!
  edx ] eax mov,
  4 # edx add,
  eax ] ebx mov,
  1 # ebx add,
  ebx eax ] mov,
  next,
  
code 1-!
  edx ] eax mov,
  4 # edx add,
  eax ] ebx mov,
  1 # ebx sub,
  ebx eax ] mov,
  next,
  
code roll
  esi push,
  edx ] ecx mov,
  4 # edx add,
  ecx ecx or,
  nz, if,
    0>=, if,
      edx ecx *4 i] eax lea,
      eax ] ebx mov,
      do,
        4 # eax sub,
        eax ] esi mov,
        esi 4 eax d] mov,
      loop,
      ebx edx ] mov,
    else,
      ecx neg,
      edx ] esi mov,
      edx eax mov,
      do,
        4 eax d] ebx mov,
        ebx eax ] mov,
        4 # eax add,
      loop,
      esi eax ] mov,
    endif,
  endif,
  esi pop,
  next,
  
code 2dup
  edx ] eax mov,
  4 edx d] ebx mov,
  8 # edx sub,
  eax edx ] mov,
  ebx 4 edx d] mov,
  next,
  
code 2swap
  edx ] eax mov,
  8 edx d] ebx mov,
  eax 8 edx d] mov,
  ebx edx ] mov,
  4 edx d] eax mov,
  12 edx d] ebx mov,
  eax 12 edx d] mov,
  ebx 4 edx d] mov,
  next,
  
code 2drop
  8 # edx add,
  next,

code ndrop
  edx ] eax mov,
  4 edx eax *4 di] edx lea,
  next,
  
code ndup
  edx ] ecx mov,
  edx ecx *4 i] ebx lea,
  4 # edx add,
  ecx ecx or,
  nz, if,
    do,
      4 # edx sub,
      ebx ] eax mov,
      eax edx ] mov,
      4 # ebx sub,
    loop,
  endif,
  next,
  
code 2over
  8 edx d] eax mov,
  12 edx d] ebx mov,
  8 # edx sub,
  eax edx ] mov,
  ebx 4 edx d] mov,
  next,
  
// 5 -> 1 4 -> 0 3 -> 5 2 -> 4 1 -> 3 0 -> 2
code 2rot
  20 edx d] eax mov,
  12 edx d] ebx mov,
  ebx 20 edx d] mov,
  4 edx d] ebx mov,
  ebx 12 edx d] mov,
  eax 4 edx d] mov,
  16 edx d] eax mov,
  8 edx d] ebx mov,
  ebx 16 edx d] mov,
  edx ] ebx mov,
  ebx 8 edx d] mov,
  eax edx ] mov,
  next,
  
// 5 -> 3 4 -> 2 3 -> 1 2 -> 0 1 -> 5 0 -> 4
code -2rot
  20 edx d] eax mov,
  4 edx d] ebx mov,
  ebx 20 edx d] mov,	// 1->5 complete
  12 edx d] ebx mov,
  ebx 4 edx d] mov,		// 3->1 complete
  eax 12 edx d] mov,	// 5->3 complete
  16 edx d] eax mov,
  edx ] ebx mov,
  ebx 16 edx d] mov,	// 0->4 complete
  8 edx d] ebx mov,
  ebx edx ] mov,		// 2->0
  eax 8 edx d] mov,	// 4->2
  next,

code 2nip
  edx ] eax mov,
  4 edx d] ebx mov,
  8 # edx add,
  eax edx ] mov,
  ebx 4 edx d] mov,
  next,
  
code 2tuck
  8 # edx sub,
  8 edx d] eax mov,
  eax edx ] mov,
  16 edx d] ebx mov,
  eax 16 edx d] mov,
  ebx 8 edx d] mov,
  12 edx d] eax mov,
  eax 4 edx d] mov,
  20 edx d] ebx mov,
  eax 20 edx d] mov,
  ebx 12 edx d] mov,
  next,
  
code 2pick
  edx ] eax mov,
  1 # eax add,
  4 # edx sub,
  edx eax *8 i] ebx mov,
  ebx edx ] mov,
  4 edx eax *8 di] ebx mov,
  ebx 4 edx d] mov,
  next,
  
code 2roll
  esi push,
  edi push,
  edx ] ecx mov,
  4 # edx add,
  ecx ecx or,
  nz, if,
    0>=, if,
      edx ecx *8 i] eax lea,
      eax ] ebx mov,
      4 eax d] edi mov,
      do,
        8 # eax sub,
        eax ] esi mov,
        esi 8 eax d] mov,
        4 eax d] esi mov,
        esi 12 eax d] mov,
      loop,
      ebx edx ] mov,
      edi 4 edx d] mov,
    else,
      ecx neg,
      edx ] esi mov,
      4 edx d] edi mov,
      edx eax mov,
      do,
        8 eax d] ebx mov,
        ebx eax ] mov,
        12 eax d] ebx mov,
        ebx 4 eax d] mov,
        8 # eax add,
      loop,
      esi eax ] mov,
      edi 4 eax d] mov,
    endif,
  endif,
  edi pop,
  esi pop,
  next,
  
code lnegate
  4 edx d] eax mov,
  edx ] ebx mov,
  eax not,
  ebx not,
  1 # eax add,
  0 # ebx adc,
  ebx edx ] mov,
  eax 4 edx d] mov,
  next,
  
code labs
  edx ] ebx mov,
  ebx ebx or,
  0<, if,
    4 edx d] eax mov,
    eax not,
    ebx not,
    1 # eax add,
    0 # ebx adc,
    ebx edx ] mov,
    eax 4 edx d] mov,
  endif,
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
  
// this requires that caller not have extra junk on return stack
code tailRecurse
  // tail recurse by popping the rstack and then moving the IP back one instruction
  _RP ebp d] eax mov,
  eax ] ebx mov,
  4 # ebx sub,
  ebx esi mov,
  4 # eax add,
  eax _RP ebp d] mov,
  next,

code 2>r
  _RP ebp d] eax mov,
  8 # eax sub,
  edx ] ebx mov,
  ebx eax ] mov,
  4 edx d] ebx mov,
  ebx 4 eax d] mov,
  8 # edx add,
  eax _RP ebp d] mov,
  next,
  
code 2r>
  _RP ebp d] eax mov,
  8 # edx sub,
  eax ] ebx mov,
  ebx edx ] mov,
  4 eax d] ebx mov,
  ebx 4 edx d] mov,
  8 # eax add,
  eax _RP ebp d] mov,
  next,
  
code 2r@
  _RP ebp d] eax mov,
  8 # edx sub,
  eax ] ebx mov,
  ebx edx ] mov,
  4 eax d] ebx mov,
  ebx 4 edx d] mov,
  next,
  
code */
  edx ebx mov,
  4 ebx d] eax mov,
  8 ebx d] ecx mov,
  ecx eax imul,      // result hiword in edx, loword in eax
  ebx ] idiv,
  8 # ebx add,
  eax ebx ] mov,
  ebx edx mov,
  next,
  
code */mod
  edx ebx mov,
  4 ebx d] eax mov,
  8 ebx d] ecx mov,
  ecx eax imul,      // result hiword in edx, loword in eax
  ebx ] idiv,
  4 # ebx add,
  eax ebx ] mov,
  edx 4 ebx d] mov,
  ebx edx mov,
  next,
  
code um/mod
  // edx: 32-bit unsigned denominator
  // edx+4: 64-bit unsigned numerator
  edx ebx mov,
  ebx ] ecx mov,      // denominator
  8 ebx d] eax mov,   // numerator low part
  4 ebx d] edx mov,   // numerator high part
  ecx div,            // eax is quotient, edx is remainder
  4 # ebx add,
  edx 4 ebx d] mov,
  eax ebx ] mov,
  ebx edx mov,
  next,
	
code sm/rem
  // edx: 32-bit signed denominator
  // edx+4: 64-bit signed numerator
  // idiv takes 64-bit numerator in edx:eax
  edx ebx mov,
  8 ebx d] eax mov,   // numerator low part
  4 ebx d] edx mov,   // numerator high part
  ebx ] idiv,
  4 # ebx add,
  edx 4 ebx d] mov,
  eax ebx ] mov,
  ebx edx mov,
  next,
  
code compareMemory
  // edx: numBytes
  // edx+4: block2
  // edx+8: block1
  // returns null if numBytes memory blocks at block1 and block2 are the same
  //   else returns ptr to first non-matching byte in block1
  edx ] ecx mov,
  4 edx d] eax mov,   // eax: block2 base address
  8 # edx add,
  ecx ecx or,
  z, if,
    // block with size 0 always counts as a match
    ecx edx ] mov,
    edi jmp,    // exit
  endif,
  edi push,
  edx ] edi mov,   // edi: block1 base address
  begin,
    edi ] bl mov,
    eax ] bl cmp,
    nz, if,
      edi edx ] mov,
      edi pop,
      edi jmp,   // exit
    endif,
    1 # edi add,
    1 # eax add,
    1 # ecx sub,
  jnz,
  
  eax eax xor,
  eax edx ] mov,
  edi pop,
  next,
  
loaddone


: l*
  -> long b
  -> long a
  false -> int signs
  
  b l0< if
    b lnegate -> b
    true -> signs
  endif
  a l0< if
    a lnegate -> a
    signs 0= -> signs
  endif
  
  b
  -> int bhi
  -> int blo
  a
  -> int ahi
  -> int alo
  
  0 -> int mhi
  0 -> int mlo
  
  alo blo um* -> mhi -> mlo
  alo bhi um* drop ->+ mhi
  ahi blo um* drop ->+ mhi
  mlo mhi
  signs if
    lnegate
  endif
;



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
  esi push,
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
  esi pop,
  edi pop,
  next,
  

code goo
  _TDP ebp d] eax mov,		// eax = this ptr
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

