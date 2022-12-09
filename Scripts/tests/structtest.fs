
autoforget STRUCTTEST
: STRUCTTEST ;

struct: point
	int x
	int y
;struct

struct: line
	point p1
	point p2
;struct

// test simple assignment
point p

5 -> p.x
8 -> p.y
p.x %d %bl p.y %d %bl %nl

line l1
77 -> l1.p1.x  88 -> l1.p1.y
30 -> l1.p2.x  40 -> l1.p2.y

p %x %nl

p -> ptrTo point pp

pp.x %d %bl pp.y %d %bl %nl

: %point
  -> ptrTo point q
  "x=" %s q.x %d
  " y=" %s q.y %d
  %nl
;

p %point
l1.p1 %point
l1.p2 %point

: %line
  -> ptrTo line r
  "p1: " %s r.p1 %point
  "p2: " %s r.p2 %point
;
l1 %line

struct: rgb32
  byte r
  byte g
  byte b
  byte alpha
  union
  int rgba
;struct

"using global structs:\n" %s
// test global pointer to structs
ptrTo rgb32 gpp
: %grgb32
  -> gpp
  gpp.r %d %bl gpp.g %d %bl gpp.b %d %bl gpp.alpha %d %bl %nl
;

: gsetrgb32
  -> gpp
  -> gpp.alpha -> gpp.b -> gpp.g -> gpp.r
;

// test global arrayOf pointers to structs
here
2 arrayOf ptrTo rgb32 gqq		// 12 bytes - op, 2 pointers
rgb32 go0		// 8 bytes - op, data
rgb32 go1		// 8 bytes - op, data
here swap - "Expected size: 28 Actual size: " %s %d %nl

: testGAPS
  1 2 3 4 go0 gsetrgb32
  5 6 7 8 go1 gsetrgb32
  go0 -> 0 gqq
  go1 -> 1 gqq
  0 gqq %grgb32
  1 gqq %grgb32
  // test assignment through pointers
  77 0 -> gqq.g
  11 1 -> gqq.r
  13 -> go0.b
  0 gqq %grgb32
  1 gqq %grgb32
;

testGAPS

"using local structs:\n" %s
// test local pointer to structs
: %rgb32
  -> ptrTo rgb32 pp
  pp.r %d %bl pp.g %d %bl pp.b %d %bl pp.alpha %d %bl %nl
;

: setrgb32
  -> ptrTo rgb32 pp
  -> pp.alpha -> pp.b -> pp.g -> pp.r
;

// test local arrayOf pointers to structs
: testLAPS
  2 arrayOf ptrTo rgb32 qq
  rgb32 o0
  rgb32 o1
  ptrTo rgb32 qp
  1 2 3 4 o0 setrgb32
  5 6 7 8 o1 setrgb32
  o0 -> 0 qq
  o1 -> 1 qq
  0 qq %rgb32
  1 qq %rgb32
  // test assignment through pointers
  0 qq -> qp
  77 -> qp.g
  1 qq -> qp
  11 -> qp.r
  13 -> o0.b
  0 qq %rgb32
  1 qq %rgb32
;

testLAPS

struct: colorTri
  recursive
  3 arrayOf point p
  rgb32 color
  float size
  double cost
  ptrTo colorTri pNext
;struct

: %colorTri
  -> ptrTo colorTri t
  "p[0] " %s 0 t.p %point
  "p[1] " %s 1 t.p %point
  "p[2] " %s 2 t.p %point
  "RGBA: " %s t.color %rgb32
  "size: " %s t.size %f
  " cost: " %s t.cost %2f %nl
  "pNext: " %s t.pNext %x %nl
;

// SRCTRI DSTTRI ...
: copyColorTri
  sizeOf colorTri move
;

// X Y INDEX TRI ...
: setpt
  -> ptrTo colorTri t
  t.p -> ptrTo point p
  -> p.y -> p.x
;
  
// X0 Y0 X1 Y1 X2 Y2 R G B ALPHA SIZE COST PNEXT TRI ...
: setTri
  -> ptrTo colorTri t
  drop // -> t.pNext
  -> t.cost -> t.size
  t.color setrgb32
  2 t setpt
  1 t setpt
  0 t setpt
;

colorTri u
colorTri v
1 2 3 4 5 6 11 22 33 44 3.1214 2.71828d null u   setTri
u %colorTri


