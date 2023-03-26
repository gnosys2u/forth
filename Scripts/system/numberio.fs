
requires double

autoforget numberio
: numberio ;

base @ decimal

250 constant padSize
variable pad padSize allot

variable holdbuf padSize allot				    \ permanent base of hold buffer
align
holdbuf padSize 1- + ptrTo byte holdbuf-end!	\ permanent end of hold buffer
ptrTo byte holdptr								\ current pos in hold buffer (grows downward)
ptrTo byte holdend								\ current nested end of hold buffer

\ hold <# #> sign # #s                                 25jan92py

\ add char on tos to numeric output buffer
: hold 
  holdptr--
  if( holdptr holdbuf u< )
    drop
    holdptr++@
    "hold:{" %s holdbuf padSize type "}\n" %s
    error( "numeric output overflow in hold" )
  else
    holdptr c!
  endif
;

\ clear the numeric output string
: <# holdbuf-end dup -> holdptr -> holdend ;
<#

\ finish numeric output and return the address and length of string
: #>
  2drop
  holdptr holdend over -
;

\ start a nested numeric output
: <<#
  \ stuff length of previous nested numeric output at bottom of hold
  holdend holdptr - hold
  \ holdend now points to stuffed length byte
  holdptr holdend!
;

\ end a nested numeric output
: #>>
  if( holdend holdbuf-end u< not )
    error( "bounds error in #>>" )
  else
    holdend count bounds holdptr! holdend!
  endif
;

: sign
  if( 0< )
    hold( '-' )
  endif
;

\ generate one digit of numeric output
\ takes unsigned 64-bit on TOS, leaves same after dividing by base
: #
  base @ ud/mod rot dup 9 u> 7 and +
  hold(48+)   \ '0' +
;

\ generate all remaining numeric output digits
\ takes unsigned 64-bit on TOS, leaves 64-bit zero
: #s      ( ud -- 0 0 ) \ core	number-sign-s
  begin
    #
    2dup or 0=
  until
;

\ ( D WIDTH -- )   display signed long number D in a field WIDTH characters wide
: d.r
  >r tuck dabs <<# #s rot sign #> r> over - spaces type #>>
;
    
\ (UD WIDTH --)   display unsigned long number UD in a filed WIDTH characters wide
: ud.r >r <<# #s #> r> over - spaces type #>> ;

: .r \ n1 n2 -- ) \ core-ext	dot-r
    \ Display n1 right-aligned in a field n2 characters wide. If more than
    \ n2 characters are needed to display the number, all digits are displayed.
    \ If appropriate, @var{n2} must include a character for a leading ''-''.
    >r s>d r> d.r ;

: u.r \ u n -- )  \ core-ext	u-dot-r
    \ Display @var{u} right-aligned in a field @var{n} characters wide. If more than
    \ @var{n} characters are needed to display the number, all digits are displayed.
    0 swap ud.r ;

\ display unsigned long number in free format followed by space
: d. 0 d.r %bl ;

\ display unsigned long number in free format followed by space
: ud. 0 ud.r %bl ;

\ display signed single number in free format followed by space
: . s>d d. ;

\ display unsigned single number in free format followed by space
: u. 0 ud. ;

base !
