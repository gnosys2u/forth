\ test of refcounts being corrupted when objects are used by multiple threads

autoforget HAMMER
: HAMMER ;

2 -> int numThreads
\ $FFFFFF constant timesToHammer
1000000 -> cell timesToHammer
1000000 -> cell refCountOffset

: %thread `[` %c thisThread %x "] " %s ;

\  common core for object refcount hammer tests
: hammerer
  -> cell hammerOp
  -> ptrTo byte msg
  mko Object target
  Thread thread

  refCountOffset ->+ target.__refCount
  
  ">>>>>>>> " %s msg %s %nl
  ms@ \ "start\n" %s
  
  if(numThreads 1 >)
    system.createThread(
      f: execute thisThread.exit ;f
      1000 1000) -> thread
    thread.startWithArgs(r[ target hammerOp ]r) drop
  endif

  target hammerOp execute

  if(numThreads 1 >)
    thread.join
    oclear thread
  endif

  refCountOffset ->- target.__refCount
  "end count (should be 1): " %s target.__refCount %d %nl
  1 -> target.__refCount
  oclear target
  
  ms@ swap - %d " milliseconds\n" %s
  "<<<<<<<< " %s msg %s %nl %nl
;

: plusStoreHammer
  -> Object target
  ref target.__refCount -> ptrTo cell pt

  do(timesToHammer 0)
    1 pt +!      1 pt +!      1 pt +!      1 pt +!
    1 pt +!      1 pt +!      1 pt +!      1 pt +!
    1 pt +!      1 pt +!      1 pt +!      1 pt +!
    1 pt +!      1 pt +!      1 pt +!      1 pt +!
    -1 pt +!     -1 pt +!     -1 pt +!     -1 pt +!
    -1 pt +!     -1 pt +!     -1 pt +!     -1 pt +!
    -1 pt +!     -1 pt +!     -1 pt +!     -1 pt +!
    -1 pt +!     -1 pt +!     -1 pt +!     -1 pt +!
  loop

  oclear target
;    

: atomicPlusStoreHammer
  -> Object target
  ref target.__refCount -> ptrTo cell pt
  
  do(timesToHammer 0)
    1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop
    1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop
    1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop
    1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop   1 pt +!@ drop
    -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop
    -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop
    -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop
    -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop  -1 pt +!@ drop
  loop

  oclear target
;    

: plusStoreTest
  "test non-atomic +!" lit plusStoreHammer hammerer
  "NOTE: count not 1 for +! test is expected result\n" %s
  "test atomic +!@" lit atomicPlusStoreHammer hammerer
;

: keepHammer
  -> Object target
  \ %thread target.__refCount %d %nl

  do(timesToHammer 0)
    target.keep     target.keep     target.keep     target.keep
    target.release  target.release  target.release  target.release
    target.keep     target.keep     target.keep     target.keep
    target.release  target.release  target.release  target.release
  loop
    
  oclear target
;    

: keepTest
  "test atomic refcounting using Object.keep and Object.release"
  lit keepHammer hammerer
;

: pushHammer
  -> Object target
  mko Array arr
  do(timesToHammer 0)
    target arr.push  target arr.push  target arr.push  target arr.push
    target arr.push  target arr.push  target arr.push  target arr.push
    target arr.push  target arr.push  target arr.push  target arr.push
    target arr.push  target arr.push  target arr.push  target arr.push
    arr.clear
  loop
  
  oclear arr
  oclear target
;    

: pushTest
  "test atomic refcounting using Array.push"
  lit pushHammer hammerer
;

: objStoreHammer
  -> Object target
  \ %thread target.__refCount %d %nl
  Object aa  Object bb  Object cc  Object dd
  mko Object moo
  1000000 ->+ moo.__refCount
  \ "moo\n" %s moo.show
  
  do(timesToHammer 0)
    target -> aa  moo -> aa  target -> aa  moo -> aa
    target -> aa  moo -> aa  target -> aa  moo -> aa
    target -> aa  moo -> aa  target -> aa  moo -> aa
    target -> aa  moo -> aa  target -> aa  moo -> aa
    target -> aa  target -> bb  target -> cc  target -> dd
    oclear aa     oclear bb     oclear cc     oclear dd
    target -> aa  target -> bb  target -> cc  target -> dd
    oclear aa     oclear bb     oclear cc     oclear dd
  loop

  oclear target
  1 -> moo.__refCount
  oclear moo
;    

: storeTest
  "test atomic refcounting using local obect store"
  lit objStoreHammer hammerer
;

: test
  plusStoreTest keepTest pushTest storeTest
;

"\+
test            run all the tests below\n\+
plusStoreTest   test +! (non-atomic) and +!@ (atomic)\n\+
keepTest        test Object.keep and Object.release\n\+
pushTest        test Array.push\n\+
storeTest       test storing to local object variables\n" %s

loaddone

