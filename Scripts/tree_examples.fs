requires numberio
requires randoms

autoforget TREE_EXAMPLES
: TREE_EXAMPLES ;

false -> bool beNoisy   \ set beNoisy to true to display sort array before and after sort

setRandomSeed(ms@)
: smallRandom
  random 32767 and
;

mko IntArray elemSpacing
r[ 56   26 55   12 25 25 25  4 10 10 10 10 10 10 10   1 1 4 1 4 1 4 1 4 1 4 1 4 1 4 1 4 ]r
elemSpacing.load

\ ========================================= Node =========================================

class: Node
  int val
  Node left
  Node right
  Node parent

  m: delete
    \ "deleting " %s val . %nl
    left~    right~
    \ we don't need to do anything with our parent, since if they still referenced us, we wouldn't be deleted
  ;m
  
  m: compare
    Node n!o
    icmp(val n.val)
  ;m
  
  m: setLeft
    if(left objNotNull)
      null left.parent!o
    endif
    left!
    if(left objNotNull)
      this left.parent!o
    endif
  ;m
  
  m: setRight
    if(right objNotNull)
      null right.parent!o
    endif
    right!
    if(right objNotNull)
      this right.parent!
    endif
  ;m
  
  m: count
    recursive
    1 int numNodes!
    if(left objNotNull)
      left.count numNodes!+
    endif
    if(right objNotNull)
      right.count numNodes!+
    endif
    numNodes
  ;m
  
  m: swapValue
    Node n!o
    n.val val n.val! val!
  ;m
  
  m: clear
    recursive
    if(left objNotNull)
      left.clear
      left~
    endif
    if(right objNotNull)
      right.clear
      right~
    endif
    null parent!o
  ;m
  
  m: height
    recursive
    0 -> int leftHeight
    0 -> int rightHeight
    if(left objNotNull)
      left.height -> leftHeight
    endif
    if(right objNotNull)
      right.height -> rightHeight
    endif
    max(leftHeight rightHeight) 1+
  ;m

  m: _adoptChildren
    if(left objNotNull)
      this left.parent!o
    endif
    if(right objNotNull)
      this right.parent!o
    endif
  ;m

  m: checkLinks
    recursive
    if(left objNotNull)
      if(left.parent this =)
        left.checkLinks
      else
        "Node.checkLinks: node with value " val . "has inconsistent left link\n" %s
      endif
    endif
    if(right objNotNull)
      if(right.parent this =)
        right.checkLinks
      else
        "Node.checkLinks: node with value " val . "has inconsistent right link\n" %s
      endif
    endif
  ;m
  
;class

\ ========================================= Node2 =========================================

\ Node2 is just a test of sort/compare - reverse order of compare to reverse sort order
class: Node2 extends Node
  
  m: compare
    Node b!o
    icmp(b.val val)
  ;m
  
;class

\ ========================================= Tree =========================================

class: Tree
  Node root
  int numNodes

  m: delete
    root~
  ;m
  
  m: setRoot
    root!
    if(root objNotNull)
      null root.parent!o
      root.count numNodes!
    endif
  ;m
  
  m: clear
    if(root objNotNull)
      root.clear
      root~
    endif
    numNodes~
  ;m
  
  m: count
    root.count
  ;m
  
  m: _traverseOp
    Node n!o
    n.val .
  ;m
    
  m: _traverseInOrder
    recursive
    Node n!o
    if(n.left objNotNull)
      _traverseInOrder(n.left)
    endif
    _traverseOp(n)
    if(n.right objNotNull)
      _traverseInOrder(n.right)
    endif
  ;m
  
  m: _traversePreOrder
    recursive
    Node n!o
    _traverseOp(n)
    if(n.left objNotNull)
      _traversePreOrder(n.left)
    endif
    if(n.right objNotNull)
      _traversePreOrder(n.right)
    endif
  ;m
  
  m: _traversePostOrder
    recursive
    Node n!o
    if(n.left objNotNull)
      _traversePostOrder(n.left)
    endif
    if(n.right objNotNull)
      _traversePostOrder(n.right)
    endif
    _traverseOp(n)
  ;m
  
  m: traverseInOrder
    _traverseInOrder(root)
  ;m
  
  m: traversePreOrder
    _traversePreOrder(root)
  ;m
  
  m: traversePostOrder
    _traversePostOrder(root)
  ;m

  m: checkLinks
    root.checkLinks
  ;m
  
  m: _insert
    recursive
    Node newNode!o
    Node n!o
    if(newNode.val n.val >)
      if(n.right objNotNull)
        _insert(n.right newNode)
      else
        n.setRight(newNode)
      endif
    else
      if(n.left objNotNull)
        _insert(n.left newNode)
      else
        n.setLeft(newNode)
      endif
    endif
  ;m
  
  m: insert
    Node n!o
    if(root objNotNull)
      _insert(root n)
      numNodes++
    else
      setRoot(n)
    endif
  ;m
  
  m: addLeft
    Node n!o
    \ "addLeft " %s dup . %nl
    if(n.left objIsNull)
      numNodes++
    endif
    n.setLeft(new Node)
    n.left.val!
  ;m
  
  m: addRight
    Node n!o
    \ "addRight " %s dup . %nl
    if(n.right objIsNull)
      numNodes++
    endif
    n.setRight(new Node)
    n.right.val!
  ;m
  
  m: display
    if(root objIsNull)
      "tree is empty\n" %s
      exit
    endif
    
    "height: " %s root.height .  " #nodes: " %s numNodes . %nl
    int elementNum
    mko List q
    mko List otherQ
    q.addTail(root)
    true bool keepGoing!
    false bool minSpacing!
    begin
    while(keepGoing)
      keepGoing~
      begin
      while(not(q.isEmpty))
        if(elementNum elemSpacing.count <)
          spaces(elemSpacing.get(elementNum))
        else
          %bl
          true minSpacing!
        endif
        elementNum++
        q.unrefHead Node n!
        if(n objNotNull)
          if(n.left objNotNull n.right objNotNull or)
            true keepGoing!
          endif
          otherQ.addTail(n.left)
          otherQ.addTail(n.right)
          n.val 5 .r
        else
          if(minSpacing) " * " else "  *  " endif %s
          otherQ.addTail(null)
          otherQ.addTail(null)
        endif
      repeat
      otherQ q otherQ!o q!o   \ swap q and otherQ
      %nl
    repeat
  ;m
  
  m: _toArray
    recursive
    Array a!o
    Node n!o
    if(n.left objNotNull)
      _toArray(n.left a)
    endif
    a.push(n)
    if(n.right objNotNull)
      _toArray(n.right a)
    endif
  ;m
  
  m: toArray
    mko Array a
    _toArray(root a)
    a@~
  ;m

  m: _fixParentLinks
    recursive
    Node n!o
    n.parent!o
    if(n.left objNotNull)
      _fixParentLinks(n n.left)
    endif
    if(n.right objNotNull)
      _fixParentLinks(n n.right)
    endif
  ;m

  m: _fromArray
    int right!
    int left!
    Array a!o
    d[ "_fromArray " %s left . right . %nl ]d
    recursive
    right left - 1+ int nodeCount!
    Node branchRoot
    case(nodeCount)
      of(1)
        a.get(left) branchRoot!o
      endof
      
      of(2)
        a.get(right) branchRoot!o
        branchRoot.setLeft(a.get(left))
      endof
      
      of(3)
        a.get(left 1+) branchRoot!o
        branchRoot.setLeft(a.get(left))
        branchRoot.setRight(a.get(right))
      endof
      
      \ default case
      dup 2/ left@+ int middle!
      a.get(middle) branchRoot!o
      branchRoot.setLeft(_fromArray(a left middle 1-))
      branchRoot.setRight(_fromArray(a middle 1+ right))
      
    endcase
    
    branchRoot
  ;m
  
  m: fromArray
    Array a!o
    clear
    _fromArray(a 0 a.count 1-) root!
    _fixParentLinks(null root)
    a.count numNodes!
  ;m

  m: makeBST
    toArray Array a!
    a.sort
    fromArray(a)
    a~
  ;m

  m: _makeHeap
    Array a!o
    clear
    setRoot(a.get(0))
    
    a.count int nodeCount!
    1 int ix!
    do(nodeCount 0)
      a.get(i) Node n!o
      if(ix nodeCount >=)
        leave
      endif
      n.setLeft(a.get(ix))
      ix++
      if(ix nodeCount >=)
        leave
      endif
      n.setRight(a.get(ix))
      ix++
    loop
  ;m

  m: makeMinHeap
    toArray Array a!

    if(a.count 2 <)
      exit
    endif
    
    a.sort
    _makeHeap(a)
    a.count numNodes!
    a~
  ;m
  
  m: makeMaxHeap
    toArray Array a!

    if(a.count 2 <)
      exit
    endif
    
    a.sort
    a.reverse  \ reversed sort order is only difference from minHeap
    _makeHeap(a)
    a.count numNodes!
    a~
  ;m

  m: swapNodes
    \ this is a major pain in the ass because of the parent links
    \ this would make more sense if the Node's data payload was more than just an int
    Node a!o
    Node b!o
    true bool aIsRoot!
    true bool bIsRoot!
    
    a.right b.right a.right!o b.right!o
    a.left b.left a.left!o b.left!o
    a.parent b.parent a.parent!o b.parent!o

    \ fix childrens parent links
    a._adoptChildren
    b._adoptChildren
    
    \ fix parent of a
    a.parent Node aParent!o
    if(aParent objNotNull)
      aIsRoot~
      if(aParent.left b =)
        a aParent.left!o
      else
        if(aParent.right b =)
          a aParent.right!o
        else
          "Tree.swapHard: b reparent failure!\n" %s
        endif
      endif
    endif
    
    \ fix parent of b
    b.parent Node bParent!o
    if(bParent objNotNull)
      bIsRoot~
      if(bParent.left a =)
        b bParent.left!o
      else
        if(bParent.right a =)
          b bParent.right!o
        else
          "Tree.swapHard: a reparent failure!\n" %s
        endif
      endif
    endif
    
    if(aIsRoot)
      a root!o
    endif
    if(bIsRoot)
      b root!o
    endif
  ;m
  
  m: swapValues
    Node a!o
    Node b!o
    d[ "swapValues " %s a.val . b.val . %nl ]d
    a.val b.val a.val! b.val!
  ;m

  \ this assumes tree has all levels except bottom completely filled,
  \  bottom level is filled left-to-right
  m: insertInNextHeapSlot
    Node n!o
    if(root objNotNull)
      \ a relatively simple way of determining the left-right path to the rightmost node on
      \  the bottom (incomplete) layer of the tree - take the bits below the top set bit of the (numNodes + 1)
      \ here we stick them on the stack starting at the low bit to do the reversing
      \ this is a terrible way of reversing the bits, but it is fine for the trees we are dealing with (depth < 8 or so)
      numNodes 1+ int branchChoices!
      int numChoices
      root Node insertPoint!o
      begin
      while(branchChoices 1 >)
        branchChoices 1 and
        rshift(branchChoices 1) branchChoices!
        numChoices++
      repeat
      begin
      while(numChoices 1 >)
        if
          insertPoint.right insertPoint!o
        else
          insertPoint.left insertPoint!o
        endif
        numChoices--
      repeat
      if
        insertPoint.setRight(n)
      else
        insertPoint.setLeft(n)
      endif
      
      numNodes++
    else
      setRoot(n)
    endif
  ;m
  
  m: insertInMinHeap
    Node n!o
    insertInNextHeapSlot(n)
    begin
    while(n.parent objNotNull)
      if(n.val n.parent.val <)
        swapValues(n n.parent)
        n.parent n!o
      else
        exit
      endif
    repeat
  ;m
    
  m: insertInMaxHeap
    Node n!o
    insertInNextHeapSlot(n)
    begin
    while(n.parent objNotNull)
      if(n.val n.parent.val >)
        swapValues(n n.parent)
        n.parent n!
      else
        exit
      endif
    repeat
  ;m
  
  m: remove
    Node n!o
    
    d[ "Tree.removeNode - node with value " %s n.val . %nl ]d
    if(n.parent objNotNull)
      if(n.left objNotNull)
        n.left.count numNodes!-
        n.left~
      endif
      if(n.right objNotNull)
        n.right.count numNodes!-
        n.right~
      endif
      numNodes--
      if(n.parent.left n =)
        n.parent.left~
      else
        if(n.parent.right n =)
          n.parent.right~
        else
          "Tree.removeNode - node with value " %s n.val %d " has bad parent link\n" %s
        endif
      endif
      null n.parent!o
    else
      \ removed node is root, clear tree
      clear
    endif
  ;m

  \ this assumes tree has all levels except bottom completely filled,
  \  bottom level is filled left-to-right
  m: findBottomHeapSlot
    int branchChoices
    if(root objNotNull)
      \ a relatively simple way of determining the left-right path to the rightmost node on
      \  the bottom (incomplete) layer of the tree - take the bits below the top set bit of the (numNodes + 1)
      \ here we stick them on the stack starting at the low bit to do the reversing
      \ this is a terrible way of reversing the bits, but it is fine for the trees we are dealing with (depth < 8 or so)
      numNodes branchChoices!
      int numChoices
      root Node bottomSlot!
      begin
      while(branchChoices 1 >)
        branchChoices 1 and
        rshift(branchChoices 1) branchChoices!
        numChoices++
      repeat
      begin
      while(numChoices 0>)
        if
          bottomSlot.right bottomSlot!o
        else
          bottomSlot.left bottomSlot!o
        endif
        numChoices--
      repeat
      bottomSlot
      
    else
      null
    endif
  ;m
  
  m: _extractHeapTopValue
    \ overall idea:
    \  return value in top slot
    \  stuff value from right-bottommost slot into top slot
    \  remove right-bottommost slot
    \  swap value from top-slot down into its child with the smaller value until value reaches bottom level,
    \    reestablishing that this is a min heap
    op compareOp!
    if(root objNotNull)
      root.val
      findBottomHeapSlot Node bottomSlot!o
      if(bottomSlot objNotNull)
        \ replace value in bottomMost heap element in top slot of heap
        bottomSlot.val dup root.val!
        int movingValue!
        remove(bottomSlot)
        root Node movingSlot!o
        Node nextMovingSlot
        begin
        while(movingSlot objNotNull)
          null nextMovingSlot!o
          if(movingSlot.left objNotNull)
            if(movingSlot.right objNotNull)
              \ node has left & right children
              if(compareOp(movingSlot.right.val movingSlot.left.val))
                if(compareOp(movingValue movingSlot.left.val))
                  movingSlot.left nextMovingSlot!o
                endif
              else
                if(compareOp(movingValue movingSlot.right.val))
                  movingSlot.right nextMovingSlot!o
                endif
              endif
            else
              \ there is only a left child
              if(compareOp(movingValue movingSlot.left.val))
                movingSlot.left nextMovingSlot!o
              endif
            endif
          else
            \ there is no left child
            if(movingSlot.right objNotNull)
              if(compareOp(movingValue movingSlot.right.val))
                movingSlot.right nextMovingSlot!o
              endif
            endif
          endif
          if(nextMovingSlot objNotNull)
            swapValues(movingSlot nextMovingSlot)
          endif
          nextMovingSlot movingSlot!o
        repeat
      else
        "Tree.extractMinHeapValue: findBottomHeapSlot returned null!\n" %s
      endif
      
    else
      "Tree.extractMinHeapValue: tree empty!\n" %s
      0
    endif
  ;m
  
  m: extractMinHeapValue
    _extractHeapTopValue(['] >)
  ;m  
  
  m: extractMaxHeapValue
    _extractHeapTopValue(['] <)
  ;m  
  
  \ create a binary tree, not sorted, not necessarily balanced
  m: fillRandom    \ rootNode numNodes ...
    1- int nodeCount!
    setRoot(new Node)
    mko List q
    q.addHead(root)
    "seed: " %s getRandomSeed . %nl
    smallRandom root.val!
    begin
    while(nodeCount 0>)
      q.unrefTail Node n!
      mod(smallRandom 100) int diceRoll!
      75 diceRoll!-
      if(diceRoll 0<)
        addLeft(smallRandom n)
        addRight(smallRandom n)
        q.addHead(n.left)
        q.addHead(n.right)
        2 nodeCount!-
      else
        12 diceRoll!-
        if(diceRoll 0<)
          addLeft(smallRandom n)
          q.addHead(n.left)
        else
          addRight(smallRandom n)
          q.addHead(n.right)
        endif
        nodeCount--
      endif
      n~
    repeat
    %nl
    q~
    count numNodes!
  ;m

  \ create a sorted binary tree, not necessarily balanced
  m: fillSorted    \ numNodes ...
    1- int nodeCount!
    setRoot(new Node)
    "seed: " %s getRandomSeed . %nl
    smallRandom root.val!
    begin
    while(nodeCount 0>)
      new Node Node n!
      smallRandom n.val!
      insert(n)
      n~
      nodeCount--
    repeat
    count numNodes!
  ;m

;class

mko Tree t



\ create an unsorted binary tree, not necessarily balanced but close
: fillTree    \ rootNode numNodes ...
  1- int numNodes!
  t.root Node root!o
  root.clear
  mko List q
  q.addHead(root)
  "seed: " %s getRandomSeed . %nl
  smallRandom root.val!
  d[ root.val . ]d
  begin
  while(numNodes 0>)
    q.unrefTail Node n!
    t.addLeft(smallRandom n)
    t.addRight(smallRandom n)
    q.addHead(n.left)
    q.addHead(n.right)
    d[ n.left.val . n.right.val . ]d
    n~
    2 numNodes!-
  repeat
  %nl
  q~
;

: tt1
  t.clear
  Node n
  0 do
    new Node n!
    smallRandom n.val!
    t.insertInMinHeap(n)
    n~
  loop
  t.display
;

: tt2
  int numElems!
  mko Array a
  
  do(numElems 0)
    new Node Node n!
    smallRandom n.val!
    a.push(n)
    n~
  loop
  
  do(numElems 0)
    a.get(i) n!o
    i . n.val . %nl
  loop
  
  a.sort
  
  do(numElems 0)
    a.get(i) n!o
    i . n.val . %nl
  loop
  
  oclear a
;

: tt3
  0 do
    t.extractMinHeapValue .
  loop
;

: tt4
  0 do
    t.extractMaxHeapValue .
  loop
;

: tt5  \ list nodes at depth
  mko List qA
  mko List qB
  Node n
  
  qA.addTail(t.root)
  begin
  while(not(qA.isEmpty))
    \ print qA nodes
    qA.headIter ListIter iter!
    begin
    while(iter.next)
      <Node>.val .
    repeat
    oclear iter
    %nl
    null n!o
    
    \ add children of qA to qB
    begin
    while(not(qA.isEmpty))
      qA.unrefHead n!
      if(n.left objNotNull)
        qB.addTail(n.left)
      endif
      if(n.right objNotNull)
        qB.addTail(n.right)
      endif
    repeat
    n~
    
    \ swap qA and qB
    qA qB qA!o qB!o
    
  repeat
  qA~
  qB~
;

: go
  20 tt1 20 tt3
  20 tt2
;

loaddone
