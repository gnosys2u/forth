//
// basic ops not included in the kernel defined in amd64 assembler
//

autoforget extops

requires asm_amd64
requires forth_internals

: extops ;

//; x86 register usage:
//;	EDX		SP
//;	ESI		IP
//;	EDI		inner interp PC (constant)
//;	EBP		core ptr (constant)

// scratch: rax rcx rdx r9

code _doDoesCode
  // TODO!
  // RTOS is data ptr, RTOS+1 is IP, IP points to asm code for does action
  rip rcx mov,				// rcx is does action code
  rrp ] rax mov,			// rax is data ptr
  8 rrp d] rip mov,
  0x10 # rrp add,				// cleanup rstack
  rcx jmp,
  endcode
  
#if(0)
// sample usage
: adder
  builds
    ,
  doescode
    // rax is data ptr
    rax ] rbx mov,
    rpsp ] rbx add,
    rbx rpsp ] mov,
    next,

5 adder add5
#endif
    
code abs
  rpsp ] rax mov,
  rax rax or,
  0<, if,
    rax neg,
    rax rpsp ] mov,
  endif,
  next,
  
code +!
  rpsp ] rax mov,
  8 rpsp d] rbx mov,
  rax ] rbx add,
  rbx rax ] mov,
  0x10 # rpsp add,
  next,
  
code 1+!
  rpsp ] rax mov,
  8 # rpsp add,
  rax ] rbx mov,
  1 # rbx add,
  rbx rax ] mov,
  next,
  
code 1-!
  rpsp ] rax mov,
  8 # rpsp add,
  rax ] rbx mov,
  1 # rbx sub,
  rbx rax ] mov,
  next,
  
//0xff setTrace
code roll
  rpsp ] rcx mov,
  8 # rpsp add,
  rcx rcx or,
  nz, if,
    0>=, if,
      rpsp rcx *8 i] rax lea,
      rax ] rbx mov,
      do,
        8 # rax sub,
        rax ] r8 mov,
        r8 8 rax d] mov,
      loop,
      rbx rpsp ] mov,
    else,
      rcx neg,
      rpsp ] r8 mov,
      rpsp rax mov,
      do,
        8 rax d] rbx mov,
        rbx rax ] mov,
        8 # rax add,
      loop,
      r8 rax ] mov,
    endif,
  endif,
  next,
  
code 2dup
  rpsp ] rax mov,
  8 rpsp d] rbx mov,
  0x10 # rpsp sub,
  rax rpsp ] mov,
  rbx 8 rpsp d] mov,
  next,
  
code 2swap
  rpsp ] rax mov,
  0x10 rpsp d] rbx mov,
  rax 0x10 rpsp d] mov,
  rbx rpsp ] mov,
  8 rpsp d] rax mov,
  0x18 rpsp d] rbx mov,
  rax 0x18 rpsp d] mov,
  rbx 8 rpsp d] mov,
  next,
  
code 2drop
  0x10 # rpsp add,
  next,

code ndrop
  rpsp ] rax mov,
  8 rpsp rax *8 di] rpsp lea,
  next,
  
code ndup
  rpsp ] rcx mov,
  rpsp rcx *8 i] rbx lea,
  8 # rpsp add,
  rcx rcx or,
  nz, if,
    do,
      8 # rpsp sub,
      rbx ] rax mov,
      rax rpsp ] mov,
      8 # rbx sub,
    loop,
  endif,
  next,
  
code 2over
  0x10 rpsp d] rax mov,
  0x18 rpsp d] rbx mov,
  0x10 # rpsp sub,
  rax rpsp ] mov,
  rbx 8 rpsp d] mov,
  next,
  
// 5 -> 1 4 -> 0 3 -> 5 2 -> 4 1 -> 3 0 -> 2
code 2rot
  0x28 rpsp d] rax mov,
  0x18 rpsp d] rbx mov,
  rbx 0x28 rpsp d] mov,
  8 rpsp d] rbx mov,
  rbx 0x18 rpsp d] mov,
  rax 8 rpsp d] mov,
  0x20 rpsp d] rax mov,
  0x10 rpsp d] rbx mov,
  rbx 0x20 rpsp d] mov,
  rpsp ] rbx mov,
  rbx 0x10 rpsp d] mov,
  rax rpsp ] mov,
  next,
  
// 5 -> 3 4 -> 2 3 -> 1 2 -> 0 1 -> 5 0 -> 4
code -2rot
  0x28 rpsp d] rax mov,
  0x8 rpsp d] rbx mov,
  rbx 0x28 rpsp d] mov,	// 1->5 complete
  0x18 rpsp d] rbx mov,
  rbx 0x8 rpsp d] mov,		// 3->1 complete
  rax 0x18 rpsp d] mov,	// 5->3 complete
  0x20 rpsp d] rax mov,
  rpsp ] rbx mov,
  rbx 0x20 rpsp d] mov,	// 0->4 complete
  0x10 rpsp d] rbx mov,
  rbx rpsp ] mov,		// 2->0
  rax 0x10 rpsp d] mov,	// 4->2
  next,

code 2nip
  rpsp ] rax mov,
  8 rpsp d] rbx mov,
  0x10 # rpsp add,
  rax rpsp ] mov,
  rbx 8 rpsp d] mov,
  next,
  
code 2tuck
  0x10 # rpsp sub,
  0x10 rpsp d] rax mov,
  rax rpsp ] mov,
  0x20 rpsp d] rbx mov,
  rax 0x20 rpsp d] mov,
  rbx 0x10 rpsp d] mov,
  0x18 rpsp d] rax mov,
  rax 0x8 rpsp d] mov,
  0x28 rpsp d] rbx mov,
  rax 0x28 rpsp d] mov,
  rbx 0x18 rpsp d] mov,
  next,
  
code 2pick
  rpsp ] rax mov,
  1 # rax add,
  rax rax add,
  8 # rpsp sub,
  rpsp rax *8 i] rbx mov,
  rbx rpsp ] mov,
  8 rpsp rax *8 di] rbx mov,
  rbx 8 rpsp d] mov,
  next,
  
code 2roll
  rsi push,
  rdi push,
  rpsp ] rcx mov,
  8 # rpsp add,
  rcx rcx or,
  nz, if,
    0>=, if,
      rcx rax mov,
      4 # rax shl,
      rpsp rax add,
      rax ] rbx mov,
      8 rax d] rdi mov,
      do,
        0x10 # rax sub,
        rax ] rsi mov,
        rsi 0x10 rax d] mov,
        8 rax d] rsi mov,
        rsi 0x18 rax d] mov,
      loop,
      rbx rpsp ] mov,
      rdi 8 rpsp d] mov,
    else,
      rcx neg,
      rpsp ] rsi mov,
      8 rpsp d] rdi mov,
      rpsp rax mov,
      do,
        0x10 rax d] rbx mov,
        rbx rax ] mov,
        0x18 rax d] rbx mov,
        rbx 8 rax d] mov,
        0x10 # rax add,
      loop,
      rsi rax ] mov,
      rdi 4 rax d] mov,
    endif,
  endif,
  rdi pop,
  rsi pop,
  next,
  
// .fl for 64-bit, .fs for 32-bit

// this requires that caller not have extra junk on return stack
code tailRecurse
  // tail recurse by popping the rstack and then moving the IP back one instruction
  rrp ] rbx mov,
  8 # rbx sub,
  rbx rip mov,
  8 # rrp add,
  next,

code 2>r
  0x10 # rrp sub,
  rpsp ] rbx mov,
  rbx rrp ] mov,
  8 rpsp d] rbx mov,
  rbx 8 rrp d] mov,
  8 # rpsp add,
  next,
  
code 2r>
  0x10 # rpsp sub,
  rrp ] rbx mov,
  rbx rpsp ] mov,
  8 rrp d] rbx mov,
  rbx 8 rpsp d] mov,
  0x10 # rrp add,
  next,
  
code 2r@
  0x10 # rpsp sub,
  rrp ] rbx mov,
  rbx rpsp ] mov,
  8 rrp d] rbx mov,
  rbx 8 rpsp d] mov,
  next,
  
code */
  8 rpsp d] rax mov,
  0x10 rpsp d] rcx mov,
  rcx rax imul,      // result hiword in rdx, loword in rax
  rpsp ] idiv,
  0x10 # rpsp add,
  rax rpsp ] mov,
  next,
  
code */mod
  8 rpsp d] rax mov,
  0x10 rpsp d] rcx mov,
  rcx rax imul,      // result hiword in edx, loword in rax
  rpsp ] idiv,
  8 # rpsp add,
  rax rpsp ] mov,
  rdx 8 rpsp d] mov,
  next,

code um/mod
  // rpsp: 64-bit unsigned denominator
  // rpsp+8: 128-bit unsigned numerator
  rpsp ] rcx mov,      // denominator
  0x10 rpsp d] rax mov,   // numerator low part
  8 # rpsp add,
  rpsp ] rdx mov,   // numerator high part
  rcx div,            // rax is quotient, edx is remainder
  rdx 8 rpsp d] mov,
  rax rpsp ] mov,
  next,
	
code sm/rem
  // rpsp: 64-bit signed denominator
  // rpsp+8: 128-bit signed numerator
  // idiv takes 128-bit numerator in edx:rax
  0x10 rpsp d] rax mov,   // numerator low part
  8 rpsp d] rdx mov,   // numerator high part
  rpsp ] idiv,
  8 # rpsp add,
  rdx 8 rpsp d] mov,
  rax rpsp ] mov,
  next,
  
code compareMemory
  // rpsp: numBytes
  // rpsp+8: block2
  // rpsp+16: block1
  // returns null if numBytes memory blocks at block1 and block2 are the same
  //   else returns ptr to first non-matching byte in block1
  rpsp ] rcx mov,
  8 rpsp d] rax mov,   // rax: block2 base address
  0x10 # rpsp add,
  rcx rcx or,
  z, if,
    // block with size 0 always counts as a match
    rcx rpsp ] mov,
    rnext jmp,    // exit
  endif,
  rpsp ] r8 mov,   // r8: block1 base address
  begin,
    r8 ] bl mov,
    rax ] bl cmp,
    nz, if,
      r8 rpsp ] mov,
      rnext jmp,    // exit
    endif,
    1 # r8 add,
    1 # rax add,
    1 # rcx sub,
  jnz,
  
  rax rax xor,
  rax rpsp ] mov,
  next,
  
loaddone

