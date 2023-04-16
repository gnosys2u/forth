\
\ floating-point optional word set
\
\ pjm   march 23 2023

requires compatability
requires extops

features
kFFRegular to features

autoforget FLOATING
: FLOATING ;

: fnegate -1.0E f* ;
: sfnegate -1.0Ef sf* ;

cell precision
: set-precision precision! ;

#if FORTH64

alias fdup dup
alias fdrop drop
alias fover over
alias frot rot
alias fswap swap
alias fvariable variable
alias f>s f2l
alias s>f l2sf

15 set-precision

#else

alias fdup 2dup
alias fdrop 2drop
alias fover 2over
alias frot 2rot
alias fswap 2swap
: fvariable variable 0 , ;
alias f>s f2i
alias s>f i2sf

8 set-precision

#endif

alias fliteral lliteral
alias sfliteral iliteral
alias fconstant lconstant

: fround
  fdup f0>= if
    0.5 f+ floor
  else
    0.5 f- fceil
  endif
;

: ftrunc
  fdup f0< if
     fabs floor fnegate
  else
    floor
  endif
;

: >float
  sfloat fval
  _TempStringA blockToString
  _TempStringA "%f" fval& 1 sscanf
  if
    fval true
  else
    false
  endif
;


: f. %g %bl ;

: fdepth depth ;

: fsincos
  fdup fcos fswap fsin
;

: sfsincos
  dup sfcos swap sfsin
;


: falog
  10.0e fswap f**
;

: sfalog
  10.0ef swap sf**
;

: fvalue
  create l,
  does
    getVarop if
      f! 0 setVarop
    else
      f@
    endif
;

: fs. \ prinf 64-bit float with 1 digit before period ("%e" format)
  float val!
  mko String fmt
  fmt.format("%%.%dE " precision 1)
  fprintf(stdout fmt.get val 1)  drop
;

: fe. \ prinf 64-bit float with 1 to 3 digits before period, exponent is multiple of 3
  float val!
  mko String buffer
  buffer.resize(precision 16+)
  buffer.get buffer.size 0 fill
  buffer.set("GaRbAgE")
  represent(val buffer.get precision 1+)
  bool isValid!   bool isNegative!   1- cell exponent!
  if(isValid)
    if(isNegative)
      '-' %c
    endif
    cell leadingDigits
    if(exponent 0<)
      exponent negate 3 mod
      4 swap - leadingDigits!
      leadingDigits 1- exponent!-
    else
      exponent 3 mod 1+ leadingDigits!
      leadingDigits 1- exponent!-
    endif
    buffer.get ptrTo byte pDigits!
    do(leadingDigits 0)
      pDigits b@ %c   pDigits++
    loop
    '.' %c
    pDigits %s
    'E' %c
    exponent %d
  else
    buffer.get %s
  endif
  %bl
;

: f~ ( r1 r2 r3 -- flag ) \ called f-proximate

  float r3!   float r2!   float r1!
  
  if(r3 f0>)
    \ If r3 is positive, flag is true if the absolute value of (r1 minus r2) is less than r3.
    r1 r2 f- fabs r3 f<
  else
    if(r3 f0=)
      \ If r3 is zero, flag is true if the implementation-dependent encoding of r1 and r2 are
      \ exactly identical (positive and negative zero are unequal if they have distinct encodings).
      r1 r2 f=
    else
      \ If r3 is negative, flag is true if the absolute value of (r1 minus r2) is less than the
      \ absolute value of r3 times the sum of the absolute values of r1 and r2.
      r1 r2 f- fabs
      r3 fnegate  r1 fabs r2 fabs f+  f*
      f<
    endif
  endif
;


features!
  
loaddone

: woo  compiled
1e-7 15 0 do  compiled
fdup fe. fdup fs. 10e0 f* cr  compiled
loop ;
woo 100.0E-9 1.000E-7

1.000E-6 1.000E-6
10.00E-6 1.000E-5
100.0E-6 1.000E-4
1.000E-3 1.000E-3
10.00E-3 1.000E-2
100.0E-3 1.000E-1
1.000E0 1.000E0
10.00E0 1.000E1
100.0E0 1.000E2
1.000E3 1.000E3
10.00E3 1.000E4
100.0E3 1.000E5
1.000E6 1.000E6
10.00E6 1.000E7
 ok 0 f:1


: moo 2.0e fsqrt 0 do 10.0e0 f* fdup f. fdup fe. fdup fs. cr loop ;

20 moo
        f.              fe.             fs.
14.142135623731 14.1421356237310E0 1.41421356237310E1
141.42135623731 141.421356237310E0 1.41421356237310E2
1414.2135623731 1.41421356237310E3 1.41421356237310E3
14142.135623731 14.1421356237310E3 1.41421356237310E4
141421.35623731 141.421356237310E3 1.41421356237310E5
1414213.5623731 1.41421356237310E6 1.41421356237310E6
14142135.623731 14.1421356237310E6 1.41421356237310E7
141421356.23731 141.421356237310E6 1.41421356237310E8
1414213562.3731 1.41421356237310E9 1.41421356237310E9
14142135623.7309 14.1421356237309E9 1.41421356237309E10
141421356237.309 141.421356237309E9 1.41421356237309E11
1414213562373.1 1.41421356237310E12 1.41421356237310E12
14142135623731. 14.1421356237310E12 1.41421356237310E13
141421356237310. 141.421356237310E12 1.41421356237310E14
1414213562373100. 1.41421356237310E15 1.41421356237310E15
14142135623731000. 14.1421356237310E15 1.41421356237310E16
141421356237310000. 141.421356237310E15 1.41421356237310E17
1414213562373100000. 1.41421356237310E18 1.41421356237310E18
14142135623731000000. 14.1421356237310E18 1.41421356237310E19
141421356237310000000. 141.421356237310E18 1.41421356237310E20

variable goo 100 allot 
goo 48 0 fill
-123456789.87654321e0 goo 30 represent  ok 15 f:1
TOS: -1 (validFlag) -1 (sign) 9 (exponent)
goo 48 dump
6FFFF877C98: 31 32 33 34  35 36 37 38 - 39 38 37 36  35 34 33 32  1234567898765432
6FFFF877CA8: 30 34 37 39  38 30 35 33 - 39 32 30 39  36 38 00 00  04798053920968..
6FFFF877CB8: 00 00 00 00  00 00 00 00 - 00 00 00 00  00 00 00 00  ................


