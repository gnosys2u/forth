
autoforget rbtree

: rbtree ;

true -> compileAsserts

0 -> int _nextNodeId

class: RBNode
  Object	key
  Object	value
  RBNode		left
  RBNode		right
  int		red	// true == red   false == black
  int		N
  int		_nodeId

  : _newNode
    _allocObject
    _nextNodeId over -> <RBNode>._nodeId
    //"<created " %s _nextNodeId %d ">\n" %s
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
    //"<deleted " %s _nextNodeId %d ">\n" %s
    oclear key
    oclear value
    oclear left
    oclear right
    super.delete
  ;m

  //m: show
  //  "red:" %s red %d " N:" %s N %d
  //  " key:" %s key oshow
  //  " value:" %s value oshow
  //  "[left:" %s left oshow "][right:" %s right oshow "]" %s
  //;m

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
  
  //*************************************************************************
  //  Standard BST search
  //*************************************************************************
  
  // KEY_OBJECT NODE ... NODE
  : _get    //   returns Object
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
    
    null  // didn't find it
  ;
  
  m: get   returns Object
    _get( root )
  ;m
  
  m: contains
    _get( root ) objNotNull
  ;m
  

  //*************************************************************************
  //  red-black tree helper functions
  //*************************************************************************
  
  // make a left-leaning link lean to the right
 : rotateRight      // NODE ... NODE 
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

  
  // make a right-leaning link lean to the left

  : rotateLeft      // NODE ... NODE 
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
  
  // flip the colors of a node and its two children
  : flipColors      // NODE ...
    // h must have opposite color of its two children
    -> RBNode h
    assert[ h objNotNull h.left objNotNull and h.right objNotNull and ]
    assert[ h.left.red h.right.red = h.left.red h.red <> and ]
    h.red dup dup
    not -> h.red
    -> h.left.red
    -> h.right.red
    oclear h
  ;
  
  // Assuming that h is red and both h.left and h.left.left
  // are black, make h.left or one of its children red.
  : moveRedLeft      // NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull ]
    assert[ isRed( h ) isBlack( h.left ) and isBlack( h.left.left ) and ]

    flipColors( h )
    if( isRed( h.right.left ) )
      rotateRight( h.right ) -> h.right
      rotateLeft( h ) -> h
      // NOTE: one version of the algorithm has this line commented, the other has it uncommented:
      // flipColors( h )
    endif
    unref h
  ;

  // Assuming that h is red and both h.right and h.right.left
  // are black, make h.right or one of its children red.
  : moveRedRight      // NODE ... NODE 
    -> RBNode h
    assert[ h objNotNull ]
    assert[ isRed( h ) isBlack( h.right ) and isBlack( h.right.left ) and ]

    flipColors( h )
    if( isRed( h.left.left ) )
      rotateRight( h ) -> h
      // NOTE: one version of the algorithm has this line commented, the other has it uncommented:
      // flipColors( h )
    endif
    unref h
  ;

  : balance      // NODE ... NODE 
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
  
  //*************************************************************************
  //  Red-black insertion
  //*************************************************************************
  
  : _put      // KEY_OBJECT VALUE_OBJECT NODE ... NODE
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
    
      // fixup any right-leaning links
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
  

  m: put   // KEY_OBJECT VALUE_OBJECT
    _put( root ) -> root
    false -> root.red
  ;m


  : _min      // NODE ... NODE 
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
  
  m: min     // ... NODE
    returns Object
    if( isEmpty )
      null
    else
      _min( root )
    endif
  ;m
  
  : _max      // NODE ... NODE 
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
  
  m: max      // ... NODE
    returns Object
    if( isEmpty )
      null
    else
      _max( root )
    endif
  ;m
  
  : _floor      // VALUE_NODE KEY_NODE ... NODE
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
  
  : _ceiling      // NODE KEY ... NODE
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

  // the key of rank k in the subtree rooted at x
  : _select      // NODE KEY ... NODE
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

  // the key of rank k  
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
   

  : _rank       // NODE ... RANK_OF_NODE
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
  
  //*************************************************************************
  //  Red-black deletion
  //*************************************************************************
  
  : _removeMin      // NODE ... NODE
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
 
  : _remove       // KEY NODE ... NODE
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
      // TODO: print error
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
      // TODO: throw new NoSuchElementException("BST underflow");
    endif

    // if both children of root are black, set root to red
    if( and( isBlack( root.left ) isBlack( root.right ) ) )
      true -> root.red
    endif
    
    _removeMin( root ) -> root
    if( not( isEmpty ) )
      false -> root.red
    endif
    // assert[ check ]
  ;m
  
  
  : _removeMax      // NODE ... NODE
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
    // if (isEmpty()) throw new NoSuchElementException("BST underflow");
    
    // if both children of root are black, set root to red
    if( and( isBlack( root.left ) isBlack( root.right ) ) )
      true -> root.red
    endif

    _removeMax( root ) -> root
    if( not( isEmpty ) )
      false -> root.red
    endif
    // assert[ check ]
  ;m
  
  : _height     // NODE ... NODE_HEIGHT
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


// iterative in-order tree walk
//  from http://leetcode.com/2010/04/binary-search-tree-in-order-traversal.html
void inorder(Node *root)
{
    stack s;
    Node *cur = root;
    s.push(NULL); // Delimiter for end of processing
    
    // Left extreme arm of tree (part of step 1)
    while( cur )
    {
        s.push(cur);
        cur = cur->left;
    }
 
    // Process each node (step 2 & 3)    
    while( cur = stack.pop() )
    {
        visitNode(cur);
        cur = cur->right;
        
        // Step 1
        while( cur )
        {
            s.push(cur);
            cur = cur->left;
        }
    }
}










    // value associated with the given key; null if no such key
    public Value get(Key key) { return get(root, key); }

    // value associated with the given key in subtree rooted at x; null if no such key
    private Value get(Node x, Key key) {
        while (x <> null) {
            int cmp = key.compareTo(x.key);
            if      (cmp < 0) x = x.left;
            else if (cmp > 0) x = x.right;
            else              return x.val;
        }
        return null;
    }

    // is there a key-value pair with the given key?
    public boolean contains(Key key) {
        return (get(key) != null);
    }

    // is there a key-value pair with the given key in the subtree rooted at x?
    private boolean contains(Node x, Key key) {
        return (get(x, key) != null);
    }

   /*************************************************************************
    *  Red-black insertion
    *************************************************************************/

    // insert the key-value pair; overwrite the old value with the new value
    // if the key is already present
    public void put(Key key, Value val) {
        root = put(root, key, val);
        root.color = BLACK;
        assert check();
    }

    // insert the key-value pair in the subtree rooted at h
    private Node put(Node h, Key key, Value val) { 
        if (h == null) return new Node(key, val, RED, 1);

        int cmp = key.compareTo(h.key);
        if      (cmp < 0) h.left  = put(h.left,  key, val); 
        else if (cmp > 0) h.right = put(h.right, key, val); 
        else              h.val   = val;

        // fix-up any right-leaning links
        if (isRed(h.right) && !isRed(h.left))      h = rotateLeft(h);
        if (isRed(h.left)  &&  isRed(h.left.left)) h = rotateRight(h);
        if (isRed(h.left)  &&  isRed(h.right))     flipColors(h);
        h.N = size(h.left) + size(h.right) + 1;

        return h;
    }

   /*************************************************************************
    *  Red-black deletion
    *************************************************************************/

    // delete the key-value pair with the minimum key
    public void deleteMin() {
        if (isEmpty()) throw new NoSuchElementException("BST underflow");

        // if both children of root are black, set root to red
        if (!isRed(root.left) && !isRed(root.right))
            root.color = RED;

        root = deleteMin(root);
        if (!isEmpty()) root.color = BLACK;
        assert check();
    }

    // delete the key-value pair with the minimum key rooted at h
    private Node deleteMin(Node h) { 
        if (h.left == null)
            return null;

        if (!isRed(h.left) && !isRed(h.left.left))
            h = moveRedLeft(h);

        h.left = deleteMin(h.left);
        return balance(h);
    }


    // delete the key-value pair with the maximum key
    public void deleteMax() {
        if (isEmpty()) throw new NoSuchElementException("BST underflow");

        // if both children of root are black, set root to red
        if (!isRed(root.left) && !isRed(root.right))
            root.color = RED;

        root = deleteMax(root);
        if (!isEmpty()) root.color = BLACK;
        assert check();
    }

    // delete the key-value pair with the maximum key rooted at h
    private Node deleteMax(Node h) { 
        if (isRed(h.left))
            h = rotateRight(h);

        if (h.right == null)
            return null;

        if (!isRed(h.right) && !isRed(h.right.left))
            h = moveRedRight(h);

        h.right = deleteMax(h.right);

        return balance(h);
    }

    // delete the key-value pair with the given key
    public void delete(Key key) { 
        if (!contains(key)) {
            System.err.println("symbol table does not contain " + key);
            return;
        }

        // if both children of root are black, set root to red
        if (!isRed(root.left) && !isRed(root.right))
            root.color = RED;

        root = delete(root, key);
        if (!isEmpty()) root.color = BLACK;
        assert check();
    }

    // delete the key-value pair with the given key rooted at h
    private Node delete(Node h, Key key) { 
        assert contains(h, key);

        if (key.compareTo(h.key) < 0)  {
            if (!isRed(h.left) && !isRed(h.left.left))
                h = moveRedLeft(h);
            h.left = delete(h.left, key);
        }
        else {
            if (isRed(h.left))
                h = rotateRight(h);
            if (key.compareTo(h.key) == 0 && (h.right == null))
                return null;
            if (!isRed(h.right) && !isRed(h.right.left))
                h = moveRedRight(h);
            if (key.compareTo(h.key) == 0) {
                h.val = get(h.right, min(h.right).key);
                h.key = min(h.right).key;
                h.right = deleteMin(h.right);
            }
            else h.right = delete(h.right, key);
        }
        return balance(h);
    }

   /*************************************************************************
    *  red-black tree helper functions
    *************************************************************************/

    // make a left-leaning link lean to the right
    private Node rotateRight(Node h) {
        assert (h != null) && isRed(h.left);
        Node x = h.left;
        h.left = x.right;
        x.right = h;
        x.color = x.right.color;
        x.right.color = RED;
        x.N = h.N;
        h.N = size(h.left) + size(h.right) + 1;
        return x;
    }

    // make a right-leaning link lean to the left
    private Node rotateLeft(Node h) {
        assert (h != null) && isRed(h.right);
        Node x = h.right;
        h.right = x.left;
        x.left = h;
        x.color = x.left.color;
        x.left.color = RED;
        x.N = h.N;
        h.N = size(h.left) + size(h.right) + 1;
        return x;
    }

    // flip the colors of a node and its two children
    private void flipColors(Node h) {
        // h must have opposite color of its two children
        assert (h != null) && (h.left != null) && (h.right != null);
        assert (!isRed(h) &&  isRed(h.left) &&  isRed(h.right))
            || (isRed(h)  && !isRed(h.left) && !isRed(h.right));
        h.color = !h.color;
        h.left.color = !h.left.color;
        h.right.color = !h.right.color;
    }

    // Assuming that h is red and both h.left and h.left.left
    // are black, make h.left or one of its children red.
    private Node moveRedLeft(Node h) {
        assert (h != null);
        assert isRed(h) && !isRed(h.left) && !isRed(h.left.left);

        flipColors(h);
        if (isRed(h.right.left)) { 
            h.right = rotateRight(h.right);
            h = rotateLeft(h);
            // flipColors(h);
        }
        return h;
    }

    // Assuming that h is red and both h.right and h.right.left
    // are black, make h.right or one of its children red.
    private Node moveRedRight(Node h) {
        assert (h != null);
        assert isRed(h) && !isRed(h.right) && !isRed(h.right.left);
        flipColors(h);
        if (isRed(h.left.left)) { 
            h = rotateRight(h);
            // flipColors(h);
        }
        return h;
    }

    // restore red-black tree invariant
    private Node balance(Node h) {
        assert (h != null);

        if (isRed(h.right))                      h = rotateLeft(h);
        if (isRed(h.left) && isRed(h.left.left)) h = rotateRight(h);
        if (isRed(h.left) && isRed(h.right))     flipColors(h);

        h.N = size(h.left) + size(h.right) + 1;
        return h;
    }


   /*************************************************************************
    *  Utility functions
    *************************************************************************/

    // height of tree; 0 if empty
    public int height() { return height(root); }
    private int height(Node x) {
        if (x == null) return 0;
        return 1 + Math.max(height(x.left), height(x.right));
    }

   /*************************************************************************
    *  Ordered symbol table methods.
    *************************************************************************/

    // the smallest key; null if no such key
    public Key min() {
        if (isEmpty()) return null;
        return min(root).key;
    } 

    // the smallest key in subtree rooted at x; null if no such key
    private Node min(Node x) { 
        assert x != null;
        if (x.left == null) return x; 
        else                return min(x.left); 
    } 

    // the largest key; null if no such key
    public Key max() {
        if (isEmpty()) return null;
        return max(root).key;
    } 

    // the largest key in the subtree rooted at x; null if no such key
    private Node max(Node x) { 
        assert x != null;
        if (x.right == null) return x; 
        else                 return max(x.right); 
    } 

    // the largest key less than or equal to the given key
    public Key floor(Key key) {
        Node x = floor(root, key);
        if (x == null) return null;
        else           return x.key;
    }    

    // the largest key in the subtree rooted at x less than or equal to the given key
    private Node floor(Node x, Key key) {
        if (x == null) return null;
        int cmp = key.compareTo(x.key);
        if (cmp == 0) return x;
        if (cmp < 0)  return floor(x.left, key);
        Node t = floor(x.right, key);
        if (t != null) return t; 
        else           return x;
    }

    // the smallest key greater than or equal to the given key
    public Key ceiling(Key key) {  
        Node x = ceiling(root, key);
        if (x == null) return null;
        else           return x.key;  
    }

    // the smallest key in the subtree rooted at x greater than or equal to the given key
    private Node ceiling(Node x, Key key) {  
        if (x == null) return null;
        int cmp = key.compareTo(x.key);
        if (cmp == 0) return x;
        if (cmp > 0)  return ceiling(x.right, key);
        Node t = ceiling(x.left, key);
        if (t != null) return t; 
        else           return x;
    }


    // the key of rank k
    public Key select(int k) {
        if (k < 0 || k >= size())  return null;
        Node x = select(root, k);
        return x.key;
    }

    // the key of rank k in the subtree rooted at x
    private Node select(Node x, int k) {
        assert x != null;
        assert k >= 0 && k < size(x);
        int t = size(x.left); 
        if      (t > k) return select(x.left,  k); 
        else if (t < k) return select(x.right, k-t-1); 
        else            return x; 
    } 

    // number of keys less than key
    public int rank(Key key) {
        return rank(key, root);
    } 

    // number of keys less than key in the subtree rooted at x
    private int rank(Key key, Node x) {
        if (x == null) return 0; 
        int cmp = key.compareTo(x.key); 
        if      (cmp < 0) return rank(key, x.left); 
        else if (cmp > 0) return 1 + size(x.left) + rank(key, x.right); 
        else              return size(x.left); 
    } 

   /***********************************************************************
    *  Range count and range search.
    ***********************************************************************/

    // all of the keys, as an Iterable
    public Iterable<Key> keys() {
        return keys(min(), max());
    }

    // the keys between lo and hi, as an Iterable
    public Iterable<Key> keys(Key lo, Key hi) {
        Queue<Key> queue = new Queue<Key>();
        // if (isEmpty() || lo.compareTo(hi) > 0) return queue;
        keys(root, queue, lo, hi);
        return queue;
    } 

    // add the keys between lo and hi in the subtree rooted at x
    // to the queue
    private void keys(Node x, Queue<Key> queue, Key lo, Key hi) { 
        if (x == null) return; 
        int cmplo = lo.compareTo(x.key); 
        int cmphi = hi.compareTo(x.key); 
        if (cmplo < 0) keys(x.left, queue, lo, hi); 
        if (cmplo <= 0 && cmphi >= 0) queue.enqueue(x.key); 
        if (cmphi > 0) keys(x.right, queue, lo, hi); 
    } 

    // number keys between lo and hi
    public int size(Key lo, Key hi) {
        if (lo.compareTo(hi) > 0) return 0;
        if (contains(hi)) return rank(hi) - rank(lo) + 1;
        else              return rank(hi) - rank(lo);
    }


   /*************************************************************************
    *  Check integrity of red-black BST data structure
    *************************************************************************/
    private boolean check() {
        if (!isBST())            StdOut.println("Not in symmetric order");
        if (!isSizeConsistent()) StdOut.println("Subtree counts not consistent");
        if (!isRankConsistent()) StdOut.println("Ranks not consistent");
        if (!is23())             StdOut.println("Not a 2-3 tree");
        if (!isBalanced())       StdOut.println("Not balanced");
        return isBST() && isSizeConsistent() && isRankConsistent() && is23() && isBalanced();
    }

    // does this binary tree satisfy symmetric order?
    // Note: this test also ensures that data structure is a binary tree since order is strict
    private boolean isBST() {
        return isBST(root, null, null);
    }

    // is the tree rooted at x a BST with all keys strictly between min and max
    // (if min or max is null, treat as empty constraint)
    // Credit: Bob Dondero's elegant solution
    private boolean isBST(Node x, Key min, Key max) {
        if (x == null) return true;
        if (min != null && x.key.compareTo(min) <= 0) return false;
        if (max != null && x.key.compareTo(max) >= 0) return false;
        return isBST(x.left, min, x.key) && isBST(x.right, x.key, max);
    } 

    // are the size fields correct?
    private boolean isSizeConsistent() { return isSizeConsistent(root); }
    private boolean isSizeConsistent(Node x) {
        if (x == null) return true;
        if (x.N != size(x.left) + size(x.right) + 1) return false;
        return isSizeConsistent(x.left) && isSizeConsistent(x.right);
    } 

    // check that ranks are consistent
    private boolean isRankConsistent() {
        for (int i = 0; i < size(); i++)
            if (i != rank(select(i))) return false;
        for (Key key : keys())
            if (key.compareTo(select(rank(key))) != 0) return false;
        return true;
    }

    // Does the tree have no red right links, and at most one (left)
    // red links in a row on any path?
    private boolean is23() { return is23(root); }
    private boolean is23(Node x) {
        if (x == null) return true;
        if (isRed(x.right)) return false;
        if (x != root && isRed(x) && isRed(x.left))
            return false;
        return is23(x.left) && is23(x.right);
    } 

    // do all paths from root to leaf have same number of black edges?
    private boolean isBalanced() { 
        int black = 0;     // number of black links on path from root to min
        Node x = root;
        while (x != null) {
            if (!isRed(x)) black++;
            x = x.left;
        }
        return isBalanced(root, black);
    }

    // does every path from the root to a leaf have the given number of black links?
    private boolean isBalanced(Node x, int black) {
        if (x == null) return black == 0;
        if (!isRed(x)) black--;
        return isBalanced(x.left, black) && isBalanced(x.right, black);
    } 


   /*****************************************************************************
    *  Test client
    *****************************************************************************/
    public static void main(String[] args) { 
        RedBlackBST<String, Integer> st = new RedBlackBST<String, Integer>();
        for (int i = 0; !StdIn.isEmpty(); i++) {
            String key = StdIn.readString();
            st.put(key, i);
        }
        for (String s : st.keys())
            StdOut.println(s + " " + st.get(s));
        StdOut.println();
    }
}





/******************************************************************************
 *  Compilation:  javac RedBlackBST.java
 *  Execution:    java RedBlackBST < input.txt
 *  Dependencies: StdIn.java StdOut.java  
 *  Data files:   http://algs4.cs.princeton.edu/33balanced/tinyST.txt  
 *    
 *  A symbol table implemented using a left-leaning red-black BST.
 *  This is the 2-3 version.
 *
 *  Note: commented out assertions because DrJava now enables assertions
 *        by default.
 *
 *  % more tinyST.txt
 *  S E A R C H E X A M P L E
 *  
 *  % java RedBlackBST < tinyST.txt
 *  A 8
 *  C 4
 *  E 12
 *  H 5
 *  L 11
 *  M 9
 *  P 10
 *  R 3
 *  S 0
 *  X 7
 *
 ******************************************************************************/

import java.util.NoSuchElementException;

/**
 *  The <tt>BST</tt> class represents an ordered symbol table of generic
 *  key-value pairs.
 *  It supports the usual <em>put</em>, <em>get</em>, <em>contains</em>,
 *  <em>delete</em>, <em>size</em>, and <em>is-empty</em> methods.
 *  It also provides ordered methods for finding the <em>minimum</em>,
 *  <em>maximum</em>, <em>floor</em>, and <em>ceiling</em>.
 *  It also provides a <em>keys</em> method for iterating over all of the keys.
 *  A symbol table implements the <em>associative array</em> abstraction:
 *  when associating a value with a key that is already in the symbol table,
 *  the convention is to replace the old value with the new value.
 *  Unlike {@link java.util.Map}, this class uses the convention that
 *  values cannot be <tt>null</tt>&mdash;setting the
 *  value associated with a key to <tt>null</tt> is equivalent to deleting the key
 *  from the symbol table.
 *  <p>
 *  This implementation uses a left-leaning red-black BST. It requires that
 *  the key type implements the <tt>Comparable</tt> interface and calls the
 *  <tt>compareTo()</tt> and method to compare two keys. It does not call either
 *  <tt>equals()</tt> or <tt>hashCode()</tt>.
 *  The <em>put</em>, <em>contains</em>, <em>remove</em>, <em>minimum</em>,
 *  <em>maximum</em>, <em>ceiling</em>, and <em>floor</em> operations each take
 *  logarithmic time in the worst case, if the tree becomes unbalanced.
 *  The <em>size</em>, and <em>is-empty</em> operations take constant time.
 *  Construction takes constant time.
 *  <p>
 *  For additional documentation, see <a href="http://algs4.cs.princeton.edu/33balanced">Section 3.3</a> of
 *  <i>Algorithms, 4th Edition</i> by Robert Sedgewick and Kevin Wayne.
 *  For other implementations, see {@link ST}, {@link BinarySearchST},
 *  {@link SequentialSearchST}, {@link BST},
 *  {@link SeparateChainingHashST}, and {@link LinearProbingHashST},
 *  <i>Algorithms, 4th Edition</i> by Robert Sedgewick and Kevin Wayne.
 */

public class RedBlackBST<Key extends Comparable<Key>, Value> {

    private static final boolean RED   = true;
    private static final boolean BLACK = false;

    private Node root;     // root of the BST

    // BST helper node data type
    private class Node {
        private Key key;           // key
        private Value val;         // associated data
        private Node left, right;  // links to left and right subtrees
        private boolean color;     // color of parent link
        private int N;             // subtree count

        public Node(Key key, Value val, boolean color, int N) {
            this.key = key;
            this.val = val;
            this.color = color;
            this.N = N;
        }
    }

    /**
     * Initializes an empty symbol table.
     */
    public RedBlackBST() {
    }

   /***************************************************************************
    *  Node helper methods.
    ***************************************************************************/
    // is node x red; false if x is null ?
    private boolean isRed(Node x) {
        if (x == null) return false;
        return x.color == RED;
    }

    // number of node in subtree rooted at x; 0 if x is null
    private int size(Node x) {
        if (x == null) return 0;
        return x.N;
    } 


    /**
     * Returns the number of key-value pairs in this symbol table.
     * @return the number of key-value pairs in this symbol table
     */
    public int size() {
        return size(root);
    }

   /**
     * Is this symbol table empty?
     * @return <tt>true</tt> if this symbol table is empty and <tt>false</tt> otherwise
     */
    public boolean isEmpty() {
        return root == null;
    }


   /***************************************************************************
    *  Standard BST search.
    ***************************************************************************/

    /**
     * Returns the value associated with the given key.
     * @param key the key
     * @return the value associated with the given key if the key is in the symbol table
     *     and <tt>null</tt> if the key is not in the symbol table
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public Value get(Key key) {
        return get(root, key);
    }

    // value associated with the given key in subtree rooted at x; null if no such key
    private Value get(Node x, Key key) {
        while (x != null) {
            int cmp = key.compareTo(x.key);
            if      (cmp < 0) x = x.left;
            else if (cmp > 0) x = x.right;
            else              return x.val;
        }
        return null;
    }

    /**
     * Does this symbol table contain the given key?
     * @param key the key
     * @return <tt>true</tt> if this symbol table contains <tt>key</tt> and
     *     <tt>false</tt> otherwise
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public boolean contains(Key key) {
        return get(key) != null;
    }

   /***************************************************************************
    *  Red-black tree insertion.
    ***************************************************************************/

    /**
     * Inserts the key-value pair into the symbol table, overwriting the old value
     * with the new value if the key is already in the symbol table.
     * If the value is <tt>null</tt>, this effectively deletes the key from the symbol table.
     * @param key the key
     * @param val the value
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public void put(Key key, Value val) {
        root = put(root, key, val);
        root.color = BLACK;
        // assert check();
    }

    // insert the key-value pair in the subtree rooted at h
    private Node put(Node h, Key key, Value val) { 
        if (h == null) return new Node(key, val, RED, 1);

        int cmp = key.compareTo(h.key);
        if      (cmp < 0) h.left  = put(h.left,  key, val); 
        else if (cmp > 0) h.right = put(h.right, key, val); 
        else              h.val   = val;

        // fix-up any right-leaning links
        if (isRed(h.right) && !isRed(h.left))      h = rotateLeft(h);
        if (isRed(h.left)  &&  isRed(h.left.left)) h = rotateRight(h);
        if (isRed(h.left)  &&  isRed(h.right))     flipColors(h);
        h.N = size(h.left) + size(h.right) + 1;

        return h;
    }

   /***************************************************************************
    *  Red-black tree deletion.
    ***************************************************************************/

    /**
     * Removes the smallest key and associated value from the symbol table.
     * @throws NoSuchElementException if the symbol table is empty
     */
    public void deleteMin() {
        if (isEmpty()) throw new NoSuchElementException("BST underflow");

        // if both children of root are black, set root to red
        if (!isRed(root.left) && !isRed(root.right))
            root.color = RED;

        root = deleteMin(root);
        if (!isEmpty()) root.color = BLACK;
        // assert check();
    }

    // delete the key-value pair with the minimum key rooted at h
    private Node deleteMin(Node h) { 
        if (h.left == null)
            return null;

        if (!isRed(h.left) && !isRed(h.left.left))
            h = moveRedLeft(h);

        h.left = deleteMin(h.left);
        return balance(h);
    }


    /**
     * Removes the largest key and associated value from the symbol table.
     * @throws NoSuchElementException if the symbol table is empty
     */
    public void deleteMax() {
        if (isEmpty()) throw new NoSuchElementException("BST underflow");

        // if both children of root are black, set root to red
        if (!isRed(root.left) && !isRed(root.right))
            root.color = RED;

        root = deleteMax(root);
        if (!isEmpty()) root.color = BLACK;
        // assert check();
    }

    // delete the key-value pair with the maximum key rooted at h
    private Node deleteMax(Node h) { 
        if (isRed(h.left))
            h = rotateRight(h);

        if (h.right == null)
            return null;

        if (!isRed(h.right) && !isRed(h.right.left))
            h = moveRedRight(h);

        h.right = deleteMax(h.right);

        return balance(h);
    }

    /**
     * Removes the key and associated value from the symbol table
     * (if the key is in the symbol table).
     * @param key the key
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public void delete(Key key) { 
        if (!contains(key)) {
            System.err.println("symbol table does not contain " + key);
            return;
        }

        // if both children of root are black, set root to red
        if (!isRed(root.left) && !isRed(root.right))
            root.color = RED;

        root = delete(root, key);
        if (!isEmpty()) root.color = BLACK;
        // assert check();
    }

    // delete the key-value pair with the given key rooted at h
    private Node delete(Node h, Key key) { 
        // assert get(h, key) != null;

        if (key.compareTo(h.key) < 0)  {
            if (!isRed(h.left) && !isRed(h.left.left))
                h = moveRedLeft(h);
            h.left = delete(h.left, key);
        }
        else {
            if (isRed(h.left))
                h = rotateRight(h);
            if (key.compareTo(h.key) == 0 && (h.right == null))
                return null;
            if (!isRed(h.right) && !isRed(h.right.left))
                h = moveRedRight(h);
            if (key.compareTo(h.key) == 0) {
                Node x = min(h.right);
                h.key = x.key;
                h.val = x.val;
                // h.val = get(h.right, min(h.right).key);
                // h.key = min(h.right).key;
                h.right = deleteMin(h.right);
            }
            else h.right = delete(h.right, key);
        }
        return balance(h);
    }

   /***************************************************************************
    *  Red-black tree helper functions.
    ***************************************************************************/

    // make a left-leaning link lean to the right
    private Node rotateRight(Node h) {
        // assert (h != null) && isRed(h.left);
        Node x = h.left;
        h.left = x.right;
        x.right = h;
        x.color = x.right.color;
        x.right.color = RED;
        x.N = h.N;
        h.N = size(h.left) + size(h.right) + 1;
        return x;
    }

    // make a right-leaning link lean to the left
    private Node rotateLeft(Node h) {
        // assert (h != null) && isRed(h.right);
        Node x = h.right;
        h.right = x.left;
        x.left = h;
        x.color = x.left.color;
        x.left.color = RED;
        x.N = h.N;
        h.N = size(h.left) + size(h.right) + 1;
        return x;
    }

    // flip the colors of a node and its two children
    private void flipColors(Node h) {
        // h must have opposite color of its two children
        // assert (h != null) && (h.left != null) && (h.right != null);
        // assert (!isRed(h) &&  isRed(h.left) &&  isRed(h.right))
        //    || (isRed(h)  && !isRed(h.left) && !isRed(h.right));
        h.color = !h.color;
        h.left.color = !h.left.color;
        h.right.color = !h.right.color;
    }

    // Assuming that h is red and both h.left and h.left.left
    // are black, make h.left or one of its children red.
    private Node moveRedLeft(Node h) {
        // assert (h != null);
        // assert isRed(h) && !isRed(h.left) && !isRed(h.left.left);

        flipColors(h);
        if (isRed(h.right.left)) { 
            h.right = rotateRight(h.right);
            h = rotateLeft(h);
            flipColors(h);
        }
        return h;
    }

    // Assuming that h is red and both h.right and h.right.left
    // are black, make h.right or one of its children red.
    private Node moveRedRight(Node h) {
        // assert (h != null);
        // assert isRed(h) && !isRed(h.right) && !isRed(h.right.left);
        flipColors(h);
        if (isRed(h.left.left)) { 
            h = rotateRight(h);
            flipColors(h);
        }
        return h;
    }

    // restore red-black tree invariant
    private Node balance(Node h) {
        // assert (h != null);

        if (isRed(h.right))                      h = rotateLeft(h);
        if (isRed(h.left) && isRed(h.left.left)) h = rotateRight(h);
        if (isRed(h.left) && isRed(h.right))     flipColors(h);

        h.N = size(h.left) + size(h.right) + 1;
        return h;
    }


   /***************************************************************************
    *  Utility functions.
    ***************************************************************************/

    /**
     * Returns the height of the BST (for debugging).
     * @return the height of the BST (a 1-node tree has height 0)
     */
    public int height() {
        return height(root);
    }
    private int height(Node x) {
        if (x == null) return -1;
        return 1 + Math.max(height(x.left), height(x.right));
    }

   /***************************************************************************
    *  Ordered symbol table methods.
    ***************************************************************************/

    /**
     * Returns the smallest key in the symbol table.
     * @return the smallest key in the symbol table
     * @throws NoSuchElementException if the symbol table is empty
     */
    public Key min() {
        if (isEmpty()) throw new NoSuchElementException("called min() with empty symbol table");
        return min(root).key;
    } 

    // the smallest key in subtree rooted at x; null if no such key
    private Node min(Node x) { 
        // assert x != null;
        if (x.left == null) return x; 
        else                return min(x.left); 
    } 

    /**
     * Returns the largest key in the symbol table.
     * @return the largest key in the symbol table
     * @throws NoSuchElementException if the symbol table is empty
     */
    public Key max() {
        if (isEmpty()) throw new NoSuchElementException("called max() with empty symbol table");
        return max(root).key;
    } 

    // the largest key in the subtree rooted at x; null if no such key
    private Node max(Node x) { 
        // assert x != null;
        if (x.right == null) return x; 
        else                 return max(x.right); 
    } 


    /**
     * Returns the largest key in the symbol table less than or equal to <tt>key</tt>.
     * @param key the key
     * @return the largest key in the symbol table less than or equal to <tt>key</tt>
     * @throws NoSuchElementException if there is no such key
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public Key floor(Key key) {
        if (isEmpty()) throw new NoSuchElementException("called floor() with empty symbol table");
        Node x = floor(root, key);
        if (x == null) return null;
        else           return x.key;
    }    

    // the largest key in the subtree rooted at x less than or equal to the given key
    private Node floor(Node x, Key key) {
        if (x == null) return null;
        int cmp = key.compareTo(x.key);
        if (cmp == 0) return x;
        if (cmp < 0)  return floor(x.left, key);
        Node t = floor(x.right, key);
        if (t != null) return t; 
        else           return x;
    }

    /**
     * Returns the smallest key in the symbol table greater than or equal to <tt>key</tt>.
     * @param key the key
     * @return the smallest key in the symbol table greater than or equal to <tt>key</tt>
     * @throws NoSuchElementException if there is no such key
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public Key ceiling(Key key) {  
        if (isEmpty()) throw new NoSuchElementException("called ceiling() with empty symbol table");
        Node x = ceiling(root, key);
        if (x == null) return null;
        else           return x.key;  
    }

    // the smallest key in the subtree rooted at x greater than or equal to the given key
    private Node ceiling(Node x, Key key) {  
        if (x == null) return null;
        int cmp = key.compareTo(x.key);
        if (cmp == 0) return x;
        if (cmp > 0)  return ceiling(x.right, key);
        Node t = ceiling(x.left, key);
        if (t != null) return t; 
        else           return x;
    }

    /**
     * Return the kth smallest key in the symbol table.
     * @param k the order statistic
     * @return the kth smallest key in the symbol table
     * @throws IllegalArgumentException unless <tt>k</tt> is between 0 and
     *     <em>N</em> &minus; 1
     */
    public Key select(int k) {
        if (k < 0 || k >= size()) throw new IllegalArgumentException();
        Node x = select(root, k);
        return x.key;
    }

    // the key of rank k in the subtree rooted at x
    private Node select(Node x, int k) {
        // assert x != null;
        // assert k >= 0 && k < size(x);
        int t = size(x.left); 
        if      (t > k) return select(x.left,  k); 
        else if (t < k) return select(x.right, k-t-1); 
        else            return x; 
    } 

    /**
     * Return the number of keys in the symbol table strictly less than <tt>key</tt>.
     * @param key the key
     * @return the number of keys in the symbol table strictly less than <tt>key</tt>
     * @throws NullPointerException if <tt>key</tt> is <tt>null</tt>
     */
    public int rank(Key key) {
        return rank(key, root);
    } 

    // number of keys less than key in the subtree rooted at x
    private int rank(Key key, Node x) {
        if (x == null) return 0; 
        int cmp = key.compareTo(x.key); 
        if      (cmp < 0) return rank(key, x.left); 
        else if (cmp > 0) return 1 + size(x.left) + rank(key, x.right); 
        else              return size(x.left); 
    } 

   /***************************************************************************
    *  Range count and range search.
    ***************************************************************************/

    /**
     * Returns all keys in the symbol table as an <tt>Iterable</tt>.
     * To iterate over all of the keys in the symbol table named <tt>st</tt>,
     * use the foreach notation: <tt>for (Key key : st.keys())</tt>.
     * @return all keys in the sybol table as an <tt>Iterable</tt>
     */
    public Iterable<Key> keys() {
        if (isEmpty()) return new Queue<Key>();
        return keys(min(), max());
    }

    /**
     * Returns all keys in the symbol table in the given range,
     * as an <tt>Iterable</tt>.
     * @return all keys in the sybol table between <tt>lo</tt> 
     *    (inclusive) and <tt>hi</tt> (exclusive) as an <tt>Iterable</tt>
     * @throws NullPointerException if either <tt>lo</tt> or <tt>hi</tt>
     *    is <tt>null</tt>
     */
    public Iterable<Key> keys(Key lo, Key hi) {
        Queue<Key> queue = new Queue<Key>();
        // if (isEmpty() || lo.compareTo(hi) > 0) return queue;
        keys(root, queue, lo, hi);
        return queue;
    } 

    // add the keys between lo and hi in the subtree rooted at x
    // to the queue
    private void keys(Node x, Queue<Key> queue, Key lo, Key hi) { 
        if (x == null) return; 
        int cmplo = lo.compareTo(x.key); 
        int cmphi = hi.compareTo(x.key); 
        if (cmplo < 0) keys(x.left, queue, lo, hi); 
        if (cmplo <= 0 && cmphi >= 0) queue.enqueue(x.key); 
        if (cmphi > 0) keys(x.right, queue, lo, hi); 
    } 

    /**
     * Returns the number of keys in the symbol table in the given range.
     * @return the number of keys in the sybol table between <tt>lo</tt> 
     *    (inclusive) and <tt>hi</tt> (exclusive)
     * @throws NullPointerException if either <tt>lo</tt> or <tt>hi</tt>
     *    is <tt>null</tt>
     */
    public int size(Key lo, Key hi) {
        if (lo.compareTo(hi) > 0) return 0;
        if (contains(hi)) return rank(hi) - rank(lo) + 1;
        else              return rank(hi) - rank(lo);
    }


   /***************************************************************************
    *  Check integrity of red-black tree data structure.
    ***************************************************************************/
    private boolean check() {
        if (!isBST())            StdOut.println("Not in symmetric order");
        if (!isSizeConsistent()) StdOut.println("Subtree counts not consistent");
        if (!isRankConsistent()) StdOut.println("Ranks not consistent");
        if (!is23())             StdOut.println("Not a 2-3 tree");
        if (!isBalanced())       StdOut.println("Not balanced");
        return isBST() && isSizeConsistent() && isRankConsistent() && is23() && isBalanced();
    }

    // does this binary tree satisfy symmetric order?
    // Note: this test also ensures that data structure is a binary tree since order is strict
    private boolean isBST() {
        return isBST(root, null, null);
    }

    // is the tree rooted at x a BST with all keys strictly between min and max
    // (if min or max is null, treat as empty constraint)
    // Credit: Bob Dondero's elegant solution
    private boolean isBST(Node x, Key min, Key max) {
        if (x == null) return true;
        if (min != null && x.key.compareTo(min) <= 0) return false;
        if (max != null && x.key.compareTo(max) >= 0) return false;
        return isBST(x.left, min, x.key) && isBST(x.right, x.key, max);
    } 

    // are the size fields correct?
    private boolean isSizeConsistent() { return isSizeConsistent(root); }
    private boolean isSizeConsistent(Node x) {
        if (x == null) return true;
        if (x.N != size(x.left) + size(x.right) + 1) return false;
        return isSizeConsistent(x.left) && isSizeConsistent(x.right);
    } 

    // check that ranks are consistent
    private boolean isRankConsistent() {
        for (int i = 0; i < size(); i++)
            if (i != rank(select(i))) return false;
        for (Key key : keys())
            if (key.compareTo(select(rank(key))) != 0) return false;
        return true;
    }

    // Does the tree have no red right links, and at most one (left)
    // red links in a row on any path?
    private boolean is23() { return is23(root); }
    private boolean is23(Node x) {
        if (x == null) return true;
        if (isRed(x.right)) return false;
        if (x != root && isRed(x) && isRed(x.left))
            return false;
        return is23(x.left) && is23(x.right);
    } 

    // do all paths from root to leaf have same number of black edges?
    private boolean isBalanced() { 
        int black = 0;     // number of black links on path from root to min
        Node x = root;
        while (x != null) {
            if (!isRed(x)) black++;
            x = x.left;
        }
        return isBalanced(root, black);
    }

    // does every path from the root to a leaf have the given number of black links?
    private boolean isBalanced(Node x, int black) {
        if (x == null) return black == 0;
        if (!isRed(x)) black--;
        return isBalanced(x.left, black) && isBalanced(x.right, black);
    } 


    /**
     * Unit tests the <tt>RedBlackBST</tt> data type.
     */
    public static void main(String[] args) { 
        RedBlackBST<String, Integer> st = new RedBlackBST<String, Integer>();
        for (int i = 0; !StdIn.isEmpty(); i++) {
            String key = StdIn.readString();
            st.put(key, i);
        }
        for (String s : st.keys())
            StdOut.println(s + " " + st.get(s));
        StdOut.println();
    }
}