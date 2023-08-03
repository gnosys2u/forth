\
\ floating-point optional word set using fp stack
\
\ pjm   2023march23   (fpstack version 2023august2)

requires compatability
requires extops


features
kFFRegular to features

autoforget FLOATING
: FLOATING ;

cell precision
: set-precision precision! ;


#if FORTH64

\ _fdup/_fswap/_fdrop/_fover  are aliases which manipulates FP values on param stack
alias _fdup dup
alias _fswap swap
alias _fdrop drop
alias _fover over

: frot fpop fpop fpop >r fpush fpush r> fpush ;
: f>s fpop df>l ;
: s>f l>df fpush ;
alias fvariable variable

15 set-precision

: dfnegate $8000000000000000 xor ;

#else

alias _fdup 2dup
alias _fswap 2swap
alias _fdrop 2drop
alias _fover 2over

: frot fpop fpop fpop 2>r fpush fpush 2r> fpush ;
: f>s fpop df>i ;
: s>f i>df fpush ;
: fvariable variable 0 , ;

8 set-precision

: dfnegate swap $80000000 xor swap ;

#endif

\ display fp stack
: fs
  fdepth "fs[" %s _fdup %d "] " %s
  dup >r
  0 ?do
    fpop _fdup %g %bl
  loop
  r> 0 ?do
    fpush
  loop
  cr
;

: fdup fpop _fdup fpush fpush ;
: fswap fpop fpop _fswap fpush fpush ;
: fdrop fpop _fdrop ;
: fover fpop fpop _fswap _fover fpush fpush fpush ;

: fnegate fpop dfnegate fpush ;

: f@ l@ fpush ;
: f! >r fpop r> l! ;
alias df@ f@
alias df! f!

: fconstant
  create fpop l,
  does l@ fpush
;

: >float
  >dfloat if
    fpush true
  else
    false
  endif
;

: represent 2>r fpop 2r> dfrepresent ;

: sf! >r fpop df>sf r> i! ;
: sf@ ui@ sf>df fpush ;

DFloat:nan  fpush fconstant nan
DFloat:+inf fpush fconstant +inf
DFloat:-inf fpush fconstant -inf

: dfround
  dfloat val!
  val df0>= if
    val 0.5e df+ dffloor
  else
    val 0.5e df- dfceil
  endif
;
: fround fpop dfround fpush ;

: dftrunc
  dfloat val!
  val df0< if
     val dfabs dffloor dfnegate
  else
    val dffloor
  endif
;
: ftrunc fpop dftrunc fpush ;

: f. fpop %g %bl ;

: fsincos
  fpop dfloat angle!
  angle dfsin fpush angle dfcos fpush
;

: falog
  10.0e fpop df** fpush
;

: fvalue
  create l,
  does
    getVarop if
      fpop l! 0 setVarop
    else
      l@ fpush
    endif
;

: fs. \ prinf 64-bit float with 1 digit before period ("%e" format)
  fpop dfloat val!
  mko String fmt
  fmt.format("%%.%dE " precision 1)
  fprintf(stdout fmt.get val 1)  drop
;

: fe. \ prinf 64-bit float with 1 to 3 digits before period, exponent is multiple of 3
  fpop dfloat val!
  mko String buffer
  buffer.resize(precision 16+)
  buffer.get buffer.size 0 fill
  buffer.set("GaRbAgE")
  dfrepresent(val buffer.get precision 1+)
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

  fpop dfloat r3!   fpop dfloat r2!   fpop dfloat r1!
  
  if(r3 df0>)
    \ If r3 is positive, flag is true if the absolute value of (r1 minus r2) is less than r3.
    r1 r2 df- dfabs r3 df<
  else
    if(r3 df0=)
      \ If r3 is zero, flag is true if the implementation-dependent encoding of r1 and r2 are
      \ exactly identical (positive and negative zero are unequal if they have distinct encodings).
      r1 r2 l=      \ use l= instead of df= so +0 and -0 return unequal
    else
      \ If r3 is negative, flag is true if the absolute value of (r1 minus r2) is less than the
      \ absolute value of r3 times the sum of the absolute values of r1 and r2.
      r1 r2 df- dfabs
      r3 dfnegate  r1 dfabs r2 dfabs df+  df*
      df<
    endif
  endif
;

: f0< fpop df0< ;
: f0= fpop df0= ;
: f< fpop fpop _fswap df< ;
: f> fpop fpop _fswap df> ;

: fabs fpop dfabs fpush ;
: facos fpop dfacos fpush ;
: facosh fpop dfacosh fpush ;
: fasin fpop dfasin fpush ;
: fasinh fpop dfasinh fpush ;
: fatan fpop dfatan fpush ;
: fatanh fpop dfatanh fpush ;
: floor fpop dffloor fpush ;
: fround fpop dfround fpush ;
: fcos fpop dfcos fpush ;
: fcosh fpop dfcosh fpush ;
: fexp fpop dfexp fpush ;
: fexpm1 fpop dfexpm1 fpush ;
: fln fpop dfln fpush ;
: flnp1 fpop dflnp1 fpush ;
: flog fpop dflog fpush ;
: fsin fpop dfsin fpush ;
: fsinh fpop dfsinh fpush ;
: fsqrt fpop dfsqrt fpush ;
: ftan fpop dftan fpush ;
: ftanh fpop dftanh fpush ;
: ftrunc fpop dftrunc fpush ;

: f+ fpop fpop df+ fpush ;
: f* fpop fpop df* fpush ;
: fmax fpop fpop dfmax fpush ;
: fmin fpop fpop dfmin fpush ;

: f- fpop fpop _fswap df- fpush ;
: f/ fpop fpop _fswap df/ fpush ;
: f** fpop fpop _fswap df** fpush ;
: fatan2 fpop fpop _fswap dfatan2 fpush ;

: fliteral ['] flit i, fpop l, ;  precedence fliteral

: d>f d>df fpush ;
: f>d fpop df>d ;

: ds ds fdepth if fs endif ;

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


