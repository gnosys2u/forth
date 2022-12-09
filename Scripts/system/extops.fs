// basic ops not included in the kernel

FORTH64 [if]

lf extops_amd64

[else]

ARCH_X86 [if]

lf extops_x86

[else]

ARCH_ARM [if]

lf extops_arm

[endif]
[endif]
[endif]

: doescode
  postpone does
  ['] _doDoesCode here 4- !
  also assembler
  0 state !
  postpone recursive
;
precedence doescode

: fm/mod
  -> int denominator
  -> long numerator
  sm/rem( numerator denominator )
  -> int quotient
  -> int remainder
  if( remainder 0= )
    remainder quotient
    exit
  endif
  if( and( denominator 0<  remainder 0< ) )
    remainder quotient
    exit
  endif
  if( and( denominator 0>  remainder 0>= ) )
    remainder quotient
    exit
  endif
  denominator ->+ remainder
  1 ->- quotient
  remainder quotient
;

// ud1 c-addr1 u1 -- ud2 c-addr2 u2 
: >number
  //ds
  -> int numChars
  -> int pSrc
  -> long accum
  base @ i2l -> long lbase
  begin
    pSrc @ -> byte ch
    if( ch `0` >= ch `9` <= and )
      `0` ->- ch
    else
      if( ch `A` >= ch `Z` <= and  ch `a` >= ch `z` <= and  or )
        ch 0x20 or -> ch  // lowercase it
        `a` 0xa - ->- ch
      else
        // force exit
        base @ -> ch
      endif
    endif
    if( ch base @ < )
      1 ->+ pSrc
      1 ->- numChars
      accum lbase l* ch i2l l+ -> accum
      //"adding " %s ch %d " total " %s accum %2d  %bl %bl numChars %d " chars left" %s %nl
      numChars 0=
    else
      true
    endif
  until

  accum
  pSrc
  numChars
;

loaddone

