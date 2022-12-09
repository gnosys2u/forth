requires numberio
requires randoms

autoforget TREE_EXAMPLES
: TREE_EXAMPLES ;

false -> bool beNoisy   // set beNoisy to true to display sort array before and after sort

setRandomSeed(ms@)
: smallRandom
  random 32767 and
;

mko IntArray elemSpacing
r[ 56   26 55   12 25 25 25  4 10 10 10 10 10 10 10   1 1 4 1 4 1 4 1 4 1 4 1 4 1 4 1 4 ]r
elemSpacing.load

//========================================= Node =========================================

class: Node
  int val
  Node left
  Node right
  Node parent

  m: delete
    // "deleting " %s val . %nl
    oclear left
    oclear right
    // we don't need to do anything with our parent, since if they still referenced us, we wouldn't be deleted
  ;m
  
  m: compare
    ->o Node n
    icmp(val n.val)
  ;m
  
  m: setLeft
    if(left objNotNull)
      null ->o left.parent
    endif
    -> left
    if(left objNotNull)
      this ->o left.parent
    endif
  ;m
  
  m: setRight
    if(right objNotNull)
      null ->o right.parent
    endif
    -> right
    if(right objNotNull)
      this ->o right.parent
    endif
  ;m
  
  m: count
    recursive
    1 -> int numNodes
    if(left objNotNull)
      left.count ->+ numNodes
    endif
    if(right objNotNull)
      right.count ->+ numNodes
    endif
    numNodes
  ;m
  
  m: swapValue
    ->o Node n
    n.val val -> n.val -> val
  ;m
  
  m: clear
    recursive
    if(left objNotNull)
      left.clear
      oclear left
    endif
    if(right objNotNull)
      right.clear
      oclear right
    endif
    null ->o parent
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
      this ->o left.parent
    endif
    if(right objNotNull)
      this ->o right.parent
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

//========================================= Node2 =========================================

// Node2 is just a test of sort/compare - reverse order of compare to reverse sort order
class: Node2 extends Node
  
  m: compare
    ->o Node b
    icmp(b.val val)
  ;m
  
;class

//========================================= Tree =========================================

class: Tree
  Node root
  int numNodes

  m: delete
    oclear root
  ;m
  
  m: setRoot
    -> root
    if(root objNotNull)
      null ->o root.parent
      root.count -> numNodes
    endif
  ;m
  
  m: clear
    if(root objNotNull)
      root.clear
      oclear root
    endif
    0 -> numNodes
  ;m
  
  m: count
    root.count
  ;m
  
  m: _traverseOp
    ->o Node n
    n.val .
  ;m
    
  m: _traverseInOrder
    recursive
    ->o Node n
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
    ->o Node n
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
    ->o Node n
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
    ->o Node newNode
    ->o Node n
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
    ->o Node n
    if(root objNotNull)
      _insert(root n)
      1 ->+ numNodes
    else
      setRoot(n)
    endif
  ;m
  
  m: addLeft
    ->o Node n
    //"addLeft " %s dup . %nl
    if(n.left objIsNull)
      1 ->+ numNodes
    endif
    n.setLeft(new Node)
    -> n.left.val
  ;m
  
  m: addRight
    ->o Node n
    //"addRight " %s dup . %nl
    if(n.right objIsNull)
      1 ->+ numNodes
    endif
    n.setRight(new Node)
    -> n.right.val
  ;m
  
  m: display
    if(root objIsNull)
      "tree is empty\n" %s
      exit
    endif
    
    "height: " %s root.height .  " #nodes: " %s numNodes . %nl
    0 -> int elementNum
    mko List q
    mko List otherQ
    q.addTail(root)
    true -> bool keepGoing
    false -> bool minSpacing
    begin
    while(keepGoing)
      false -> keepGoing
      begin
      while(not(q.isEmpty))
        if(elementNum elemSpacing.count <)
          spaces(elemSpacing.get(elementNum))
        else
          %bl
          true -> minSpacing
        endif
        1 ->+ elementNum
        q.unrefHead -> Node n
        if(n objNotNull)
          if(n.left objNotNull n.right objNotNull or)
            true -> keepGoing
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
      otherQ q ->o otherQ ->o q   // swap q and otherQ
      %nl
    repeat
  ;m
  
  m: _toArray
    recursive
    ->o Array a
    ->o Node n
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
    unref a
  ;m

  m: _fixParentLinks
    recursive
    ->o Node n
    ->o n.parent
    if(n.left objNotNull)
      _fixParentLinks(n n.left)
    endif
    if(n.right objNotNull)
      _fixParentLinks(n n.right)
    endif
  ;m

  m: _fromArray
    -> int right
    -> int left
    ->o Array a
    d[ "_fromArray " %s left . right . %nl ]d
    recursive
    right left - 1+ -> int nodeCount
    Node branchRoot
    case(nodeCount)
      of(1)
        a.get(left) ->o branchRoot
      endof
      
      of(2)
        a.get(right) ->o branchRoot
        branchRoot.setLeft(a.get(left))
      endof
      
      of(3)
        a.get(left 1+) ->o branchRoot
        branchRoot.setLeft(a.get(left))
        branchRoot.setRight(a.get(right))
      endof
      
      // default case
      dup 2/ left + -> int middle
      a.get(middle) ->o branchRoot
      branchRoot.setLeft(_fromArray(a left middle 1-))
      branchRoot.setRight(_fromArray(a middle 1+ right))
      
    endcase
    
    branchRoot
  ;m
  
  m: fromArray
    ->o Array a
    clear
    _fromArray(a 0 a.count 1-) -> root
    _fixParentLinks(null root)
    a.count -> numNodes
  ;m

  m: makeBST
    toArray -> Array a
    a.sort
    fromArray(a)
    oclear a
  ;m

  m: _makeHeap
    ->o Array a
    clear
    setRoot(a.get(0))
    
    a.count -> int nodeCount
    1 -> int ix
    do(nodeCount 0)
      a.get(i) ->o Node n
      if(ix nodeCount >=)
        leave
      endif
      n.setLeft(a.get(ix))
      1 ->+ ix
      if(ix nodeCount >=)
        leave
      endif
      n.setRight(a.get(ix))
      1 ->+ ix
    loop
  ;m

  m: makeMinHeap
    toArray -> Array a

    if(a.count 2 <)
      exit
    endif
    
    a.sort
    _makeHeap(a)
    a.count -> numNodes
    oclear a
  ;m
  
  m: makeMaxHeap
    toArray -> Array a

    if(a.count 2 <)
      exit
    endif
    
    a.sort
    a.reverse  // reversed sort order is only difference from minHeap
    _makeHeap(a)
    a.count -> numNodes
    oclear a
  ;m

  m: swapNodes
    // this is a major pain in the ass because of the parent links
    // this would make more sense if the Node's data payload was more than just an int
    ->o Node a
    ->o Node b
    true -> bool aIsRoot
    true -> bool bIsRoot
    
    a.right b.right ->o a.right ->o b.right
    a.left b.left ->o a.left ->o b.left
    a.parent b.parent ->o a.parent ->o b.parent

    // fix childrens parent links
    a._adoptChildren
    b._adoptChildren
    
    // fix parent of a
    a.parent ->o Node aParent
    if(aParent objNotNull)
      false -> aIsRoot
      if(aParent.left b =)
        a ->o aParent.left
      else
        if(aParent.right b =)
          a ->o aParent.right
        else
          "Tree.swapHard: b reparent failure!\n" %s
        endif
      endif
    endif
    
    // fix parent of b
    b.parent ->o Node bParent
    if(bParent objNotNull)
      false -> bIsRoot
      if(bParent.left a =)
        b ->o bParent.left
      else
        if(bParent.right a =)
          b ->o bParent.right
        else
          "Tree.swapHard: a reparent failure!\n" %s
        endif
      endif
    endif
    
    if(aIsRoot)
      a ->o root
    endif
    if(bIsRoot)
      b ->o root
    endif
  ;m
  
  m: swapValues
    ->o Node a
    ->o Node b
    d[ "swapValues " %s a.val . b.val . %nl ]d
    a.val b.val -> a.val -> b.val
  ;m

  // this assumes tree has all levels except bottom completely filled,
  //  bottom level is filled left-to-right
  m: insertInNextHeapSlot
    ->o Node n
    if(root objNotNull)
      // a relatively simple way of determining the left-right path to the rightmost node on
      //  the bottom (incomplete) layer of the tree - take the bits below the top set bit of the (numNodes + 1)
      // here we stick them on the stack starting at the low bit to do the reversing
      // this is a terrible way of reversing the bits, but it is fine for the trees we are dealing with (depth < 8 or so)
      numNodes 1+ -> int branchChoices
      0 -> int numChoices
      root -> Node insertPoint
      begin
      while(branchChoices 1 >)
        branchChoices 1 and
        rshift(branchChoices 1) -> branchChoices
        1 ->+ numChoices
      repeat
      begin
      while(numChoices 1 >)
        if
          insertPoint.right ->o insertPoint
        else
          insertPoint.left ->o insertPoint
        endif
        1 ->- numChoices
      repeat
      if
        insertPoint.setRight(n)
      else
        insertPoint.setLeft(n)
      endif
      
      1 ->+ numNodes
    else
      setRoot(n)
    endif
  ;m
  
  m: insertInMinHeap
    ->o Node n
    insertInNextHeapSlot(n)
    begin
    while(n.parent objNotNull)
      if(n.val n.parent.val <)
        swapValues(n n.parent)
        n.parent ->o n
      else
        exit
      endif
    repeat
  ;m
    
  m: insertInMaxHeap
    ->o Node n
    insertInNextHeapSlot(n)
    begin
    while(n.parent objNotNull)
      if(n.val n.parent.val >)
        swapValues(n n.parent)
        n.parent ->o n
      else
        exit
      endif
    repeat
  ;m
  
  m: remove
    ->o Node n
    
    d[ "Tree.removeNode - node with value " %s n.val . %nl ]d
    if(n.parent objNotNull)
      if(n.left objNotNull)
        n.left.count ->- numNodes
        oclear n.left
      endif
      if(n.right objNotNull)
        n.right.count ->- numNodes
        oclear n.right
      endif
      1 ->- numNodes
      if(n.parent.left n =)
        oclear n.parent.left
      else
        if(n.parent.right n =)
          oclear n.parent.right
        else
          "Tree.removeNode - node with value " %s n.val %d " has bad parent link\n" %s
        endif
      endif
      null ->o n.parent
    else
      // removed node is root, clear tree
      clear
    endif
  ;m

  // this assumes tree has all levels except bottom completely filled,
  //  bottom level is filled left-to-right
  m: findBottomHeapSlot
    int branchChoices
    if(root objNotNull)
      // a relatively simple way of determining the left-right path to the rightmost node on
      //  the bottom (incomplete) layer of the tree - take the bits below the top set bit of the (numNodes + 1)
      // here we stick them on the stack starting at the low bit to do the reversing
      // this is a terrible way of reversing the bits, but it is fine for the trees we are dealing with (depth < 8 or so)
      numNodes -> branchChoices
      0 -> int numChoices
      root -> Node bottomSlot
      begin
      while(branchChoices 1 >)
        branchChoices 1 and
        rshift(branchChoices 1) -> branchChoices
        1 ->+ numChoices
      repeat
      begin
      while(numChoices 0>)
        if
          bottomSlot.right ->o bottomSlot
        else
          bottomSlot.left ->o bottomSlot
        endif
        1 ->- numChoices
      repeat
      bottomSlot
      
    else
      null
    endif
  ;m
  
  m: _extractHeapTopValue
    // overall idea:
    //  return value in top slot
    //  stuff value from right-bottommost slot into top slot
    //  remove right-bottommost slot
    //  swap value from top-slot down into its child with the smaller value until value reaches bottom level,
    //    reestablishing that this is a min heap
    -> op compareOp
    if(root objNotNull)
      root.val
      findBottomHeapSlot ->o Node bottomSlot
      if(bottomSlot objNotNull)
        // replace value in bottomMost heap element in top slot of heap
        bottomSlot.val dup -> root.val
        -> int movingValue
        remove(bottomSlot)
        root ->o Node movingSlot
        Node nextMovingSlot
        begin
        while(movingSlot objNotNull)
          null ->o nextMovingSlot
          if(movingSlot.left objNotNull)
            if(movingSlot.right objNotNull)
              // node has left & right children
              if(compareOp(movingSlot.right.val movingSlot.left.val))
                if(compareOp(movingValue movingSlot.left.val))
                  movingSlot.left ->o nextMovingSlot
                endif
              else
                if(compareOp(movingValue movingSlot.right.val))
                  movingSlot.right ->o nextMovingSlot
                endif
              endif
            else
              // there is only a left child
              if(compareOp(movingValue movingSlot.left.val))
                movingSlot.left ->o nextMovingSlot
              endif
            endif
          else
            // there is no left child
            if(movingSlot.right objNotNull)
              if(compareOp(movingValue movingSlot.right.val))
                movingSlot.right ->o nextMovingSlot
              endif
            endif
          endif
          if(nextMovingSlot objNotNull)
            swapValues(movingSlot nextMovingSlot)
          endif
          nextMovingSlot ->o movingSlot
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
    _extractHeapTopValue(lit >)
  ;m  
  
  m: extractMaxHeapValue
    _extractHeapTopValue(lit <)
  ;m  
  
  // create a binary tree, not sorted, not necessarily balanced
  m: fillRandom    // rootNode numNodes ...
    1- -> int nodeCount
    setRoot(new Node)
    mko List q
    q.addHead(root)
    "seed: " %s getRandomSeed . %nl
    smallRandom -> root.val
    begin
    while(nodeCount 0>)
      q.unrefTail -> Node n
      mod(smallRandom 100) -> int diceRoll
      75 ->- diceRoll
      if(diceRoll 0<)
        addLeft(smallRandom n)
        addRight(smallRandom n)
        q.addHead(n.left)
        q.addHead(n.right)
        2 ->- nodeCount
      else
        12 ->- diceRoll
        if(diceRoll 0<)
          addLeft(smallRandom n)
          q.addHead(n.left)
        else
          addRight(smallRandom n)
          q.addHead(n.right)
        endif
        1 ->- nodeCount
      endif
      oclear n
    repeat
    %nl
    oclear q
    count -> numNodes
  ;m

  // create a sorted binary tree, not necessarily balanced
  m: fillSorted    // numNodes ...
    1- -> int nodeCount
    setRoot(new Node)
    "seed: " %s getRandomSeed . %nl
    smallRandom -> root.val
    begin
    while(nodeCount 0>)
      new Node -> Node n
      smallRandom -> n.val
      insert(n)
      oclear n
      1 ->- nodeCount
    repeat
    count -> numNodes
  ;m

;class

mko Tree t



// create an unsorted binary tree, not necessarily balanced but close
: fillTree    // rootNode numNodes ...
  1- -> int numNodes
  t.root ->o Node root
  root.clear
  mko List q
  q.addHead(root)
  "seed: " %s getRandomSeed . %nl
  smallRandom -> root.val
  d[ root.val . ]d
  begin
  while(numNodes 0>)
    q.unrefTail -> Node n
    t.addLeft(smallRandom n)
    t.addRight(smallRandom n)
    q.addHead(n.left)
    q.addHead(n.right)
    d[ n.left.val . n.right.val . ]d
    oclear n
    2 ->- numNodes
  repeat
  %nl
  oclear q
;

: tt1
  t.clear
  Node n
  0 do
    new Node -> n
    smallRandom -> n.val
    t.insertInMinHeap(n)
    oclear n
  loop
  t.display
;

: tt2
  -> int numElems
  mko Array a
  
  do(numElems 0)
    new Node -> Node n
    smallRandom -> n.val
    a.push(n)
    oclear n
  loop
  
  do(numElems 0)
    a.get(i) ->o n
    i . n.val . %nl
  loop
  
  a.sort
  
  do(numElems 0)
    a.get(i) ->o n
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

: tt5  // list nodes at depth
  mko List qA
  mko List qB
  Node n
  
  qA.addTail(t.root)
  begin
  while(not(qA.isEmpty))
    // print qA nodes
    qA.headIter -> ListIter iter
    begin
    while(iter.next)
      <Node>.val .
    repeat
    oclear iter
    %nl
    null ->o n
    
    // add children of qA to qB
    begin
    while(not(qA.isEmpty))
      qA.unrefHead -> n
      if(n.left objNotNull)
        qB.addTail(n.left)
      endif
      if(n.right objNotNull)
        qB.addTail(n.right)
      endif
    repeat
    oclear n
    
    // swap qA and qB
    qA qB ->o qA ->o qB
    
  repeat
  oclear qA
  oclear qB
;

: go
  20 tt1 20 tt3
  20 tt2
;

loaddone
