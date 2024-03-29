autoforget classtest
: classtest ;

class: ook
  
  int num

  32 string name

  m: ouch
    "ouch!" %s
  ;m

  m: bowwow
    "woof!" %s
  ;m

  m: setstr
    -> name
  ;m

  m: getstr
    returns ptrTo string
    ref name
  ;m
  
  m: boohoo
    "my name is " %s name %s
  ;m

;class



class: kool
  extends ook

  m: zvooba
    boohoo
    " and my number is " %s num %d
  ;m

  m: init
    initMemberString name
    11 -> num
  ;m
  
;class

new kool -> kool kk
kk.init
kk.bowwow
kk.boohoo
43 -> kk.num

"I am kk" kk.setstr

: kookoo
  -> ptrTo kool pp
  pp.zvooba
;

ref kk kookoo

struct: sRGBA
  byte r
  byte g
  byte b
  byte alpha
  union
  int RGBA
;struct

class: rgba
  sRGBA s
  int num
  
  m: set
	  -> s.alpha -> s.b -> s.g -> s.r
  ;m
  
  m: add
    \ TOS is object to add
    -> rgba t
    t.s.r ->+ s.r
    t.s.g ->+ s.g
    t.s.b ->+ s.b
    t.s.alpha ->+ s.alpha
    oclear t
  ;m
  
;class


"\n==================\nusing global objects:\n" %s

\ test global pointer to structs
ptrTo rgba gpp
: %grgb32
  -> gpp
  gpp.s.r . %bl gpp.s.g . %bl gpp.s.b . %bl gpp.s.alpha . %nl
;

: gsetrgb32
  -> gpp
  -> gpp.s.alpha -> gpp.s.b -> gpp.s.g -> gpp.s.r
;

\ test global arrayOf pointers to objects
here
2 arrayOf ptrTo rgba gapo		\ expected size 12: gapo opcode (4) + pointers (2 x 4)
new rgba -> rgba go0			\ expected size 12: opcode (4) + object reference (8)
new rgba -> rgba go1			\ expected size 12: opcode (4) + object reference (8)
128 -> go0.num
256 -> go1.num

here swap - "Expected size: 36 Actual size: " %s %d %nl

2 arrayOf rgba ago
: ll go0.s.g gpp.s.g 1 ago.s.g 1 gapo.s.g ;
: ll2 rgba hh ptrTo rgba phh 4 arrayOf rgba ahh 4 arrayOf ptrTo rgba pahh   hh.s.g phh.s.g 1 ahh.s.g 1 pahh.s.g ;

: testGAPO
  71 2 3 4 ref go0 gsetrgb32
  5 6 7 8 ref go1 gsetrgb32
  ref go0 -> 0 gapo
  ref go1 -> 1 gapo
  0 gapo %grgb32
  1 gapo %grgb32
  \ test assignment through pointers
  22 0 -> gapo.s.g
  55 1 -> gapo.s.r
  33 -> go0.s.b
  10 ->+( 0 ) gapo.s.r
  0 gapo.show
  1 gapo.show
  go1 0 gapo.add
  0 gapo.show
;

testGAPO

"\n==================\nusing local objects:\n" %s
\ test local pointer to structs
: %rgba
  -> ptrTo rgba pp
  pp.s.r %d %bl pp.s.g %d %bl pp.s.b %d %bl pp.s.alpha %d %bl %nl
;

: setrgb32
  -> ptrTo rgba pp
  -> pp.s.alpha -> pp.s.b -> pp.s.g -> pp.s.r
;

\ test local arrayOf pointers to objects
: testLAPO
  2 arrayOf ptrTo rgba lapo
  new rgba -> rgba lo0
  new rgba -> rgba lo1
  41 2 3 4 ref lo0 setrgb32
  5 6 7 8 ref lo1 setrgb32
  ref lo0 -> 0 lapo
  ref lo1 -> 1 lapo
  0 lapo %rgba
  1 lapo %rgba
  \ test assignment through pointers
  22 ->( 0 ) lapo.s.g
  55 ->( 1 ) lapo.s.r
  33 -> lo0.s.b
  10 ->+( 0 ) lapo.s.r
  
  0 lapo.show
  1 lapo.show
  lo1 0 lapo.add
  lo0.show
  
  oclear lo0
  oclear lo1
;

testLAPO


class: urg
  extends rgba
  
  rgba partner
  
  m: boo
    returns rgba
    partner
  ;m

;class

new rgba -> rgba mm

new urg -> urg nn

mm -> nn.partner

11 2 3 4 mm.set
mm.show

: krunk nn.partner.show ;
: klunk nn.boo.show ;

krunk
klunk

100 -> nn.s.r

class: fakeRgba
  int rrr
  int ggg
  int bbb
  int aaa

  m: blah
  	"blah!\n" %s
  ;m
    
  implements: rgba
  
  m: set
	  -> aaa -> bbb -> ggg -> rrr
  ;m
  
  m: show
	  rrr %d %bl ggg %d %bl bbb %d %bl aaa %d "!!!" %s %nl
  ;m
  
  m: add
    \ TOS is object to add
    -> rgba t
    t.s.r ->+ rrr
    t.s.g ->+ ggg
    t.s.b ->+ bbb
    t.s.alpha ->+ aaa
  ;m

  ;implements

;class


: cleanup
  oclear kk
  oclear go0
  oclear go1
  oclear mm
  oclear nn.partner
  oclear nn
;

loaddone

