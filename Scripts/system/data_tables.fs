autoforget data_tables

: data_tables ;

\ local variables can't be used inside a builds/does op, so this op does the builds part
\ the top of the stack is the number of ints, followed by the ints themselves
\ the ints on stack are in reverse order of how we want to store them


\ ====================== bytes ======================

: buildByteTable
  here cell pBase!
  dup  allot cell tableSize!
  tableSize 1-  pBase + cell pDst!
  tableSize 0 do
    pDst b! pDst--
  loop
  align
;

: byteTable
  builds
    buildByteTable
  does
    byte[] b@
;

\ ====================== shorts ======================

: buildShortTable
  here -> cell pBase
  dup 2* allot -> cell tableSize
  tableSize 1- 2* pBase + -> cell pDst
  tableSize 0 do
    pDst s! 2 pDst!-
  loop
  align
;

: shortTable
  builds
    buildShortTable
  does
    short[] s@
;

\ ====================== ints ======================

: buildIntTable
  here -> cell pBase
  dup 4* allot -> cell tableSize
  tableSize 1- 4* pBase + -> cell pDst
  tableSize 0 do
    pDst i! 4 pDst!-
  loop
;

: intTable
  builds
    buildIntTable
  does
    int[] i@
;

\ ====================== longs ======================

: buildLongTable
  here int pBase!
  dup 8* allot int tableSize!
  tableSize 1- 8* pBase + int pDst!
  tableSize 0 do
    pDst! 8 pDst!-
  loop
;

: longTable
  builds
    buildLongTable
  does
    long[] @
;

\ ====================== single-precision floats ======================

: sfloatTable
  builds
    buildIntTable
  does
    sfloat[] i@
;

\ ====================== double-precision floats ======================

: buildFloatTable
  here int pBase!
  dup 8* allot int tableSize!
  tableSize 1- 8* pBase + int pDst!
  tableSize 0 do
    pDst 2! 8 pDst!-
  loop
  dstack
;

: floatTable
  builds
    buildFloatTable
  does
    swap 8* + 2@
;

\ ====================== strings ======================

: buildStringTable
  here -> int pBase
  dup -> int tableSize
  4 * allot
  tableSize 1- 4* pBase + -> int pDst
  int pSrc
  \ create the table of string pointers
  tableSize 0 do
    \ dup %s %nl
    pDst ! 4 ->- pDst
  loop
  \ now copy the string data after the string pointer table
  tableSize 0 do
    here dup -> pDst
    pBase @ -> pSrc
    pBase !
    \ pSrc dup %x %bl %s %nl
    allot( strlen( pSrc ) 1+ )
    strcpy( pDst pSrc )
     4 ->+ pBase
  loop
  align
;

: stringTable
  builds
    buildStringTable
  does
    swap 4* + @
;

loaddone

\ tests

r[ 1 2 3 5 7 11 13 ]r intTable primes

r[ "one" "two" "three" ]r stringTable nums
: nn nums %s %nl ;
