
autoforget rbtree

: rbtree ;

true -> compileAsserts

0 -> int _nextNodeId

class: RBNode
  Object	key
  Object	value
  RBNode		left
  RBNode		right
  int		red	\ true == red   false == black
  int		N
  int		_nodeId

  : _newNode
    _allocObject
    _nextNodeId over -> <RBNode>._nodeId
    \ "<created " %s _nextNodeId %d ">\n" %s
    1 ->+ _nextNodeId
  ;

  ' _newNode RBNode.setNew
  
  
  m: init
    -> N
    -> red
    -> value
    -> key
  ;m
 
  m: delete
    \ "<deleted " %s _nextNodeId %d ">\n" %s
    oclear key
    oclear value
    oclear left
    oclear right
    super.delete
  ;m

  \ m: show
  \  "red:" %s red %d " N:" %s N %d
  \  " key:" %s key oshow
  \  " value:" %s value oshow
  \  "[left:" %s left oshow "][right:" %s right oshow "]" %s
  \ ;m

;class

class: RBTree
  RBNode		root

  m: delete
    oclear root
    super.delete
  ;m
      
  : isRed
    if(objNotNull(dup))
      <RBNode>.red
    endif
  ;

  : isBlack
    if(objNotNull(dup))
      <RBNode>.red not
    endif
  ;

  : _size
    if(objNotNull(dup))
      <RBNode>.N
    endif
  ;

  m: size
    _size( root )
  ;m
  
  m: isEmpty
    root objIsNull
  ;m
  
  \ *************************************************************************
  \  Standard BST search
  \ *************************************************************************
  
  \ KEY_OBJECT NODE ... NODE
  : _get    \   returns Object
    -> RBNode x
    -> Object key
    begin
    while( x objNotNull )
      key.compare( x.key ) -> int cmp
      if( cmp 0< )
        x.left -> x
      else
        if( cmp 0> )
          x.right -> x
        else
          x.value
          oclear x
          exit
        endif
      endif
      
    repeat
    
    null  \ didn't find it
  ;
  
  m: get   returns Object
    _get( root )
  ;m
  
  m: contains
    _get( root ) objNotNull
  ;m
  

  \ *************************************************************************
  \  red-black tree helper functions
  \ *************************************************************************
  
  \ make a left-leaning link lean to the right
 : rotateRight      \ NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull  isRed( h.left ) and ]
    h.left -> RBNode x
    x.right -> h.left
    h -> x.right
    x.right.red -> x.red
    true -> x.right.red
    h.N -> x.N
    _size( h.left ) _size( h.right ) 1+ + -> h.N
    unref x
    oclear h
  ;

  
  \ make a right-leaning link lean to the left

  : rotateLeft      \ NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull  isRed( h.right ) and ]
    h.right -> RBNode x
    x.left -> h.right
    h -> x.left
    x.left.red -> x.red
    true -> x.left.red
    h.N -> x.N
    _size( h.left ) _size( h.right ) 1+ + -> h.N
    unref x
    oclear h
  ;
  
  \ flip the colors of a node and its two children
  : flipColors      \ NODE ...
    \ h must have opposite color of its two children
    -> RBNode h
    assert[ h objNotNull h.left objNotNull and h.right objNotNull and ]
    assert[ h.left.red h.right.red = h.left.red h.red <> and ]
    h.red dup dup
    not -> h.red
    -> h.left.red
    -> h.right.red
    oclear h
  ;
  
  \ Assuming that h is red and both h.left and h.left.left
  \ are black, make h.left or one of its children red.
  : moveRedLeft      \ NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull ]
    assert[ isRed( h ) isBlack( h.left ) and isBlack( h.left.left ) and ]

    flipColors( h )
    if( isRed( h.right.left ) )
      rotateRight( h.right ) -> h.right
      rotateLeft( h ) -> h
      \ NOTE: one version of the algorithm has this line commented, the other has it uncommented:
      \ flipColors( h )
    endif
    unref h
  ;

  \ Assuming that h is red and both h.right and h.right.left
  \ are black, make h.right or one of its children red.
  : moveRedRight      \ NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull ]
    assert[ isRed( h ) isBlack( h.right ) and isBlack( h.right.left ) and ]

    flipColors( h )
    if( isRed( h.left.left ) )
      rotateRight( h ) -> h
      \ NOTE: one version of the algorithm has this line commented, the other has it uncommented:
      \ flipColors( h )
    endif
    unref h
  ;

  : balance      \ NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull ]

    if( isRed( h.right ) )
      rotateLeft( h ) -> h
    endif
    if( and(isRed( h.left ) isRed( h.left.left )) )
      rotateRight( h ) -> h
    endif
    if( and(isRed( h.left ) isRed( h.right )) )
      flipColors( h )
    endif
    _size( h.left ) _size( h.right ) + 1+ -> h.N
    unref h
  ;
  
  \ *************************************************************************
  \  Red-black insertion
  \ *************************************************************************
  
  : _put      \ KEY_OBJECT VALUE_OBJECT NODE ... NODE
    recursive
    -> RBNode h
    -> Object val
    -> Object key
    if( h objIsNull )
    "empty tree\n" %s
      new RBNode -> h
      h.init( key val true 1 )
      
    else
    
      key.compare( h.key ) -> int cmp
      if( cmp 0< )
      dstack "add left\n" %s
        _put( key val h.left ) -> h.left
      else
      "add right\n" %s
        if( cmp 0> )
          _put( key val h.right ) -> h.right
        else
          val -> h.value
        endif
      endif
    
      \ fixup any right-leaning links
      if( isRed( h.right ) isRed( h.left ) not and )
        rotateLeft( h ) -> h
      endif
      if( isRed( h.left ) isRed( h.left.left ) and )
        rotateRight( h ) -> h
      endif
      if( isRed( h.left ) isRed( h.right ) and )
        flipColors( h )
      endif
      _size( h.left ) _size( h.right ) + 1+ -> h.N
      
    endif
    unref h
    oclear val
    oclear key
  ;
  

  m: put   \ KEY_OBJECT VALUE_OBJECT
    _put( root ) -> root
    false -> root.red
  ;m


  : _min      \ NODE ... NODE 
    recursive
    -> RBNode x
    assert[ x objNotNull ]
    if( x.left objIsNull )
      x
    else
      _min( x.left )
    endif
    oclear x
  ;
  
  m: min     \ ... NODE
    returns Object
    if( isEmpty )
      null
    else
      _min( root )
    endif
  ;m
  
  : _max      \ NODE ... NODE 
    recursive
    -> RBNode x
    assert[ x objNotNull ]
    if( x.right objIsNull )
      x
    else
      _max( x.right )
    endif
    oclear x
  ;
  
  m: max      \ ... NODE
    returns Object
    if( isEmpty )
      null
    else
      _max( root )
    endif
  ;m
  
  : _floor      \ VALUE_NODE KEY_NODE ... NODE
    recursive
    -> Object key
    -> RBNode x
    if( x objIsNull )
      null
    else
      key.compare( x.key ) -> int cmp
      if( cmp 0= )
        x
      else
        if( cmp 0< )
          _floor( x.left key )
        else
          _floor( x.right key ) -> RBNode t
          if( t objNotNull )
            t
          else
            x
          endif
          oclear t
        endif
      endif
    endif
    oclear x
    oclear key
  ;
  
  m: floor   returns RBNode
    -> Object key
    RBNode x
    _floor( root key ) -> x
    if( x objIsNull )
      null
    else
      x.key
    endif
    oclear x
    oclear key
  ;m
  
  : _ceiling      \ NODE KEY ... NODE
    recursive
    -> Object key
    -> RBNode x
    if( x objIsNull )
      null
    else
      key.compare( x.key ) -> int cmp
      if( cmp 0= )
        x
      else
        if( cmp 0> )
          _ceiling( x.right key )
        else
          _ceiling( x.left key ) -> RBNode t
          if( t objNotNull )
            t
          else
            x
          endif
          oclear t
        endif
      endif
    endif
    oclear x
    oclear key
  ;
  
  m: ceiling   returns RBNode
    -> Object key
    RBNode x
    _floor( root key ) -> x
    if( x objIsNull )
      null
    else
      x.key
    endif
    oclear x
    oclear key
  ;m

  \ the key of rank k in the subtree rooted at x
  : _select      \ NODE KEY ... NODE
    recursive
    -> int k
    -> RBNode x
    assert[ x objNotNull ]
    assert[ k 0>=  k _size( x ) < and ]
    _size( x.left ) -> int t
    if( t k > )
      _select( x.left k )
    else
      if( t k < )
        _select( x.right k t - 1- )
      else
        x
      endif
    endif
    oclear x
  ;

  \ the key of rank k  
  m: select   returns Object
    -> int k
    RBNode x
    if( k 0< k size >= or )
      null
    else
      _select( root k )
    endif
    x.key
    oclear x
  ;m
   

  : _rank       \ NODE ... RANK_OF_NODE
    recursive
    -> RBNode x
    -> Object key
    if( x objIsNull )
      null
    else
      key.compare( x.key ) -> int cmp
      if( cmp 0< )
        _rank( key x.left )
      else
        if( cmp 0> )
          _size( x.left ) _rank( key x.right ) + 1+
        else
          _size( x.left )
        endif
      endif
    endif
    oclear x
    oclear key
  ;
  
  m: rank
    _rank( root )
  ;m
  
  \ *************************************************************************
  \  Red-black deletion
  \ *************************************************************************
  
  : _removeMin      \ NODE ... NODE
    recursive
    -> RBNode h
    if( h.left objIsNull )
      null
    else
      if( and(isBlack( h.left ) isBlack( h.left.left )) )
        moveRedLeft( h ) -> h
      endif
      
      _removeMin( h.left ) -> h.left
      
      balance( h )
      
    endif
    oclear h
  ;
 
  : _remove       \ KEY NODE ... NODE
    recursive
    -> RBNode h
    -> Object key
    assert[ contains( key h ) ]
    
    if( key.compare( h.key ) 0< )
      if( and( isBlack( h.left ) isBlack( h.left.left ) ) )
        moveRedLeft( h ) -> h
      endif
      _remove( key h.left ) -> h.left
    else
      if( isRed( h.left ) )
        rotateRight( h ) -> h
      endif
      if( key.compare( h.key ) 0= h.right 0= and )
        oclear key
        oclear h
        null
        exit
      endif
      if( and( isBlack( h.right ) isBlack( h.right.left ) ) )
        moveRedRight( h ) -> h
      endif
      if( key.compare( h.key 0= ) )
        _get( min( h.right ) <RBNode>.key h.right ) -> h.value
        min( h.right ) <RBNode>.key -> h.key
        _removeMin( h.right ) -> h.right
      else
        _remove( key h.right ) -> h.right
      endif
    endif
    balance( h )
    oclear key
    oclear h
  ;
  
  m: remove
    -> Object key
    if( contains( key ) not )
      \ TODO: print error
    else
      if( and( isBlack( root.left ) isBlack( root.right ) ) )
        true -> root.red
      endif
      
      _remove( key root ) -> root
      if( isEmpty not )
        false -> root.red
      endif
    
    endif
    
    oclear key
  ;m
  
  m: removeMin
    if( isEmpty )
      \ TODO: throw new NoSuchElementException("BST underflow");
    endif

    \ if both children of root are black, set root to red
    if( and( isBlack( root.left ) isBlack( root.right ) ) )
      true -> root.red
    endif
    
    _removeMin( root ) -> root
    if( not( isEmpty ) )
      false -> root.red
    endif
    \ assert[ check ]
  ;m
  
  
  : _removeMax      \ NODE ... NODE
    recursive
    -> RBNode h
    if( isRed( h.left ) )
      rotateRight( h ) -> h
    endif
    
    if( h.right objIsNull )
      null
    else
      if( and( isBlack( h.right ) isBlack( h.right.left ) ) )
        moveRedRight( h ) -> h
      endif
      _removeMax( h.right ) -> h.right
      balance( h )
    endif
    
    oclear h
  ;
  
  
  m: removeMax
    assert[ not( isEmpty ) ]
    \ if (isEmpty()) throw new NoSuchElementException("BST underflow");
    
    \ if both children of root are black, set root to red
    if( and( isBlack( root.left ) isBlack( root.right ) ) )
      true -> root.red
    endif

    _removeMax( root ) -> root
    if( not( isEmpty ) )
      false -> root.red
    endif
    \ assert[ check ]
  ;m
  
  : _height     \ NODE ... NODE_HEIGHT
    "_height" %s ds
    recursive
    -> RBNode x
    if( x objIsNull )
      0
    else
      forth:max( _height( x.left ) _height( x.right ) ) 1+
    endif

    oclear x
  ;
  
  m: height
    _height( root )
  ;m
  
;class

loaddone


