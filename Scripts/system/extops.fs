\ basic ops not included in the kernel

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

#if 0
: fm/mod
  cell denominator!
  long numerator!
  sm/rem( numerator denominator )
  int quotient!
  int remainder!
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
  denominator remainder!+
  quotient--
  remainder quotient
;
#endif

: fm/mod ( d n -- rem quot )
  dup >r
  sm/rem
  ( if the remainder is not zero and has a different sign than the divisor )
  over dup 0<> swap 0< r@ 0< xor and if
    1- swap r> + swap
  else
    rdrop
  then
;

\ ud1 c-addr1 u1 -- ud2 c-addr2 u2 
: >number
  \ ds
  cell numChars!
  cell pSrc!
  dcell accum
  accum 2!
  begin
    pSrc b@ byte ch!
    if( ch `0` >= ch `9` <= and )
      `0` ch!-
    else
      if( ch `A` >= ch `Z` <= and  ch `a` >= ch `z` <= and  or )
        ch $20 or ch!  \ lowercase it
        `a` $a - ch!-
      else
        \ force exit
        base @ ch!
      endif
    endif
    if( ch base @ < )
      pSrc++
      numChars--
      accum.lo base @ um* accum.hi base @ um* swap d+ ch 0 d+ accum 2!
      \ "adding " %s ch %d " total " %s accum %2d  %bl %bl numChars %d " chars left" %s %nl
      numChars 0=
    else
      true
    endif
  until

  accum 2@
  pSrc
  numChars
;

: ud/mod
\  >r 0 i um/mod r> swap >r um/mod r>
  cell aa! 0 aa um/mod aa swap aa! um/mod aa
;

: dabs
  dup 0< if
    dnegate
  endif
;


loaddone

