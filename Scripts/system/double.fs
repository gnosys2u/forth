\
\ double-number optional word set
\
\ pjm   march 23 2023

requires extops
requires compatability

getFeatures
kFFRegular setFeatures

autoforget double
: double ;

: dabs
  dup 0< if
    dnegate
  endif
;

: dcmp
  cell bhi! cell blo!  cell ahi! cell alo!
  ahi bhi cmp
  dup 0= if
    drop alo blo ucmp
  endif
;

: d= d- or 0= ;
: d0= or 0= ;
: d< dcmp 0< ;
: d<= dcmp 0<= ;
: d> dcmp 0> ;
: d>= dcmp 0>= ;
: d0< swap drop 0< ;
  
: du<
  rot swap      \ alo blo ahi bhi :TOS
  ucmp ?dup if
    >r  2drop r> 0<
  else
    u<
  endif
;

: dmax 2over 2over d< if 2swap endif 2drop ;

: dmin 2over 2over d> if 2swap endif 2drop ;

: d2* 2dup d+ ;

: d2/
  swap 1 rshift
  over 1 and if
    CELL_HIGHEST_BIT or
  endif
  swap 2/
;

: m+ s>d d+ ;

: m*/
  >r s>d >r abs -rot s>d r> xor r> swap
  >r >r dabs rot tuck um* 2swap um* swap
  >r 0 d+ r> -rot i um/mod -rot r> um/mod -rot r>
  if
    if
      1 0 d+
    endif
    dnegate
  else
    drop
  endif
;

: 2value
  create , ,
  does
    getVarop if
      2! 0 setVarop
    else
      2@
    endif
;

: 2variable  variable 0 , ;

setFeatures

loaddone
