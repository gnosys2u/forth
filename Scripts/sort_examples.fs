\ example sort algorithms in forth    Pat McElhatton  2017-feb-25
\ sort algorithms are from wikipedia

\ insertion, selection, bubble, shell, comb, quick, heap, merge sorts

autoforget SORT_EXAMPLES

: SORT_EXAMPLES ;

false extParam bool beNoisy   \ set beNoisy to true to display sort array before and after sort
3000 extParam int numElems

mko IntArray v

: fillArray
  IntArray a!o
  if(a.count numElems <>)
    a.resize(numElems)
  endif
  do(a.count 0)
    a.set(rand i)
  loop
;

ucell _time
: startTime
  ms@ _time!
;

: endTime
  if(testsRunning)
    ms@ _time - . " milliseconds elapsed\n" %s
  else
    %nl
  endif
;

: showResult
  IntArray a!o
  if(beNoisy)
    '{' %c
    if(a.count)
      do(a.count 0)
        a.get(i) .
      loop
    endif
    '}' %c  %nl
  endif
;


: isSorted
  IntArray a!o
  true
  do(a.count 1)
    if(a.get(i) a.get(i 1-) <)
      drop false leave
    endif
  loop
;

: checkResult
  IntArray a!o
  do(a.count 1)
    if(a.get(i) a.get(i 1-) <)
      "out of order at " %s i %d %nl
    endif
  loop
;

\ ========================== bubbleSort ==========================

: bubbleSort
  fillArray(v)
  showResult(v)
  startTime
  
  v.count int n!
  begin
    0 -> int newN
    do(n 1)
      if(v.get(i 1-) v.get(i) >)
        v.swap(i 1- i)
        d[ "  swapping " %s i 1- . i . showResult(v) ]d
        i -> newN
      endif
    loop
    newN -> n
  until(n 0=)
  
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== bubble2Sort ==========================

: bubble2Sort
  fillArray(v)
  showResult(v)
  startTime
  
  do(v.count 0)
    if(v.get(i 1+) v.get(i) <)
      v.swap(i 1+ i)
    endif
  +loop(2)
  
  v.count int n!
  do(n 2/ 0)
    n--
    if(v.get(n) v.get(i) <)
      v.swap(n i)
        d[ "  swapping " %s n . i . showResult(v) ]d
    endif
  loop
  
  v.count n!
  int newN
  begin
    newN~
    do(n 1)
      if(v.get(i 1-) v.get(i) >)
        v.swap(i 1- i)
        d[ "  swapping " %s i 1- . i . showResult(v) ]d
        i newN!
      endif
    loop
    newN n!
  until(n 0=)
  
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== shakerSort ==========================

: shakerSort
  fillArray(v)
  showResult(v)
  startTime
  
  true bool swapped!
  int beginIdx
  v.count 1- int endIdx!
  int ii
  begin
  while(and(swapped   beginIdx endIdx <=))
    swapped~
    endIdx int newBeginIdx!
    beginIdx int newEndIdx!
    beginIdx ii!
    d[ "scan: " %s ii . endIdx 1- . %nl ]d
    begin
    while(ii endIdx <)
      d[ ii . ]d
      if(v.get(ii) v.get(ii 1+) >)
        v.swap(ii 1+ ii)
        d[ "  swapping " %s ii . ii 1+ . showResult(v) ]d
        true swapped!
        ii newEndIdx!
      endif
      ii++
    repeat
    if(swapped)
      swapped~
      newEndIdx endIdx!
      endIdx 1- ii!
      d[ "Scan: " %s ii . beginIdx . %nl ]d
      begin
      while(ii beginIdx >=)
        d[ '*' %c ii . ]d
        if(v.get(ii) v.get(ii 1+) >)
          v.swap(ii 1+ ii)
          d[ "  Swapping " %s ii . ii 1+ . showResult(v) ]d
          ii newBeginIdx!
          true swapped!
        endif
        ii--
      repeat
    endif
    newBeginIdx 1+ beginIdx!
  repeat
  
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== gnomeSort ==========================
: gsortInner
  int upperBound!
  IntArray a!o
  d[ "sort upper bound " %s upperBound . %nl ]d
  upperBound int pos!
  begin
    false
    if(pos 0>)
      drop a.get(pos 1-) a.get(pos) >
    endif
  while
    a.swap(pos 1- pos)
    d[ "  swapping " %s pos 1- . pos . showResult(a) ]d
    pos--
  repeat
;  

: optimizedGnomeSort
  ->o IntArray a
  do(a.count 1)
    gsortInner(a i)
  loop
;

: gnomeSort
  fillArray(v)
  showResult(v)
  startTime
  optimizedGnomeSort(v)
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== selectionSort ==========================

: selectionSort
  fillArray(v)
  showResult(v)
  startTime
  int dstVal
  int dstIndex
  do(v.count 1- 0)
    i dstIndex!
    v.get(i) dstVal!
    d[ '[' %c i %d ']' %c %bl ]d
    do(v.count dstIndex 1+)
      v.get(i) int curVal!
      if(curVal dstVal <)
        v.set(dstVal i)
        v.set(curVal dstIndex)
        d[ "index " %s i . "swap " %s curVal . dstVal . showResult(v) ]d
        curVal dstVal!
      endif
    loop
    d[ %nl ]d
  loop
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== insertionSort ==========================

: insertionSort
  fillArray(v)
  showResult(v)
  startTime
  int srcVal
  int srcIndex
  do(v.count 1)
    i srcIndex!
    v.get(i) srcVal!
    d[ '[' %c srcIndex %d ']' %c %bl ]d
    do(srcIndex 0)
      if(v.get(i) srcVal >)
        \ this is not optimized, it is to test insert and remove
        v.remove(srcIndex) -> int tmpVal
        v.insert(tmpVal i)
        d[ "move " %s tmpVal %d " from " %s srcIndex . " to" %s i . showResult(v) ]d
        leave
      endif
    loop
    d[ %nl ]d
  loop
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== combSort ==========================
: combSort
  fillArray(v)
  showResult(v)
  startTime

  v.count int gap!
  1.3ef sfloat shrink!
  false bool sorted!
  
  begin
  while(not(sorted))
    \ Update the gap value for a next comb
    gap i2sf shrink sf/ sf>i gap!
    if(gap 1 >)
      sorted~
    else
      1 gap!
      true sorted!
    endif
    
    \ A single "comb" over the input list
    0 int ii!
    begin
    while(ii gap + v.count <)
      if(v.get(ii) v.get(ii gap +) >)
        v.swap(ii ii gap +)
        d[ "  Swapping " %s ii . ii gap + . showResult(v) ]d
        sorted~
        \ If this assignment never happens within the loop,
        \ then there have been no swaps and the list is sorted.
      endif
      ii++
    repeat
  repeat
  
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== heapSort ==========================
: iParent       1- 2/ ;
: iLeftChild    2* 1+ ;
\ : iRightChild   2* 2+ ;

\ Repair the heap whose root element is at index 'start', assuming the heaps rooted at its children are valid
: siftDown          \ (a, start, end)
  int end!
  int start!
  IntArray a!o
  start int root!
  d[ "siftDown " %s start . end . ]d

  begin
  while(iLeftChild(root) end <=)    \ While the root has at least one child
    iLeftChild(root) int child!   \ Left child of root
    root int swapChild!
    if(a.get(swapChild) a.get(child) <)
      child swapChild!
    endif
    \ If there is a right child and that child is greater
    if(child 1+ end <=)
      if(a.get(swapChild) a.get(child 1+) <)
        child 1+ swapChild!
      endif
    endif
    if(swapChild root =)
        \ The root holds the largest element. Since we assume the heaps rooted at the children are valid, this means that we are done.
      exit
    else
      d[ "  swapping " %s root . swapChild . showResult(v) ]d
      a.swap(root swapChild)
      swapChild root!              \ repeat to continue sifting down the child now
    endif
  repeat
;

\ Put elements of 'a' in heap order, in-place
: heapify
  IntArray a!o
  \ start is assigned the index in 'a' of the last parent node
  \ the last element in a 0-based array is at index count-1; find the parent of that element
  iParent(a.count 1-) int start!
  begin
  while(start 0>=)
    \ sift down the node at index 'start' to the proper place such that all nodes below the start index are in heap order
    siftDown(a start a.count 1-)
    \ go to the next parent node
    start--
  repeat
  \ after sifting down the root all nodes/elements are in heap order)
  d[ "heapify " %s showResult(a) ]d
;
    
: hsort
  \ input: an unordered array a
  IntArray a!o
  \ Build the heap in array a so that largest value is at the root
  heapify(a)

  \ The following loop maintains the invariants that a[0:end] is a heap and every element
  \   beyond end is greater than everything before it (so a[end:count] is in sorted order))
  a.count 1- int end!
  begin
  while(end 0>)
    \ a[0] is the root and largest value. The swap moves it in front of the sorted elements.
    a.swap(end 0)
    d[ "  swapping " %s end . "0 " %s showResult(a) ]d
    \ the heap size is reduced by one
    end--
    \ the swap ruined the heap property, so restore it
    siftDown(a 0 end)
  repeat
;

: heapSort
  fillArray(v)
  showResult(v)
  startTime
  hsort(v)
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== mergeSort ==========================
: copyArray
  int n!
  IntArray dstArray!o
  IntArray srcArray!o
  do(n 0)
    dstArray.set(srcArray.get(i) i)
  loop
;

: bottomUpMerge
  IntArray dstArray!o
  int iEnd!
  int iRight!
  int iLeft!
  IntArray srcArray!o
  iLeft int lIndex!
  iRight int rIndex!
  do(iEnd iLeft)
    false
    if(lIndex iRight <)
      if(rIndex iEnd >=)
        not
      else
        if(srcArray.get(lIndex) srcArray.get(rIndex) <=)
          not
        endif
      endif
    endif
    
    if
      dstArray.set(srcArray.get(lIndex) i)
      lIndex++
    else
      dstArray.set(srcArray.get(rIndex) i)
      rIndex++
    endif
  loop
;

: bottomUpMergeSort
  int n!
  IntArray srcArray!o
  
  mko IntArray tmpArray
  tmpArray.resize(n)
  
  
  \ Each 1-element run in srcArray is already "sorted".
  \ Make successively longer sorted runs of length 2, 4, 8, 16... until whole array is sorted.
  1 int width!
  int ii
  begin
  while(width n <)
    ii~
    begin
    while(ii n <)
      bottomUpMerge(srcArray ii min(width ii@+ n) min(width 2* ii@+ n) tmpArray)
      width 2* ii!+
    repeat
    copyArray(tmpArray srcArray n)
    
    width 2* width!
  repeat
  oclear tmpArray
;

: mergeSort
  fillArray(v)
  showResult(v)
  startTime
  bottomUpMergeSort(v v.count)
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== shellSort ==========================

1 arrayOf int gaps
701 0 gaps! 301 i, 132 i, 57 i, 23 i, 10 i, 4 i, 1 i, 0 i,

: shellSort
  fillArray(v)
  showResult(v)
  startTime
  
  int gapIndex
  v.count int n!
  begin
    gaps(gapIndex) int gap!
  while(gap)
    if(gap n <)
      do(n gap)
        v.get(i) int temp!
        i int jj!
        begin
          false
          if(jj gap >=)
            drop v.get(jj gap -) temp >
          endif
        while
          v.set(v.get(jj gap -) jj)
          gap jj!-
        repeat
        v.set(temp jj)
        d[ "  swapping " %s i . jj . showResult(v) ]d
      loop
    endif
    gapIndex++
  repeat
  
  endTime
  showResult(v)
  checkResult(v)
;

\ ========================== quickSort ==========================

: qsPartition  \ hi lo IntArray
  int hi!
  int lo!
  IntArray a!o
  d[ "partition " %s lo . hi . %nl ]d
  a.get(lo) int pivot!
  lo 1- int loIndex!
  hi 1+ int hiIndex!
  begin
    begin
      loIndex++
    until(a.get(loIndex) pivot >=)

    begin
      hiIndex--
    until(a.get(hiIndex) pivot <=)
    
    if(loIndex hiIndex >=)
      hiIndex
      exit
    endif
    
    a.swap(loIndex hiIndex)
    d[ "  swapping " %s loIndex . hiIndex . showResult(v) ]d
  again
;

: qs
  recursive
  int hi!
  int lo!
  IntArray a!o
  int p
  if(lo hi <)
    qsPartition(a lo hi) p!
    qs(a lo p)
    qs(a p 1+ hi)
  endif
;

: quickSort
  fillArray(v)
  showResult(v)
  startTime
  qs(v 0 v.count 1-)
  endTime
  showResult(v)
  checkResult(v)
;


\ ========================== builtinQuickSort ==========================
: builtinQuickSort
  \ qsort( ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
  fillArray(v)
  showResult(v)
  startTime
  qsort(v.base v.count 4 4 0)
  endTime
  showResult(v)
  checkResult(v)
;

: go
  "for " %s numElems . "elements:\n" %s
  "bubbleSort " %s bubbleSort
  \ "bubble2Sort " %s bubble2Sort   \ boring variation on bubble
  "shakerSort " %s shakerSort
  "gnomeSort " %s gnomeSort
  "selectionSort " %s selectionSort
  "insertionSort " %s insertionSort
  "combSort " %s combSort
  "heapSort " %s heapSort
  "mergeSort " %s mergeSort
  "shellSort " %s shellSort
  "quickSort " %s quickSort
  "builtinQuickSort " %s builtinQuickSort
;

"type testSorts to test multiple algorithms for " %s numElems . "elements\n" %s
"set numElems to change number of elements\n" %s
"set beNoisy to true to display sort array before and after sort\n" %s
"or run individual tests with ops: insertionSort selectionSort bubbleSort shellSort combSort quickSort heapSort mergeSort\n" %s
"rebuild with compileDebug true to get extremely verbose output\n" %s

loaddone

\ ========================== 2stackSort ==========================

: peek
  ->o IntArray v
  v.get(v.count 1-)
;
  
: 2stackSorta
  fillArray(v)
  
  v.clone IntArray a!
  showResult(a)
  mko IntArray b
  
  startTime
  bool done
  \ numElems 2+ int nc!
  0 int numPasses!
  begin
  while(not(done))

    true done!
    \ b is empty
    b.push(a.pop)
    begin
    while(a.count)
      if(peek(a) peek(b) >)
        d[ "swapping " %s peek(a) . peek(b) . %nl ]d
        b.pop a.pop b.push b.push
        done~
      else 
        a.pop b.push
      endif
    repeat
    \ showResult(b)
    
    \ a is empty
    a.push(b.pop)
    begin
    while(b.count)
      if(peek(b) peek(a) <)
        d[ "Swapping " %s peek(a) . peek(b) . %nl ]d
        a.pop b.pop a.push a.push
        done~
      else 
        b.pop a.push
      endif
    repeat
    \ showResult(a)
    
    done numPasses numElems dup 2* * >= or -> done
    numPasses++

  repeat
  
  endTime
  numPasses . " passes for " %s numElems . %nl

  if(or(not(isSorted(a))   numPasses numElems >=))
    beNoisy
    true beNoisy!
    showResult(v)
    showResult(a)
    beNoisy!
  endif
  
  \ showResult(b)
  checkResult(a)
  oclear a
  oclear b
;

: 2stackSort
  fillArray(v)
  
  v.clone IntArray a!
  showResult(a)
  mko IntArray b
  
  startTime
  bool done
\  0 -> int numPasses
  begin
  while(a.count)

    a.pop int val!
    
    begin
      false
      if(b.count)
        val peek(b) > or
      endif
    while
      a.push(b.pop)
    repeat

    b.push(val)
  repeat
  
  begin
  while(b.count)
    a.push(b.pop)
  repeat
  
  showResult(a)
  
  endTime

  
  \ showResult(b)
  checkResult(a)
  oclear a
  oclear b
;

: ss 1000 numElems! 2stackSort 2stackSorta ;
: dd 20 0 do ss loop ss ;

