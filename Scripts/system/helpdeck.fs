: HELPDECK ;

lf notedeck.fs

autoforget HelpDeck

class: HelpDeck extends NoteDeck

  NoteSchema opSchema
  NoteDef stackParamDef
  NoteDef memberTypeParamDef
  
  NoteSchema classSchema
  
  NoteSchema methodSchema
  
  NoteSchema memberSchema
  
  Note opsNote                          \ note for "help ops"
  Note classesNote                      \ note for "help classes"
  Note newestClassNote
  long curTags

  long kTagOp
  long kTagMethod
  long kTagClass
  long kTagMember
  
  byte separator

  m: init
    super.init("help")
  
    addParamDef("stack" "stack behavior") -> stackParamDef
  
    addSchema("opDef" "operator definition") -> opSchema
    opSchema.addOptionalParam(stackParamDef)

    addSchema("classDef" "class definition") -> classSchema

    addSchema("methodDef" "method definition") -> methodSchema
    methodSchema.addOptionalParam(stackParamDef)
    
    addSchema("memberDef" "member definition") -> memberSchema
    addParamDef("type" "member type") -> memberTypeParamDef
    memberSchema.addOptionalParam(memberTypeParamDef)
    
    addTagDef("op" "Operator" "An executable forth operation").mask -> kTagOp
    addTagDef("method" "Methods" "Class methods").mask -> kTagMethod
    addTagDef("class" "Builtin classes" "Convenience classes - arrays, lists, maps, strings, threads, etc.").mask -> kTagClass
    addTagDef("member" "Member" "Class member").mask -> kTagMember
    
    addNote("ops" "forth operators") -> opsNote
    addNote("classes" "builtin classes") -> classesNote
    
    '|' -> separator
  ;m
  
  m: delete
    oclear opSchema
    oclear stackParamDef
    oclear classSchema
    oclear methodSchema
    oclear newestClassNote
    oclear opsNote
    oclear classesNote
    oclear memberSchema
    oclear memberTypeParamDef
  ;m
  
  m: setTags
    ptrTo byte tagName
    NoteTagDef tagDef
    
    0l -> curTags
    begin
    while(blword?)
      -> tagName
      if(tagMap.grab(tagName))
        ->o tagDef
        tagDef.mask ->+ curTags  \ note: this assumes no duplicate tag values
        \ tagName %s %bl tagDef.mask %2x %bl
      else
        \ TODO: report error, tag not found
      endif
    repeat
    \ "tags set to " %s curTags %x %nl
  ;m
  
  m: getOpNote            \ NAME ...   false   OR   NOTE_OBJ true
    opsNote getChild
  ;m
  
  m: getClassNote            \ NAME ...   false   OR   NOTE_OBJ true
    classesNote getChild
  ;m
  
  \ addOp opName|stack behavior|description|extra tags
  m: addOp
    Note newNote
    curTags kTagOp l+ -> long tagMask
    ptrTo byte opName
    ptrTo byte tagName
  
    if(word?(separator))
      -> opName
      \ opName %s %nl
      addChild(opName "" opsNote) -> newNote
      
      \ look for stack behavior
      if(word?(separator))
        mko NoteParam newParam
        new String -> newParam.sval
        newParam.sval.set
        newNote.params.set(newParam stackParamDef.id)
        oclear newParam
      endif
      
      \ look for description
      if(word?(separator))
        newNote.body.set
      endif
      
      \ look for extra tags
      begin
      while(blword?)
        -> tagName
        if(getTag(tagName))
          <NoteTagDef>.mask ->+ tagMask  \ note: this assumes no duplicate tag values
        else
          error("addOp - extra tag not found")
        endif
        
      repeat
      
      tagMask -> newNote.tags
    else
      error("addOp - op name missing")
    endif
  
    oclear newNote  
  ;m
  
  \ addClass className|baseClass|description|tags
  m: addClass
    kTagClass -> long tags
    ptrTo byte className
    ptrTo byte tagName
  
    if(word?(separator))
      -> className
      \ className %s %nl
      addChild(className "" classesNote) -> newestClassNote
      
      \ look for baseClass
      if(word?(separator))
        \ TODO!
        drop
      endif
      
      \ look for description
      if(word?(separator))
        newestClassNote.body.set
      endif
      
      \ look for tags
      begin
      while(blword?)
        -> tagName
        if(getTag(tagName))
          <NoteTagDef>.mask ->+ tags  \ note: this assumes no duplicate tag values
        else
          error("addOp - extra tag not found")
        endif
        
      repeat
      
      tags -> newestClassNote.tags
    else
      error("addClass - class name missing")
    endif
  ;m
  
  \ addMethod methodName|stack behavior|description|extra tags
  m: addMethod
    Note newMethodNote
    curTags kTagMethod l+ -> long tags
    ptrTo byte methodName
    ptrTo byte tagName
  
    if(word?(separator))
      -> methodName
      \ methodName %s %nl
      addChild(methodName "" newestClassNote) -> newMethodNote
      
      \ look for stack behavior
      if(word?(separator))
        mko NoteParam newParam
        new String -> newParam.sval
        newParam.sval.set
        newMethodNote.params.set(newParam stackParamDef.id)
        oclear newParam
      endif
      
      \ look for description
      if(word?(separator))
        newMethodNote.body.set
      endif
      
      \ look for extra tags
      begin
      while(blword?)
        -> tagName
        if(getTag(tagName))
          <NoteTagDef>.mask ->+ tags  \ note: this assumes no duplicate tag values
        else
          error("addOp - extra tag not found")
        endif
        
      repeat
      
      tags -> newMethodNote.tags
    else
      error("addMethod - method name missing")
    endif
  
    oclear newMethodNote  
  ;m
  
  \ addMember memberName|type|description|extra tags
  m: addMember
    Note newMemberNote
    kTagMember -> long tags
    ptrTo byte memberName
    ptrTo byte tagName
  
    if(word?(separator))
      -> memberName
      \ memberName %s %nl
      addChild(memberName "" newestClassNote) -> newMemberNote
      
      \ look for type
      if(word?(separator))
        drop \ TODO!
      endif
      
      \ look for description
      if(word?(separator))
        newMemberNote.body.set
      endif
      
      \ look for extra tags
      begin
      while(blword?)
        -> tagName
        if(getTag(tagName))
          <NoteTagDef>.mask ->+ tags  \ note: this assumes no duplicate tag values
        else
          error("addOp - extra tag not found")
        endif
        
      repeat
      
      tags -> newMemberNote.tags
    else
      error("addMember - member name missing")
    endif
  
    oclear newMemberNote  
  ;m
  
  m: listOperators
    Note foundNote
    notes.headIter -> ArrayIter iter
    
    begin
    while(iter.next)
      ->o foundNote
      if(foundNote.tags kTagOp land l0<>)
        foundNote.name.get %s " - " %s foundNote.body.get %s %nl
        \ foundNote.name.get %s %bl foundNote.tags %2x " - " %s foundNote.body.get %s %nl
      endif
    repeat
    oclear iter
  ;m  

  m: listClasses
    Note foundNote
    notes.headIter -> ArrayIter iter
    
    begin
    while(iter.next)
      ->o foundNote
      if(foundNote.tags kTagClass land l0<>)
        foundNote.name.get %s " - " %s foundNote.body.get %s %nl
      endif
    repeat
    oclear iter
  ;m
  
  m: listSections
    NoteTagDef tagDef
    tags.headIter -> ArrayIter iter
    
    begin
    while(iter.next)
      ->o tagDef
      tagDef.name.get %s " - " %s tagDef.description.get %s %nl
    repeat
    oclear iter
  ;m
  
  m: help
    Note foundNote
    NoteParam stackBehavior
    
    if(blword?)
      -> ptrTo byte helpTarget
      if(strcmp(helpTarget "classes") 0=)
      
        listClasses
        
      else
      
        if(getChild(helpTarget classesNote))
          \ help CLASSNAME
          ->o foundNote
          helpTarget %s " - " %s foundNote.body.get %s %nl
         
          getLinkIter(foundNote) -> NoteLinkIter linkIter
          begin
          while(linkIter.next(kLTChild))
            -> int childNodeId
            notes.get(childNodeId) ->o Note methodNote
            "   " %s methodNote.name.get %s " - " %s methodNote.body.get %s %nl
            if(methodNote.getParam(stackParamDef.id))
              ->o stackBehavior
              "      " %s stackBehavior.sval.get %s %nl
            endif
          repeat
          oclear linkIter
        elseif(tagMap.grab(helpTarget))
          \ help SECTIONNAME
          -> NoteTagDef tagDef
          notes.headIter -> ArrayIter noteIter
          begin
          while(noteIter.next)
            ->o foundNote
            if(and(foundNote.tags tagDef.mask))
              foundNote.name.get %s " - " %s foundNote.body.get %s %nl
            endif
          repeat
        elseif(getChild(helpTarget opsNote))
          \ help OPNAME
          ->o foundNote
          helpTarget %s " - " %s foundNote.body.get %s %nl
          if(foundNote.getParam(stackParamDef.id))
              ->o stackBehavior
              stackBehavior.sval.get %s %nl
          endif
        else
            "help - " %s helpTarget %s " not found\n" %s
        endif
      endif
    else
      "=========== Sections ===========\n" %s
      listSections
      "=========== Operators ===========\n" %s
      listOperators
      "=========== Classes ===========\n" %s
      listClasses
    endif
  ;m
  
  m: check
    system.getVocabByName("forth") -> Vocabulary forthVoc
    forthVoc.getValueLength -> int valueLength
    forthVoc.headIter -> VocabularyIter iter
    mko String opName
    begin
    while(iter.next)
      valueLength + -> ptrTo byte pEntry
      opName.setBytes(pEntry 1+ pEntry c@)
      if(getChild(opName.get opsNote))
        drop
      else
        opName.get %s " not found in help.\n" %s
      endif
      
    repeat
    oclear forthVoc
    oclear opName
    oclear iter
  ;m

;class

mko HelpDeck helpDeck
helpDeck.init

: cleanupHelp
  oclear helpDeck
;

: check
  helpDeck.check
;

: mkTag
  helpDeck.addTagDef(blword $word(helpDeck.separator) $word(0)) drop
;

\ op, method, class and member tags are defined in HelpDeck:init

mkTag control Control|Ops which change the flow of execution
mkTag immediate Immediate|Ops which are executed at compile time
mkTag classdef Class defining|Ops which are used to define classes or class instances
mkTag method Methods|Class methods
mkTag struct Structure defining|Ops which are used to define structures
mkTag stack Parameter stack manipulation|Ops which manipulate the parameter stack
mkTag logic Logical|Ops which do logic operations
mkTag math Arithmetic|Ops which do math

mkTag int 32-bit integer|Ops for 32-bit integers
mkTag sfloat Single-precision floating-point|Ops for 32-bit floating point numbers
mkTag float Double-precision floating-point|Ops for 64-bit floating point numbers
mkTag long 64-bit integer|Ops for 64-bit integers
mkTag string String|Ops for strings

mkTag print Printing|Ops which output to the console
mkTag format Formatting|Ops which format text
mkTag internal Internal|Forth compilation internals
mkTag defining Defining words|Ops for defining ops and functions
mkTag native Native type defining words|Ops for defining variables or fields of native types - int, sfloat, double, etc.
mkTag compare Comparison|Compare numeric values
mkTag convert Conversion|Conversion between numeric types
mkTag memory Memory|Memory fetch and store
mkTag file File|Ops for files
mkTag dir Directory|Ops for directories
mkTag system System|System ops
mkTag if If|Part of if statement
mkTag case Case|Part of case statement
mkTag loop Loop|Part of do or begin loops
mkTag thread Thread|Ops related to threads

\ add string compile if loop case except
\ ? short byte unsigned

: tags:
  helpDeck.setTags
;

: addOp
  helpDeck.addOp
;

: addClass
  helpDeck.addClass
;

: addMethod
  helpDeck.addMethod
;

: addMember
  helpDeck.addMember
;

: help
  helpDeck.help
;

tags: control
addOp if|BOOL ...|start if statement|if
addOp andif|BOOL ...|add andif clause|if
addOp orif|BOOL ...|add orif clause|if
addOp else||start else branch of if statement|if
addOp elseif||start elseif branch of if statement|if
addOp endif||end if statement|if
addOp begin||begin loop statement|loop
addOp until|BOOL ...|loop back to "begin" if BOOL is false|loop
addOp while|BOOL ...|exit loop after "repeat" if BOOL is false|loop
addOp repeat||end loop statement|loop
addOp again||loop back to "begin"|loop
addOp do|ENDVAL STARTVAL ...|start do loop, ends at ENDVAL-1|loop
addOp loop||end do loop|loop
addOp +loop|INCREMENT ...|end do loop, add INCREMENT to index each time|loop
addOp case|TESTVAL ... |begin case statement|case
addOp of|CASEVAL ... |begin one case branch|case
addOp endof||end case branch|case
addOp endcase|TESTVAL ...|end all cases - drops TESTVAL|case
addOp abort||terminate execution with fatal error

tags: stack
addOp drop|VAL ...|drop top element of param stack
addOp dup|A ... A A|duplicate top of stack
addOp ?dup|VARIABLE|duplicate TOS if non 0
addOp swap|A B ... B A|swap top two elements of stack
addOp over|A B ... A B A|copy second stack element to top of stack
addOp rot|A B C ... B C A|rotate third stack element to top of stack
addOp -rot|A B C ... C A B|
addOp nip|A B C ... A C|
addOp tuck|A B C ... A C B C|
addOp pick|A ... PS(A)|
addOp roll|N ...|rolls Nth item to top of params stack
addOp sp|... PARAM_STACK_PTR|
addOp s0|... EMPTY_PARAM_STACK_PTR|
addOp fp|... LOCAL_VAR_FRAME_PTR|
addOp 2dup|DA ... DA DA|
addOp 2swap|DA DB ... DB DA|
addOp 2drop|DA ...|
addOp 2over|DA DB ... DA DB DA|
addOp 2rot|DA DB DC ... DB DC DA|
addOp r[||push SP on rstack, used with ]r to count a variable number of arguments
addOp ]r||remove old SP from rstack, push count of elements since r[ on TOS

tags: internal
addOp _doDoes||compiled at start of "does" section of words created by a builds...does word
addOp _doVariable||compiled at start of words defined by "variable"
addOp _doConstant||compiled at start of words defined by "constant"
addOp _doDConstant||compiled at start of words defined by "dconstant"
addOp _endBuilds||compiled at end of "builds" section
addOp done||makes inner interpreter return - used by outer interpreter
addOp _doByte||compiled at start of byte global vars
addOp _doShort||compiled at start of short global vars
addOp _doInt||compiled at start of int global vars
addOp _doFloat||compiled at start of sfloat global vars
addOp _doDouble||compiled at start of float global vars
addOp _doString||compiled at start of string global vars
addOp _doOp||compiled at start of opcode global vars
addOp _doLong||compiled at start of long global vars
addOp _doObject||compiled at start of object global vars
addOp _doUByte||compiled at start of unsigned byte global vars
addOp _doUShort||compiled at start of unsigned short global vars
addOp _exit||compiled at end of user definitions which have no local vars
addOp _exitL||compiled at end of user definitions which have local vars
addOp _exitM||compiled at end of method definitions which have no local vars
addOp _exitML||compiled at end of method definitions which have local vars
addOp _doVocab||compiled at start of vocabularies
addOp _doByteArray||compiled at start of byte global arrays
addOp _doShortArray||compiled at start of short global arrays
addOp _doIntArray||compiled at start of int global arrays
addOp _doFloatArray||compiled at start of sfloat global arrays
addOp _doDoubleArray||compiled at start of float global arrays
addOp _doStringArray||compiled at start of string global arrays
addOp _doOpArray||compiled at start of opcode global arrays
addOp _doLongArray||compiled at start of 64-bit global arrays
addOp _doObjectArray||compiled at start of object global arrays
addOp _doUByteArray||compiled at start of opcode global arrays
addOp _doUShortArray||compiled at start of opcode global arrays
addOp initString||compiled when a local string variable is declared
addOp initStringArray||compiled when a local string array is declared
addOp badOp||set bad opcode error
addOp _doStruct||compiled at start of opcode global arrays
addOp _doStructArray||compiled at start of global structure array instances, next word is padded element length
addOp _doStructType||compiled at the start of each user-defined structure defining word 
addOp _doClassType||compiled at the start of each user-defined class defining word 
addOp _doEnum||compiled at start of enum defining word, acts just like 'int'
addOp _do||compiled at start of do loop
addOp _loop||compiled at end of do loop
addOp _+loop||compiled at end of do +loop
addOp _doNew|CLASS_TYPE_INDEX ... NEW_OBJECT|compiled by 'new', takes class type index, executes class 'new' operator|classdef
addOp _allocObject|CLASS_VOCAB_PTR...NEW_OBJECT|default class 'new' operator, mallocs spaces for a new object instance and pushes new object|classdef

tags: int math
addOp +|A B ... (A+B)|add top two items
addOp -|A B ... (A-B)|subtract top two items
addOp *|A B ... (A*B)|mutliply top two items
addOp 2*|A ... (A*2)|multiply top item by 2
addOp 4*|A ... (A*4)|multiply top item by 4
addOp 8*|A ... (A*8)|multiply top item by 8
addOp /|A B ... (A/B)|divide top two items
addOp 2/|A ... (A/2)|divide top item by 2
addOp 4/|A ... (A/4)|divide top item by 4
addOp 8/|A ... (A/8)|divide top item by 4
addOp /mod|A B ... (A/B) (A mod B)|divide top two items, return quotient & remainder
addOp mod|A B ... (A mod B)|take modulus of top two items
addOp negate|A ... (-A)|negate top item

tags: int compare
addOp =|A B ... A=B|
addOp <>|A B ... A<>B|
addOp >|A B ... A>B|
addOp >=|A B ... A>=B|
addOp <|A B ... A<B|
addOp <=|A B ... A<=B|
addOp 0=|A ... A=0|
addOp 0<>|A ... A<>0|
addOp 0>|A ... A>0|
addOp 0>=|A ... A>=0|
addOp 0<|A ... A<0|
addOp 0<=|A ... A<=0|
addOp u>|A B ... A>B|unsigned comparison
addOp u<|A B ... A<B|unsigned comparison
addOp within|VAL LO HI ... (LO<=VAL<HI)|
addOp min|A B ... min(A,B)|
addOp max|A B ... max(A,B)|

tags: sfloat math
addOp sf+|FA FB ... (FA+FB)|add top two floating point items
addOp sf-|FA FB ... (FA-FB)|subtract top two floating point items
addOp sf*|FA FB ... (FA*FB)|multiply top two floating point items
addOp sf/|FA FB ... (FA/FB)|divide top two floating point items
addOp sfsin|FA ... sin(FA)|sfloat sine
addOp sfarcsin|FA ... arcsin(FA)|sfloat arcsine
addOp sfcos|FA ... cos(FA)|sfloat cosine
addOp sfarccos|FA ... arccos(FA)|sfloat arccosine
addOp sftan|FA ... tan(FA)|sfloat tan
addOp sfarctan|FA ... arctan(FA)|sfloat arctan
addOp sfarctan2|FA FB ... arctan(FA/FB)|sfloat arctan of ratio
addOp sfexp|FA ... exp(FA)|
addOp sfln|FA ... ln(FA)|
addOp sflog10|FA ... log10(FA)|
addOp sfpow|FA FB ... FA**FB|
addOp sfsqrt|FA ... sqrt(FA)|
addOp sfceil|FA ... ceil(FA)|
addOp sffloor|FA ... floor(FA)|
addOp sfabs|FA ... abs(FA)|
addOp sfldexp|FA B ... ldexp(FA,B)|
addOp sffrexp|FA ... frac(FA) exponent(FA)|
addOp sfmodf|FA ... frac(FA) whole(FA)|
addOp sffmod|FA FB ... fmod(FA,FB)|

tags: sfloat compare
addOp sf=|FA FB ... FA=FB|
addOp sf<>|FA FB ... FA<>FB|
addOp sf>|FA FB ... FA>FB|
addOp sf>=|FA FB ... FA>=FB|
addOp sf<|FA FB ... FA<FB|
addOp sf<=|FA FB ... FA<=FB|
addOp sf0=|FA ... FA=0|
addOp sf0<>|FA ... FA<>0|
addOp sf0>|FA ... FA>0|
addOp sf0>=|FA ... FA>=0|
addOp sf0<|FA ... FA<0|
addOp sf0<=|FA ... FA<=0|
addOp sfwithin|FVAL FLO FHI ... (FLO<=FVAL<FHI)|
addOp sfmin|FA FB... min(FA,FB)|
addOp sfmax|FA FB... max(FA,FB)|

tags: sfloat block
addOp sfAddBlock|SRCA SRCB DST NUM ...|add blocks of NUM floats at SRCA and SRCB and store results in DST
addOp sfSubBlock|SRCA SRCB DST NUM ...|subtract blocks of NUM floats at SRCA and SRCB and store results in DST
addOp sfMulBlock|SRCA SRCB DST NUM ...|multiply blocks of NUM floats at SRCA and SRCB and store results in DST
addOp sfDivBlock|SRCA SRCB DST NUM ...|divide blocks of NUM floats at SRCA and SRCB and store results in DST
addOp sfScaleBlock|SRC DST SCALE NUM ...|multiply block of NUM floats at SRC by SCALE and store results in DST
addOp sfOffsetBlock|SRC DST OFFSET NUM ...|add OFFSET to block of NUM floats at SRC and store results in DST
addOp sfMixBlock|SRC DST SCALE NUM ...|multiply block of NUM floats at SRC by SCALE and add results into DST

tags: float math
addOp d+|DA DB ... (DA+DB)|add top two double floating point items
addOp d-|DA DB ... (DA-DB)|subtract top two double floating point items
addOp d*|DA DB ... (DA*DB)|multiply top two double floating point items
addOp d/|DA DB ... (DA/DB)|divide top two double floating point items
addOp dsin|DA ... sin(DA)|double sine
addOp darcsin|DA ... arcsin(DA)|double arcsine
addOp dcos|DA ... cos(DA)|double cosine
addOp darccos|DA ... arccos(DA)|double arccosine
addOp dtan|DA ... tan(DA)|double tan
addOp darctan|DA ... arctan(DA)|double arctan
addOp darctan2|DA DB ... arctan(DA/DB)|double arctan of ratio
addOp dexp|DA ... exp(DA)|
addOp dln|DA ... ln(DA)|
addOp dlog10|DA ... log10(DA)|
addOp dpow|DA DB ... DA**DB|
addOp dsqrt|DA ... sqrt(DA)|
addOp dceil|DA ... ceil(DA)|
addOp dfloor|DA ... floor(DA)|
addOp dabs|DA ... abs(DA)|
addOp dldexp|DA B ... ldexp(DA,B)|
addOp dfrexp|DA ... frac(DA) exponent(DA)|
addOp dmodf|DA ... frac(DA) whole(DA)|
addOp dfmod|DA DB ... fmod(DA,DB)|

tags: float compare
addOp f=|DA DB ... DA=DB|
addOp f<>|DA DB ... DA<>DB|
addOp f>|DA DB ... DA>DB|
addOp f>=|DA DB ... DA>=DB|
addOp f<|DA DB ... DA<DB|
addOp f<=|DA DB ... DA<=DB|
addOp f0=|DA ... DA=0|
addOp f0<>|DA ... DA<>0|
addOp f0>|DA ... DA>0|
addOp f0>=|DA ... DA>=0|
addOp f0<|DA ... DA<0|
addOp f0<=|DA ... DA<=0|
addOp fwithin|DVAL DLO DHI ... (DLO<=DVAL<DHI)|
addOp fmin|DA DB ... min(DA,DB)|
addOp fmax|DA DB ... max(DA,DB)|

tags: float block
addOp fAddBlock|SRCA SRCB DST NUM ...|add blocks of NUM doubles at SRCA and SRCB and store results in DST
addOp fSubBlock|SRCA SRCB DST NUM ...|subtract blocks of NUM doubles at SRCA and SRCB and store results in DST
addOp fMulBlock|SRCA SRCB DST NUM ...|multiply blocks of NUM doubles at SRCA and SRCB and store results in DST
addOp fDivBlock|SRCA SRCB DST NUM ...|divide blocks of NUM doubles at SRCA and SRCB and store results in DST
addOp fScaleBlock|SRC DST SCALE NUM ...|multiply block of NUM doubles at SRC by SCALE and store results in DST
addOp fOffsetBlock|SRC DST OFFSET NUM ...|add OFFSET to block of NUM doubles at SRC and store results in DST
addOp fMixBlock|SRC DST SCALE NUM ...|multiply block of NUM doubles at SRC by SCALE and add results into DST

tags: convert
addOp i2f|A ... sfloat(A)|convert int to sfloat|int sfloat
addOp i2d|A ... float(A)|convert int to float|int float
addOp f2i|A ... int(A)|convert sfloat to int|int sfloat
addOp f2d|A ... float(A)|convert sfloat to float|sfloat float
addOp d2i|A ... int(A)|convert float to int|int float
addOp d2f|A ... sfloat(A)|convert float to sfloat|float sfloat
addOp i2l|INTA ... LONGA|convert signed 32-bit int to signed 64-bit int|int long
addOp l2f|LONGA ... FLOATA|convert signed 64-bit int to 32-bit sfloat|long sfloat
addOp l2d|LONGA ... DOUBLEA|convert signed 64-bit int to 64-bit sfloat|long float
addOp f2l|FLOATA ... LONGA|convert 32-bit sfloat to signed 64-bit int|sfloat long
addOp d2l|DOUBLEA ... LONGA|convert 64-bit sfloat to signed 64-bit int|float long

tags: logic
addOp or|A B ... or(A,B)|
addOp and|A B ... and(A,B)|
addOp xor|A B ... xor(A,B)|
addOp invert|A ... ~A|
addOp lshift|A B ... A<<B|
addOp rshift|A B ... A>>B|
addOp urshift|A B ... A>>B|unsigned right shift
addOp not|A ... not(A)|true iff A is 0, else false
addOp true|... -1|
addOp false|... 0|
addOp null|... 0|

tags: rstack
addOp >r|A ...|pushes top of param stack on top of return stack
addOp r>|... A|pops top of return stack to top of param stack
addOp rdrop||drops top of return stack
addOp rp|... RETURN_STACK_PTR|return pointer to top of return stack
addOp r0|... EMPTY_RETURN_STACK_PTR|return pointer to bottom of return stack (top if empty)

tags: memory
addOp @|PTR ... A|fetches longword from address PTR
addOp 2@|PTR ... DA|fetch double from address PTR
addOp ref|ref VAR ... PTR|return address of VAR
addOp !|A PTR ...|stores longword A at address PTR
addOp c!|A PTR ...|stores byte A at address PTR
addOp c@|PTR ... A|fetches unsigned byte from address PTR
addOp sc@|PTR ... A|fetches signed byte from address PTR
addOp c2i|A ... LA|sign extends byte to long
addOp w!|A PTR ...|stores word A at address PTR
addOp w@|PTR ... A|fetches unsigned word from address PTR
addOp sw@|PTR ... A|fetches signed word from address PTR
addOp w2i|WA ... LA|sign extends word to long
addOp 2!|DA PTR ...|store double at address PTR
addOp move|SRC DST N ...|copy N bytes from SRC to DST
addOp fill|DST N A ...|fill N bytes at DST with byte value A
addOp varAction!|A varAction! ...|set varAction to A (use not recommended)
addOp varAction@|varAction@ ... A|fetch varAction (use not recommended)

tags: string
addOp strcpy|STRA STRB ...|copies string from STRB to STRA
addOp strncpy|STRA STRB N ...|copies up to N chars from STRB to STRA
addOp strlen|STR ... LEN|returns length of string at STR
addOp strcat|STRA STRB ...|appends STRB to string at STRA
addOp strncat|STRA STRB N ...|appends up to N chars from STRB to STRA
addOp strchr|STR CHAR ... PTR|returns ptr to first occurence of CHAR in string STR
addOp strrchr|STR CHAR ... PTR|returns ptr to last occurence of CHAR is string STR
addOp strcmp|STRA STRB ... V|returns 0 iff STRA = STRB, else result of last char comparison
addOp stricmp|STRA STRB ... V|returns 0 iff STRA = STRB ignoring case, else result of last char comparison
addOp strncmp|STRA STRB N ... V|returns 0 iff STRA = STRB ignoring case, else result of last char comparison, for first N chars
addOp strstr|STRA STRB ... PTR|returns ptr to first occurence of STRB in string STRA
addOp strtok|STRA STRB ... PTR|returns ptr to next token in STRA, delimited by a char in STRB, modifies STRA, pass 0 for STRA after first call

tags: file
addOp fopen|PATH_STR ATTRIB_STR ... FILE|open file
addOp fclose|FILE ... RESULT|close file
addOp fseek|FILE OFFSET CTRL ... RESULT|seek in file, CTRL: 0 from start, 1 from current, 2 from end 
addOp fread|NITEMS ITEMSIZE FILE ... RESULT|read items from file
addOp fwrite|NITEMS ITEMSIZE FILE ... RESULT|write items to file
addOp fgetc|FILE ... CHARVAL|read a character from file
addOp fputc|CHARVAL FILE ... RESULT|write a character to file
addOp feof|FILE ... RESULT|check if file is at end-of-file
addOp fexists|PATH_STR ... RESULT|check if file exists
addOp ftell|FILE ... OFFSET|return current read/write position in file
addOp flen|FILE ... FILE_LENGTH|return length of file
addOp fgets|BUFFER MAXCHARS FILE ... NUMCHARS|read a line of up to MAXCHARS from FILE into BUFFER
addOp fputs|BUFFER FILE ...|write null-terminated string from BUFFER to FILE
addOp stdin|... FILE|get standard in file
addOp stdout|... FILE|get standard out file
addOp stderr|... FILE|get standard error file
addOp findResource|FILENAME ... PATH_PTR|return directory holding FILENAME in paths from FORTH_RESOURCES

tags: long math
addOp l+|LA LB ... (LA+LB)|
addOp l-|LA LB ... (LA-LB)|
addOp l*|LA LB ... (LA*LB)|
addOp l/|LA LB ... (LA/LB)|
addOp lmod|LA LB ... (LA mod LB)|
addOp l/mod|A B ... (LA/LB) (LA mod LB)|divide top two items, return quotient & remainder
addOp lnegate|A ... (-LA)|negate top item

tags: long compare
addOp l=|LA LB ... LA=LB|
addOp l<>|LA LB ... LA<>LB|
addOp l>|LA LB ... LA>LB|
addOp l>=|LA LB ... LA>=LB|
addOp l<|LA LB ... LA<LB|
addOp l<=|LA LB ... LA<=LB|
addOp l0=|LA ... LA=0|
addOp l0<>|LA ... LA<>0|
addOp l0>|LA ... LA>0|
addOp l0>=|LA ... LA>=0|
addOp l0<|LA ... LA<0|
addOp l0<=|LA ... LA<=0|
addOp lwithin|LVAL LLO LHI ... (LLO<=LVAL<LHI)|
addOp lmin|LA LB ... min(LA,LB)|
addOp lmax|LA LB ... max(LA,LB)|

tags: except
addOp try||start of code section protected by exception handler
addOp except||start of exception handler code
addOp finally||defines code run after exception handler
addOp endtry||end of exception handler
addOp raise|EXCEPTION_NUM ...|raise an exception

tags: defining
addOp builds||starts a builds...does definition
addOp does||starts the runtime part of a builds...does definition
addOp exit||exit current hi-level op
addOp ;||ends a colon definition
addOp :||starts a colon definition
addOp f:||starts a function definition
addOp ;f||ends a function definition, leaves function opcode on stack
addOp code||starts an assembler definition
addOp create||starts a 
addOp variable|variable NAME|creates a variable op which pushes its address when executed
addOp constant|A constant NAME|creates a constant op which pushes A when executed
addOp 2constant|DA 2constant NAME|creates a double constant op which pushes DA when executed
addOp byte|byte VAR|declare a 8-bit integer variable or field, may be preceeded with initializer "VAL ->"
addOp ubyte|ubyte VAR|declare a unsigned 8-bit integer variable or field, may be preceeded with initializer "VAL ->"
addOp short|short VAR|declare a 16-bit integer variable or field, may be preceeded with initializer "VAL ->"
addOp ushort|ushort VAR|declare a unsigned 16-bit integer variable or field, may be preceeded with initializer "VAL ->"
addOp int|int VAR|declare a 32-bit integer variable or field, may be preceeded with initializer "VAL ->"
addOp uint|uint VAR|declare a unsigned 32-bit integer variable or field, may be preceeded with initializer "VAL ->"
addOp long|long VAR|declare a 64-bit int variable or field, may be preceeded with initializer "VAL ->"
addOp ulong|ulong VAR|declare a 64-bit unsigned int variable or field, may be preceeded with initializer "VAL ->"
addOp sfloat|sfloat VAR|declare a 32-bit floating point variable or field, may be preceeded with initializer "VAL ->"
addOp float|float VAR|declare a 64-bit floating point variable or field, may be preceeded with initializer "VAL ->"
addOp string|MAXLEN string NAME declare a string variable or field with a specified maximum length
addOp op|op VAR|declare a forthop variable or field
addOp void|returns void declare that a method returns nothing ?does this work?
addOp arrayOf|NUM arrayOf TYPE declare an array variable or field with NUM elements of specified TYPE
addOp ptrTo|ptrTo TYPE declare a pointer variable or field of specified TYPE
addOp struct:|struct: NAME|start a structure type definition
addOp ;struct||end a structure type definition
addOp union||within a structure definition, resets the field offset to 0
addOp sizeOf|sizeOf STRUCT_NAME ... STRUCT_SIZE_IN_BYTES|
addOp offsetOf|offsetOf STRUCT_NAME FIELD_NAME ... OFFSET_OF_FIELD_IN_BYTES|

addOp class:|class: NAME|start a class type definition|classdef
addOp ;class||end a class definition|classdef
addOp m:|m: NAME|start a class method definition|classdef
addOp ;m ||end a class method definition|classdef
addOp returns|returns TYPE|specify the return type of a method, can only occur in a class method definition|classdef
addOp doMethod|OBJECT METHOD# doMethod|execute specified method on OBJECT
addOp implements:|implements: NAME|start an interface definition within a class definition|classdef
addOp ;implements||ends an interface definition|classdef

addOp extends|extends NAME|within a class/struct definition, declares the parent class/struct|classdef
addOp new|new TYPE ... OBJECT|creates an object of specified type
addOp initMemberString|initMemberString NAME|used inside a method, sets the current and max len fields of named member string
addOp enum:|enum: NAME|starts an enumerated type definition
addOp ;enum||ends an enumerated type definition
addOp recursize||used inside a colon definition, allows it to invoke itself recursively
addOp precedence||used inside a colon definition, makes it have precedence (execute in compile mode)
addOp load|load PATH|start loading a forth source file whose name immediately follows "load"
addOp $load|PATH $load ...|start loading a forth source file whose name is on TOS
addOp loaddone||terminate loading a forth source file before the end of file
addOp requires|requires NAME|if forthop NAME exists do nothing, if not load NAME.txt
addOp $evaluate|STR $evaluate|interpret string on TOS
addOp ]||sets state to compile
addOp [||sets state to interpret
addOp state|... @STATE_ADDR|leaves address of "state" var on TOS
addOp '|' NAME|push opcode of NAME on TOS (does not have precedence)
addOp postpone|postpone NAME|compile opcode of NAME, including ops which have precedence
addOp compile|compile OPCODE|compile immediately following opcode (opcode must not have precedence)
addOp [']|['] NAME|push opcode of NAME (has precedence)

tags: print
addOp .|NUM ...|prints number in current base followed by space
addOp .2|LNUM ...|prints 64-bit number in current base followed by space
addOp %d|NUM ...|prints number in decimal
addOp %x|NUM ...|prints number in hex
addOp %2d|LNUM ...|prints 64-bit number in decimal
addOp %2x|LNUM ...|prints 64-bit number in hex
addOp %s|STRING ...|prints string
addOp %c|CHARVAL ...|prints character
addOp %nc|NUM CHARVAL ...|prints character NUM times
addOp spaces|NUM ...|prints NUM spaces
addOp type|STRING NCHARS ...|print a block of NCHARS characters of text
addOp %4c|NUM ...|prints 32-bit int as 4 characters
addOp %8c|LNUM ...|prints 64-bit int as 8 characters
addOp %bl||prints a space
addOp %nl||prints a newline
addOp %f|FPNUM ...|prints a single-precision floating point number with %f format
addOp %2f|DOUBLE_FPNUM ...|prints a double-precision floating point number with %f format
addOp %g|FPNUM ...|prints a single-precision floating point number with %g format
addOp %2g|DOUBLE_FPNUM ...|prints a double-precision floating point number with %g format
addOp format|ARG FORMAT_STRING ... STRING_ADDR|format a 32-bit numeric value
addOp 2format|ARG64 FORMAT_STRING ... STRING_ADDR|format a 64-bit numeric value
addOp addTempString|STRING NUM ... TSTRING|allocate a temporary string - STRING may be null or NUM may be 0
addOp atoi|STRING ... NUM|convert string to integer
addOp atof|STRING ... DOUBLE|convert string to double-precision floating point
addOp fprintf|FILE FORMAT_STRING (ARGS) NUM_ARGS ... RESULT|print formatted string to file
addOp snprintf|BUFFER BUFFER_SIZE FORMAT_STRING (ARGS) NUM_ARGS ... RESULT|print formatted string to BUFFER
addOp fscanf|FILE FORMAT_STRING (ARGS) NUM_ARGS ... RESULT|read formatted string from file
addOp sscanf|BUFFER FORMAT_STRING (ARGS) NUM_ARGS ... RESULT|read formatted string from BUFFER
addOp base|leaves address of "base" var on TOS
addOp octal|sets current base to 8
addOp decimal|sets current base to 10
addOp hex|sets current base to 16
addOp printDecimalSigned|makes all decimal printing signed, printing in any other base is unsigned 
addOp printAllSigned|makes printing in any base signed
addOp printAllUnsigned|makes printing in any base unsigned

tags: thread
addOp thisThread|... THREAD_OBJECT|get currently running thread
addOp thisFiber|... FIBER_OBJECT|get currently running fiber
addOp yield|...|current fiber yields to any other ready fiber

tags: dll
addOp DLLVocabulary|DLLVocabulary VOCAB_NAME DLL_FILEPATH|creates a new DLL vocabulary, also loads the dll
addOp addDLLEntry|NUM_ARGS "ENTRY_NAME" ...|adds a new entry point to current definitions DLL vocabulary
addOp addDLLEntryEx|NUM_ARGS "OP_NAME" "ENTRY_NAME" ...|adds a new entry point to current definitions DLL vocabulary
addOp DLLVoid||when used prior to dll_0...dll_15, newly defined word will return nothing on TOS


\ TODO: tag these
tags:
addOp lit|... IVAL|pushes longword which is compiled immediately after it
addOp flit|... FVAL|pushes sfloat which is compiled immediately after it
addOp dlit|... DVAL|pushes float which is compiled immediately after it

addOp ->|V -> VAR|store V in VAR
addOp ->+|N ->+ VAR ...|add N to VAR, append for strings
addOp ->-|N ->- VAR ...|subtract N from VAR

addOp this|... OBJECT|get value of THIS object

addOp execute|OP ...|execute op which is on TOS
addOp call|IP ...|rpushes current IP and sets IP to that on TOS
addOp goto|IP ...|sets IP to that on TOS
addOp i|... LOOPINDEX_I|pushes innermost doloop index
addOp j|... LOOPINDEX_J|pushes next innermost doloop index
addOp unloop||remove do-loop info from rstack, use this before an 'exit' inside a do-loop
addOp leave||exit a do-loop continuing just after the 'loop'
addOp here|... DP|returns DP

addOp forth||overwrites the top of the vocabulary stack with forth vocabulary
addOp definitions||makes the top of the vocabulary stack be the destination of newly defined words
addOp vocabulary|vocabulary|VOCAB create a new vocabulary
addOp also||duplicates top of vocabulary stack, use "also vocab1" to add vocab1 to the stack above current vocab
addOp previous||drops top of vocabulary stack
addOp only||sets the vocabulary stack to just one element, forth, use "only vocab1" to make vocab1 the only vocab in stack
addOp forget|forget WORDNAME|forget named word and all newer definitions
addOp autoforget|autoforget WORDNAME|forget named word and all newer definitions, don't report error if WORDNAME is not defined yet
addOp vlist||display current search vocabulary
addOp find||

addOp align||aligns DP to a lonword boundary
addOp allot|A ...|adds A to DP, allocating A bytes
addOp ,|A ...|compiles longword A
addOp c,|A ...|compiles byte A
addOp malloc|A ... PTR|allocates a block of memory with A bytes
addOp realloc|IPTR A ... OPTR|resizes block at IPTR to be A bytes long, leaves new address OPTR
addOp free|PTR ...|frees a block of memory

addOp getConsoleOut|... OBJECT|get console output stream
addOp getDefaultConsoleOut|... OBJECT|get default console output stream
addOp setConsoleOut|OBJECT ...|set console output stream
addOp resetConsoleOut||set console output stream to default

addOp toupper|CHAR ... UCHAR|change character to upper case
addOp isupper|CHAR ... RESULT|test if character is upper case
addOp isspace|CHAR ... RESULT|test if character is whitespace
addOp isalpha|CHAR ... RESULT|test if character is alphabetic
addOp isgraph|CHAR ... RESULT|test if character is graphic
addOp tolower|CHAR ... LCHAR|change character to lower case
addOp islower|CHAR ... RESULT|test if character is lower case

addOp outToFile|FILE ...|redirect output to file
addOp outToScreen||set output to screen (standard out)
addOp outToString|STRING ...|redirect output to string
addOp outToOp|OP ...|redirect output to forth op (op takes a single string argument, returns nothing)
addOp getConOutFile|... FILE|returns redirected output file

addOp blword|... STRING_ADDR|fetch next whitespace-delimited token from input stream, return its address
addOp $word|CHARVAL ... STRING_ADDR|fetch next token delimited by CHARVAL from input stream, return its address
addOp (|( COMMENT TEXT )|inline comment, ends at ')' or end of line
addOp features|... FEATURES|variable that allows you to enable and disable language features
addOp .features||displays what features are currently enabled
addOp source|... INPUT_BUFFER_ADDR LENGTH|return address of base of input buffer and its length
addOp >in|... INPUT_OFFSET_ADDR|return pointer to input buffer offset variable
addOp fillInputBuffer|PROMPT_STRING ... INPUT_BUFFER_ADDR|display prompt, fill input buffer & return input buffer address
addOp consumeInputBuffer|STRING_OBJ ...|get a line from current input and append to specified string
addOp emptyInputBuffer||empty the current input buffer
addOp time|... TIME_AS_INT64|
addOp strftime|BUFFADDR BUFFLEN FORMAT_STRING TIME_AS_INT64 ...|puts formatted string in buffer
addOp ms@|... TIME_AS_INT32|returns milliseconds since forth started running
addOp ms|MILLISECONDS ...|sleep for specified number of milliseconds

addOp rand|... VAL|returns pseudo-random number
addOp srand|VAL ...|sets pseudo-random number seed
addOp hash|PTR LEN HASHIN ... HASHOUT|generate a 32-bit hashcode for LEN bytes at PTR starting with initial hashcode HASHIN
addOp qsort|ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET ...|for strings, set COMPARE_TYPE to kBTString + (256 * maxLen)
addOp bsearch|KEY_ADDR ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET ... INDEX|(-1 if not found)

addOp dstack||display parameter stack
addOp drstack||display return stack
addOp system|STRING ... RESULT|pass DOS command line to system, returns exit code (0 means success)
addOp chdir|STRING ... RESULT|change current working directory, returns exit code (0 means success)
addOp remove|STRING ... RESULT|remove named file, returns exit code (0 means success)
addOp bye||exit forth
addOp argv|INDEX ... STRING_ADDR|return arguments from command line that started forth
addOp argc|... NUM_ARGUMENTS|return number of arguments from command line that started forth (not counting "forth" itself)
addOp turbo||switches between slow and fast mode
addOp stats||displays forth engine statistics
addOp describe|describe OPNAME|displays info on op, disassembles userops
addOp error|ERRORCODE ...|set the error code
addOp addErrorText|TEXT ...|add error text
addOp setTrace|FLAGS ...|turn tracing output on/off

addOp #if|BOOLVAL #if ... beginning of conditional compilation section
addOp #ifdef|#ifdef SYMBOL|beginning of conditional compilation section
addOp #ifndef|#ifndef SYMBOL|beginning of conditional compilation section
addOp #else|#else|begin else part of conditional compilation section
addOp #endif|#endif|end of conditional compilation sections

addOp CreateEvent|MANUALRESET INITIALSTATE NAMESTR ... HANDLE|
addOp CloseHandle|HANDLE ... RESULT|
addOp SetEvent|HANDLE ... RESULT|
addOp ResetEvent|HANDLE ... RESULT|
addOp PulseEvent|HANDLE ... RESULT|
addOp GetLastError|... LASTERROR|
addOp WaitForSingleObject|HANDLE TIMEOUT ... RESULT|
addOp WaitForMultipleObject|NUMHANDLES HANDLESPTR WAITALL TIMEOUT ... RESULT|
addOp InitializeCriticalSection|CRITSECTIONPTR ...|
addOp DeleteCriticalSection|CRITSECTIONPTR ...|
addOp EnterCriticalSection|CRITSECTIONPTR ...|
addOp LeaveCriticalSection|CRITSECTIONPTR ...|
addOp FindFirstFile|PATHSTR FINDDATA_PTR ... SEARCH_HANDLE|
addOp FindNextFile|SEARCH_HANDLE FINDDATA_PTR ... RESULT|
addOp FindClose|SEARCH_HANDLE ... RESULT|
addOp windowsConstants|... PTR_TO_CONSTANTS|
addOp dumpProfile|... dump opcode execution counts. start profiling with setTrace(1024)|
addOp resetProfile|...|reset opcode execution counts.

\ =============================================================================================

addClass Object||base object class
addMethod delete|...|delete the object - do not invoke this
addMethod show|...|show object contacts in json-like format
addMethod showInner|...|shows innards of object|internal
addMethod getClass|... CLASS_OBJECT|return the Class object this object is an instance of
addMethod compare|OBJECT ... -1/0/1|compare this object to OBJECT, used for Array sort method
addMethod keep|...|add 1 to this objects reference count
addMethod release|...|subtract 1 from this objects reference count

addClass Class|Object|Class object
addMethod create|... OBJECT|create a new instance of this class
addMethod getParent|... SUPERCLASS_OBJECT|get the class object for the parent of this class
addMethod getName|... CLASSNAME|get the class name string
addMethod getVocabulary|... VOCAB_OBJECT|get the class vocabulary object
addMethod getTypeIndex|... INT_TYPE_INDEX|get the class type index
addMethod setNew|OP ...|set 'new' operator for this class

addClass Iter|Object|Abstract iterator
addMethod seekNext|...|move iterator to next element
addMethod seekPrev|...|move iterator to previous element
addMethod seekHead|...|move iterator to first element
addMethod seekTail|...|move iterator to last element
addMethod atHead|... BOOL|true if at start of collection
addMethod atTail|... BOOL|true if at end of collection
addMethod next|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at last element, advance to next element
addMethod prev|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at first element, advance to previous element
addMethod current|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at last element
addMethod remove|...|remove the object in the associated collection at the iterator position
addMethod unref|... OBJECT|return the object in the associated collection at the iterator position and stick a null object in its place in collection
addMethod clone|... THIS_ITER|return a copy of this iterator

addClass Iterable|Object|Abstract iterable container class
addMethod headIter|... ITER_OBJECT|return an iterator positioned at start of this container
addMethod tailIter|... ITER_OBJECT|return an iterator positioned at end of this container
addMethod find|SOUGHT_THING ... ITER_OBJECT true   OR   false|return iterator positioned at SOUGHT_THING
addMethod clone|... CLONE_OBJECT|return a copy of this container including its contents
addMethod count|... count|return number of objects in this container
addMethod clear|...|empty container

addClass Array|Iterable|array of objects
addMethod find|OBJECT ... ITER_OBJECT true   OR   false|return iterator positioned at OBJECT plus true, or just false if not found in array
addMethod get|INDEX ... OBJECT|get array object at specified index
addMethod set|OBJECT INDEX ...|set array object at specified index
addMethod ref|INDEX ... OBJECT_PTR|return address of object at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap objects at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE objects
addMethod insert|OBJECT INDEX ...|insert OBJECT at INDEX
addMethod remove|INDEX ... OBJECT|remove OBJECT at specified index
addMethod push|OBJECT ...|add OBJECT to end of array
addMethod pop|... OBJECT|remove last OBJECT in array
addMethod base|... OBJECT_PTR|return address of first object in array
addMethod load|<N OBJECTS> N ...|set array to contain N objects on stack
addMethod fromMemory|OBJECT_PTR N OFFSET ...|copy N objects from memory at OBJECT_PTR into array starting at index OFFSET
addMethod findValue|OBJECT ... INDEX|return INDEX of object in array, return -1 if object not found
addMethod reverse|...|reverse order of elements in array
addMethod sort|...|sort array objects using their compare method - objects must have same type
addMethod toList|... NEW_LIST_OBJECT|create a List object with the same elements in the array
addMethod unref|INDEX ... OBJECT|return the object in the associated collection at INDEX and stick a null object in its place in collection

addClass ArrayIter|Iter|array iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|SOUGHT_OBJECT ... BOOL|return true if found, iterator is positioned at SOUGHT_OBJECT
addMethod insert|OBJECT ...|insert OBJECT in collection just before current iterator position
addMember parent|Array|Array object this iter is associated with

addClass Bag|Iterable|bag of objects
addMethod get|INDEX ... OBJECT LONG_TAG|get object,tag pair at specified index
addMethod geto|INDEX ... OBJECT|get object at specified index
addMethod gett|INDEX ... LONG_TAG|get tag at specified index
addMethod set|OBJECT LONG_TAG INDEX ...|set object,tag pair into position specified by index
addMethod ref|INDEX ... PAIR_PTR|get address of object,tag pair specified by index
addMethod swap|INDEX_I INDEX_J ...|swap pairs at specified indices
addMethod resize|NEW_SIZE ...|set bag size to NEW_SIZE pairs
addMethod insert|OBJECT LONG_TAG INDEX ...|insert pair at INDEX
addMethod remove|INDEX ... OBJECT LONG_TAG|remove pair at specified index
addMethod push|OBJECT LONG_TAG...|add pair to end of bag
addMethod pop|... OBJECT LONG_TAG|remove last pair in bar
addMethod base|... PAIR_PTR|return address of first pair in bag
addMethod load|<N PAIRS> N ...|set bag to contain N pairs on stack
addMethod fromMemory|PAIR_PTR N OFFSET ...|copy N pairs from memory at PAIR_PTR into bag starting at index OFFSET
addMethod find|LONG_TAG ... ITER_OBJECT true   OR   false|return iterator positioned at pair with tag plus true, or just false if not found in bag
addMethod findValue|LONG_TAG ... INDEX|return INDEX of pair with tag in array, return -1 if tag not found
addMethod grab|LONG_TAG ... OBJECT true   OR   false|return object with tag plus true, or just false if not found
addMethod toLongMap|... NEW_LMAP_OBJECT|create a LongMap object with same elements as bag
addMethod unref|INDEX ... OBJECT LONG_TAG|remove object from bag and unref, old position holds null object

addClass BagIter|Iter|bag iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|LONG_TAG ... BOOL|return true if found, iterator is positioned at pair matching LONG_TAG
addMethod insert|OBJECT LONG_TAG ...|insert OBJECT in collection just before current iterator position
addMember parent|Bag|Bag object this iter is associated with

addClass ByteArray|Iterable|array of bytes
addMethod get|INDEX ... BYTE_VALUE|get byte value at specified index
addMethod set|BYTE_VALUE INDEX ...|set byte value at specified index
addMethod ref|INDEX ... BYTE_PTR|return address of byte at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap bytes at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE bytes
addMethod insert|BYTE_VALUE INDEX ...|insert BYTE_VALUE at INDEX
addMethod remove|INDEX ... BYTE_VALUE|remove BYTE_VALUE at specified index
addMethod push|BYTE_VALUE ...|add BYTE_VALUE to end of array
addMethod pop|... BYTE_VALUE|remove last BYTE_VALUE in array
addMethod base|... BYTE_PTR|return address of first byte in array
addMethod load|<N BYTE_VALUEs> N ...|set array to contain N BYTE_VALUEs on stack
addMethod fromMemory|BYTE_PTR N OFFSET ...|copy N bytes from memory at BYTE_PTR into array starting at index OFFSET
addMethod find|BYTE_VALUE ... ITER_OBJECT true   OR   false|return iterator positioned at BYTE_VALUE plus true, or just false if not found in array
addMethod findValue|BYTE_VALUE ... INDEX|return INDEX of byte in array, return -1 if byte not found
addMethod reverse|...|reverse order of bytes in array
addMethod sort|...|sort array bytes
addMethod usort|...|sort array unsigned bytes
addMethod setFromString|STRING_PTR ...|resize array to hold string at STRING_PTR and copy it there

addClass ByteArrayIter|Iter|byte array iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|BYTE_VALUE ... BOOL|return true if found, iterator is positioned at matching byte
addMember parent|ByteArray|ByteArray object this iter is associated with

addClass ShortArray|Iterable|array of short (16-bit integer)
addMethod get|INDEX ... SHORT_VALUE|get short value at specified index
addMethod set|SHORT_VALUE INDEX ...|set short value at specified index
addMethod ref|INDEX ... SHORT_PTR|return address of short at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap shorts at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE shorts
addMethod insert|SHORT_VALUE INDEX ...|insert SHORT_VALUE at INDEX
addMethod remove|INDEX ... SHORT_VALUE|remove SHORT_VALUE at specified index
addMethod push|SHORT_VALUE ...|add SHORT_VALUE to end of array
addMethod pop|... SHORT_VALUE|remove last SHORT_VALUE in array
addMethod base|... SHORT_PTR|return address of first short in array
addMethod load|<N SHORT_VALUEs> N ...|set array to contain N SHORT_VALUEs on stack
addMethod fromMemory|SHORT_PTR N OFFSET ...|copy N shorts from memory at SHORT_PTR into array starting at index OFFSET
addMethod find|SHORT_VALUE ... ITER_OBJECT true   OR   false|return iterator positioned at SHORT_VALUE plus true, or just false if not found in array
addMethod findValue|SHORT_VALUE ... INDEX|return INDEX of short in array, return -1 if short not found
addMethod reverse|...|reverse order of shorts in array
addMethod sort|...|sort array shorts
addMethod usort|...|sort array unsigned shorts
addMethod setFromString|STRING_PTR ...|resize array to hold string at STRING_PTR and copy it there

addClass ShortArrayIter|Iter|short array iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|SHORT_VALUE ... BOOL|return true if found, iterator is positioned at matching short
addMember parent|ShortArray|ShortArray object this iter is associated with

addClass IntArray|Iterable|array of ints
addMethod get|INDEX ... INT_VALUE|get int value at specified index
addMethod set|INT_VALUE INDEX ...|set int value at specified index
addMethod ref|INDEX ... INT_PTR|return address of int at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap ints at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE ints
addMethod insert|INT_VALUE INDEX ...|insert INT_VALUE at INDEX
addMethod remove|INDEX ... INT_VALUE|remove INT_VALUE at specified index
addMethod push|INT_VALUE ...|add INT_VALUE to end of array
addMethod pop|... INT_VALUE|remove last INT_VALUE in array
addMethod base|... INT_PTR|return address of first int in array
addMethod load|<N INT_VALUEs> N ...|set array to contain N INT_VALUEs on stack
addMethod fromMemory|INT_PTR N OFFSET ...|copy N ints from memory at INT_PTR into array starting at index OFFSET
addMethod find|INT_VALUE ... ITER_OBJECT true   OR   false|return iterator positioned at INT_VALUE plus true, or just false if not found in array
addMethod findValue|INT_VALUE ... INDEX|return INDEX of int in array, return -1 if int not found
addMethod reverse|...|reverse order of ints in array
addMethod sort|...|sort array ints
addMethod usort|...|sort array unsigned ints
addMethod setFromString|STRING_PTR ...|resize array to hold string at STRING_PTR and copy it there

addClass IntArrayIter|Iter|int array iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|INT_VALUE ... BOOL|return true if found, iterator is positioned at matching int
addMember parent|IntArray|IntArray object this iter is associated with

addClass SFloatArray|Iterable|array of floats
addMethod get|INDEX ... FLOAT_VALUE|get sfloat value at specified index
addMethod set|FLOAT_VALUE INDEX ...|set sfloat value at specified index
addMethod ref|INDEX ... FLOAT_PTR|return address of sfloat at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap floats at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE floats
addMethod insert|FLOAT_VALUE INDEX ...|insert FLOAT_VALUE at INDEX
addMethod remove|INDEX ... FLOAT_VALUE|remove FLOAT_VALUE at specified index
addMethod push|FLOAT_VALUE ...|add FLOAT_VALUE to end of array
addMethod pop|... FLOAT_VALUE|remove last FLOAT_VALUE in array
addMethod base|... FLOAT_PTR|return address of first sfloat in array
addMethod load|<N FLOAT_VALUEs> N ...|set array to contain N FLOAT_VALUEs on stack
addMethod fromMemory|FLOAT_PTR N OFFSET ...|copy N floats from memory at FLOAT_PTR into array starting at index OFFSET
addMethod find|FLOAT_VALUE ... ITER_OBJECT true   OR   false|return iterator positioned at FLOAT_VALUE plus true, or just false if not found in array
addMethod findValue|FLOAT_VALUE ... INDEX|return INDEX of sfloat in array, return -1 if sfloat not found
addMethod reverse|...|reverse order of floats in array
addMethod sort|...|sort array of floats

addClass SFloatArrayIter|Iter|sfloat array iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|FLOAT_VALUE ... BOOL|return true if found, iterator is positioned at matching sfloat
addMember parent|FloatArray|FloatArray object this iter is associated with

addClass LongArray|Iterable|array of longs
addMethod get|INDEX ... LONG_VALUE|get long value at specified index
addMethod set|LONG_VALUE INDEX ...|set long value at specified index
addMethod ref|INDEX ... LONG_PTR|return address of long at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap longs at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE longs
addMethod insert|LONG_VALUE INDEX ...|insert LONG_VALUE at INDEX
addMethod remove|INDEX ... LONG_VALUE|remove LONG_VALUE at specified index
addMethod push|LONG_VALUE ...|add LONG_VALUE to end of array
addMethod pop|... LONG_VALUE|remove last LONG_VALUE in array
addMethod base|... LONG_PTR|return address of first long in array
addMethod load|<N LONG_VALUEs> N ...|set array to contain N LONG_VALUEs on stack
addMethod fromMemory|LONG_PTR N OFFSET ...|copy N longs from memory at LONG_PTR into array starting at index OFFSET
addMethod find|LONG_VALUE ... ITER_OBJECT true   OR   false|return iterator positioned at LONG_VALUE plus true, or just false if not found in array
addMethod findValue|LONG_VALUE ... INDEX|return INDEX of long in array, return -1 if long not found
addMethod reverse|...|reverse order of long in array
addMethod sort|...|sort array longs
addMethod usort|...|sort array unsigned longs
addMethod setFromString|STRING_PTR ...|resize array to hold string at STRING_PTR and copy it there

addClass LongArrayIter|Iter|long array iterator
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMethod findNext|LONG_VALUE ... BOOL|return true if found, iterator is positioned at matching long
addMember parent|LongArray|LongArray object this iter is associated with

addClass FloatArray|Iterable|array of doubles
addMethod get|INDEX ... DOUBLE_VALUE|get double value at specified index
addMethod set|DOUBLE_VALUE INDEX ...|set double value at specified index
addMethod ref|INDEX ... DOUBLE_PTR|return address of double at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap doubles at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE doubles
addMethod insert|DOUBLE_VALUE INDEX ...|insert DOUBLE_VALUE at INDEX
addMethod remove|INDEX ... DOUBLE_VALUE|remove DOUBLE_VALUE at specified index
addMethod push|DOUBLE_VALUE ...|add DOUBLE_VALUE to end of array
addMethod pop|... DOUBLE_VALUE|remove last DOUBLE_VALUE in array
addMethod base|... DOUBLE_PTR|return address of first double in array
addMethod load|<N DOUBLE_VALUEs> N ...|set array to contain N DOUBLE_VALUEs on stack
addMethod fromMemory|DOUBLE_PTR N OFFSET ...|copy N doubles from memory at DOUBLE_PTR into array starting at index OFFSET
addMethod find|DOUBLE_VALUE ... ITER_OBJECT true   OR   false|return iterator positioned at DOUBLE_VALUE plus true, or just false if not found in array
addMethod findValue|DOUBLE_VALUE ... INDEX|return INDEX of double in array, return -1 if double not found
addMethod reverse|...|reverse order of double in array
addMethod sort|...|sort array doubles
addMethod usort|...|sort array unsigned doubles
addMethod setFromString|STRING_PTR ...|resize array to hold string at STRING_PTR and copy it there

addClass FloatArrayIter|Iter|double array iterator - really a renamed LongArrayIter

addClass StructArray|Iterable|array of structs
addMethod get|DEST_PTR INDEX ...|copy struct at specified index to memory at DEST_PTR
addMethod set|SRC_PTR INDEX ...|copy struct at SRC_PTR to specified index
addMethod ref|INDEX ... STRUCT_PTR|return address of struct at INDEX
addMethod swap|INDEX_I INDEX_J ...|swap structs at specified indices
addMethod resize|NEW_SIZE ...|set array size to NEW_SIZE structs
addMethod insert|SRC_PTR INDEX ...|insert struct at SRC_PTR at INDEX
addMethod remove|DST_PTR INDEX ... STRUCT_VALUE|copy struct at INDEX to memory at DST_PTR and remove from array
addMethod push|SRC_PTR ...|add struct at SRC_PTR to end of array
addMethod pop|DST_PTR ... DUMMY_INT|copy last struct in array to DST_PTR and remove
addMethod base|... STRUCT_PTR|return address of first struct in array
addMethod setType|STRUCT_VOCAB_PTR ...|set type of structs in array - this sets element size

addClass StructArrayIter|Iter|struct array iterator
addMethod next|... STRUCT_PTR true   OR   false|return struct at iterator cursor and true, return false if already at last element, advance to next element
addMethod prev|... STRUCT_PTR true   OR   false|return struct at iterator cursor and true, return false if already at first element, advance to previous element
addMethod current|... STRUCT_PTR true   OR   false|return struct at iterator cursor and true, return false if already at last element
addMethod remove|...|remove the struct at iterator cursor
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMember parent|StructArray|StructArray object this iter is associated with

addClass Pair|Iterable|pair of objects - much like a 2 element Array
addMethod getA|... OBJECT_A|get object A
addMethod setA|OBJECT_A ...|set object A
addMethod getB|... OBJECT_B|get object B
addMethod setB|OBJECT_B ...|set object B

addClass PairIter|Iter|pair iterator
addMethod next|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at last element, advance to next element
addMethod prev|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at first element, advance to previous element
addMethod current|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at last element
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMember parent|Pair|Pair object this iter is associated with

addClass Triple|Iterable|triplet of objects - much like a 3 element Array
addMethod getA|... OBJECT_A|get object A
addMethod setA|OBJECT_A ...|set object A
addMethod getB|... OBJECT_B|get object B
addMethod setB|OBJECT_B ...|set object B
addMethod getC|... OBJECT_C|get object C
addMethod setC|OBJECT_C ...|set object C

addClass TripleIter|Iter|triplet iterator
addMethod next|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at last element, advance to next element
addMethod prev|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at first element, advance to previous element
addMethod current|... OBJECT true   OR   false|return item at iterator cursor and true, return false if already at last element
addMethod seek|INDEX ...|set iterator to position specified by INDEX
addMethod tell|... INDEX|return position of iterator
addMember parent|Triple|Triple object this iter is associated with

addClass Deque|Object|double ended queue of objects
addMethod count|... count|return number of objects in this queue
addMethod clear|...|empty queue
addMethod pushHead|OBJ ...|add object to head of queue
addMethod pushTail|OBJ ...|add object to tail of queue
addMethod popHead|... OBJ|remove object from head of queue
addMethod popTail|... OBJ|remove object from tail of queue
addMethod peekHead|... OBJ|copy object from head of queue
addMethod peekTail|... OBJ|copy object from tail of queue

addClass List|Iterable|linked list of objects
addMethod find|OBJECT ... ITER true   OR   false|return list iterator at position of found object
addMethod load|<N OBJECTS> N ...|set list to contain N objects on stack
addMethod toArray|... ARRAY_OBJECT|create an array with the same objects as this list
addMethod isEmpty|... BOOL|return true if list is empty
addMethod head|... OBJECT|get object at head of list
addMethod tail|... OBJECT|get object at tail of list
addMethod addHead|OBJECT ...|add object to head of list
addMethod addTail|OBJECT ...|add object to tail of list
addMethod removeHead|...|discard object at head of list
addMethod removeTail|...|discard object at tail of list
addMethod unrefHead|... OBJECT|remove object from head of list
addMethod unrefTail|... OBJECT|remove object from tail of list
addMethod remove|OBJECT ...|if object is found in list, it will be removed (only first occurence)

addClass ListIter|Iter|list iterator
addMethod findNext|SOUGHT_OBJECT ... BOOL|return true if found, iterator is positioned at SOUGHT_OBJECT
addMethod swapNext|...|swap the order of the object at iterator position and next in list
addMethod swapPrev|...|swap the order of the object at iterator position and previous in list
addMethod split|... NEW_LIST|split this list at the cursor position and return the new list
addMember parent|StructArray|StructArray object this iter is associated with

addClass Map|Iterable|map with object keys and object values
addMethod find|OBJECT_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|OBJECT_KEY ... OBJECT_VALUE true   OR   false|return object whose key matches OBJECT_KEY and true, return false if not found
addMethod set|OBJECT_VALUE OBJECT_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|OBJECT_VALUE ... OBJECT_KEY true   OR   false|return key object given value object
addMethod remove|OBJECT_KEY ...|remove object,key pair from map
addMethod unref|OBJECT_KEY ... OBJECT_VALUE|remov object,key pair from map, return value object

\ maps have unref but map iters don't have unref - why?
addClass MapIter|Iter|object map iterator
addMethod currentPair|... OBJECT_VALUE OBJECT_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|Map|Map object this iter is associated with

addClass IntMap|Iterable|map with integer keys and object values
addMethod find|INT_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|INT_KEY ... OBJECT_VALUE true   OR   false|return object whose key matches INT_KEY and true, return false if not found
addMethod set|OBJECT_VALUE INT_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|OBJECT_VALUE ... INT_KEY true   OR   false|return key int given value object
addMethod remove|INT_KEY ...|remove key/value pair from map
addMethod unref|INT_KEY ... OBJECT_VALUE|remove key/value pair from map, return value object

addClass IntMapIter|Iter|integer map iterator
addMethod currentPair|... OBJECT_VALUE INT_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|IntMap|IntMap object this iter is associated with

addClass SFloatMap|Iterable|map with sfloat keys and object values
addMethod find|FLOAT_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|FLOAT_KEY ... OBJECT_VALUE true   OR   false|return object whose key matches FLOAT_KEY and true, return false if not found
addMethod set|OBJECT_VALUE FLOAT_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|OBJECT_VALUE ... FLOAT_KEY true   OR   false|return key int given value object
addMethod remove|FLOAT_KEY ...|remove key/value pair from map
addMethod unref|FLOAT_KEY ... OBJECT_VALUE|remove key/value pair from map, return value object

addClass SFloatMapIter|Iter|sfloat map iterator
addMethod currentPair|... OBJECT_VALUE FLOAT_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|FloatMap|FloatMap object this iter is associated with

addClass LongMap|Iterable|map with sfloat keys and object values
addMethod find|LONG_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|LONG_KEY ... OBJECT_VALUE true   OR   false|return object whose key matches LONG_KEY and true, return false if not found
addMethod set|OBJECT_VALUE LONG_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|OBJECT_VALUE ... LONG_KEY true   OR   false|return key int given value object
addMethod remove|LONG_KEY ...|remove key/value pair from map
addMethod unref|LONG_KEY ... OBJECT_VALUE|remove key/value pair from map, return value object

addClass LongMapIter|Iter|long map iterator
addMethod currentPair|... OBJECT_VALUE LONG_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|Map|Map object this iter is associated with

addClass FloatMap|Iterable|map with sfloat keys and object values
addMethod find|DOUBLE_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|DOUBLE_KEY ... OBJECT_VALUE true   OR   false|return object whose key matches DOUBLE_KEY and true, return false if not found
addMethod set|OBJECT_VALUE DOUBLE_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|OBJECT_VALUE ... DOUBLE_KEY true   OR   false|return key int given value object
addMethod remove|DOUBLE_KEY ...|remove key/value pair from map
addMethod unref|DOUBLE_KEY ... OBJECT_VALUE|remove key/value pair from map, return value object

addClass FloatMapIter|Iter|double map iterator
addMethod currentPair|... OBJECT_VALUE DOUBLE_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|DoubleMap|DoubleMap object this iter is associated with

addClass StringIntMap|Iterable|map with string keys and integer values
addMethod find|STRING_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|STRING_KEY ... INT_VALUE true   OR   false|return integer whose key matches STRING_KEY and true, return false if not found
addMethod set|INT_VALUE STRING_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|INT_VALUE ... STRING_KEY true   OR   false|return key string given integer value
addMethod remove|STRING_KEY ...|remove key/value pair from map
addMethod unref|STRING_KEY ... INT_VALUE|remove key/value pair from map, return value object

addClass StringIntMapIter|Iter|string-int map iterator
addMethod currentPair|... INT_VALUE STRING_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|StringIntMap|StringIntMap object this iter is associated with

addClass StringFloatMap|Iterable|map with string keys and sfloat values - clone of StringIntMap with its own showInner method

addClass StringFloatMapIter|Iter|string-sfloat map iterator - clone of StringIntMapIter

addClass StringLongMap|Iterable|map with string keys and long values
addMethod find|STRING_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|STRING_KEY ... LONG_VALUE true   OR   false|return long whose key matches STRING_KEY and true, return false if not found
addMethod set|LONG_VALUE STRING_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|LONG_VALUE ... STRING_KEY true   OR   false|return key string given long value
addMethod remove|STRING_KEY ...|remove key/value pair from map
addMethod unref|STRING_KEY ... LONG_VALUE|remove key/value pair from map, return value object

addClass StringLongMapIter|Iter|string-long map iterator
addMethod currentPair|... LONG_VALUE STRING_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|StringLongMap|StringLongMap object this iter is associated with

addClass StringDoubleMap|Iterable|map with string keys and double values - clone of StringLongMap with its own showInner method

addClass StringDoubleMapIter|Iter|string-double map iterator - clone of StringLongMapIter

addClass String|Object|strings  
addMethod size|... INT_SIZE|return largest string that can be stored without resizing
addMethod length|... INT_LENGTH|return number of characters in string
addMethod get|... CHAR_PTR|return pointer to first byte of string
addMethod get4c|... 4CHAR_INT|return first 4 characters of string as an integer, zero padded
addMethod get8c|... 8CHAR_LONG|return first 8 characters of string as a long, zero padded
addMethod set|CHAR_PTR ...|set string from character pointer
addMethod set4c|4CHAR_VALUE ...|set string to up to 4 characters from integer 4CHAR_VALUE
addMethod set8c|8CHAR_VALUE ...|set string to up to 8 characters from long 8CHAR_VALUE
addMethod copy|STRING_OBJ ...|copy contents of STRING_OBJ into this string
addMethod append|CHAR_PTR ...|append string at CHAR_PTR onto this string
addMethod prepend|CHAR_PTR ...|prepend string at CHAR_PTR onto start of this string
addMethod getBytes|... CHAR_PTR INT_LENGTH|get pointer to string bytes and length
addMethod setBytes|CHAR_PTR INT_LENGTH ...|set this strings contents given pointer and length
addMethod appendBytes|CHAR_PTR INT_LENGTH ...|append INT_LENGTH bytes at CHAR_PTR onto this string
addMethod prependBytes|CHAR_PTR INT_LENGTH ...|prepend INT_LENGTH bytes at CHAR_PTR onto start of this string
addMethod resize|INT_SIZE ...|change storage allocated to this string - may truncate 
addMethod keepLeft|INT_NCHARS ...|keep first INT_NCHARS of string, remove rest
addMethod keepRight|INT_NCHARS ...|keep last INT_NCHARS of string, remove rest
addMethod keepMiddle|INT_FIRST INT_NCHARS ...|keep INT_NCHARS of string starting at charater INT_FIRST (0 based), remove rest
addMethod leftBytes|INT_NCHARS ... CHAR_PTR INT_LENGTH|keep first INT_NCHARS of string, remove rest, return pointer to string and length
addMethod rightBytes|INT_NCHARS ... CHAR_PTR INT_LENGTH|keep last INT_NCHARS of string, remove rest, return pointer to string and length
addMethod middleBytes|INT_FIRST INT_NCHARS ...CHAR_PTR INT_LENGTH|keep INT_NCHARS of string starting at charater INT_FIRST (0 based), remove rest, return pointer to string and length
addMethod equals|CHAR_PTR ... BOOL|return true if this string matches string at CHAR_PTR
addMethod startsWith|CHAR_PTR ... BOOL|return true if this string begins with the string at CHAR_PTR
addMethod endsWith|CHAR_PTR ... BOOL|return true if this string ends with the string at CHAR_PTR
addMethod contains|CHAR_PTR ... BOOL|return true if this string contains the string at CHAR_PTR
addMethod clear|...|set this string to empty
addMethod hash|... INT_HASH_VALUE|return a hash code computed from this strings contents
addMethod appendChar|CHAR_VALUE ...|appends single character onto end of this string
addMethod append4c|4CHAR_INT ...|appends up to 4 characters onto end of this string
addMethod append8c|8CHAR_LONG ...|appends up to 8 characters onto end of this string
addMethod load|N_CHAR_PTRS N ...|append N strings represented as pointers onto end of this string
addMethod split|ARRAY_OBJ DELIM_CHAR ...|splits this string using DELIM_CHAR, adds resulting strings to array ARRAY_OBJ
addMethod join|ARRAY_OBJ DELIM_CHAR ...|join the strings in ARRAY_OBJ, separated by DELIM_CHAR
addMethod format|FORMAT_STR_PTR ARGS NUM_ARGS ...|set this string to formatted string with NUM_ARGS arguments
addMethod appendFormatted|FORMAT_STR_PTR ARGS NUM_ARGS ...|appends formatted string with NUM_ARGS arguments to this string
addMethod fixup|...|sets this strings length param to actual string length
addMethod toLower|...|convert all characters in this string to lower case
addMethod toUpper|...|convert all characters in this string to upper case
addMethod replaceChar|OLD_CHAR NEW_CHAR ...|replace all occurences of OLD_CHAR in this string with NEW_CHAR

addClass StringMap|Iterable|map with string keys and object values
addMethod find|STRING_KEY ... ITER true   OR   false|return map iterator at position of found object
addMethod grab|STRING_KEY ... OBJECT true   OR   false|return object whose key matches STRING_KEY and true, return false if not found
addMethod set|OBJECT STRING_KEY ...|add a key/value pair to the map
addMethod load|<N PAIRS> N ...|set map to contain N value/key pairs on stack
addMethod findValue|OBJECT ... STRING_KEY true   OR   false|return key string given object
addMethod remove|STRING_KEY ...|remove key/value pair from map
addMethod unref|STRING_KEY ... OBJECT|remove key/value pair from map, return value object

addClass StringMapIter|Iter|string map iterator
addMethod currentPair|... OBJECT STRING_KEY true   OR   false|return item at iterator cursor and true, return false if already at last element
addMember parent|StringMap|StringMap object this iter is associated with

addClass InStream|Object|Abstract input stream base class
addMethod getChar|... CHAR_VALUE   OR   -1|returns next character in stream, -1 if stream is empty
addMethod getBytes|DEST_CHAR_PTR INT_NUM_BYTES ... INT_BYTES_COPIED|copy specified number of bytes from stream to DEST_CHAR_PTR, returns number of bytes actually copied
addMethod getLine|DEST_CHAR_PTR INT_MAX_BYTES ... INT_BYTES_COPIED|copy next line terminated by linefeed, up to specified maximum bytes from stream to DEST_CHAR_PTR, returns number of bytes actually copied
addMethod getString|STRING_OBJECT ... INT_BYTES_COPIED|copy next line terminated by linefeed from stream into specified string object, returns number of bytes actually copied
addMethod atEOF|... BOOL|return true if this stream has no more characters to return
addMethod iterChar|... CHAR_VALUE true   OR   false|returns next character in stream and true, or just false if stream is empty
addMethod iterBytes|DEST_CHAR_PTR INT_NUM_BYTES ... INT_BYTES_COPIED true   OR   false|copy specified number of bytes from stream to DEST_CHAR_PTR, returns number of bytes actually copied and true, or just false if stream is empty
addMethod iterLine|DEST_CHAR_PTR INT_MAX_BYTES ... INT_BYTES_COPIED true   OR   false|copy next line terminated by linefeed, up to specified maximum bytes from stream to DEST_CHAR_PTR, returns number of bytes actually copied and true, or just false if stream is empty
addMethod iterString|STRING_OBJECT ... INT_BYTES_COPIED true   OR   false|copy next line terminated by linefeed from stream into specified string object, returns number of bytes actually copied and true, or just false if stream is empty
addMethod setTrimEOL|BOOL ...|set flag that will cause linefeed to not appear in string returned by getLine, getString and iterLine methods
addMember userData|int|scratch for derived class use
addMember trimEOL|int|flag set by setTrimEOL method

addClass FileInStream|InStream|input stream which reads from files
addMethod open|FILENAME_PTR ACCESS_STRING_PTR ... BOOL|open named file for reading by this stream, return true if open succeed
addMethod close|...|close file for this stream
addMethod setFile|FILE_PTR ...|set the input file this stream uses
addMethod getFile|... FILE_PTR|get the input file this stream uses
addMethod getSize|...INT_FILESIZE|return size of file this stream uses
addMethod tell|... LONG_POSITION|return current position in file of this stream
addMethod seek|INT_SEEKTYPE LONG_POSITION ...|seek to LONG_POSITION, INT_SEEKTYPE is 0:start, 1:end, 2:relative
addMember inFile|int|file pointer for this stream

addClass ConsoleInStream|FileInStream|console input stream

addClass OutStream|Object|Abstract output stream base class
addMethod putChar|CHAR_VALUE ...|write character CHAR_VALUE to this stream
addMethod putBytes|SRC_CHAR_PTR INT_NUM_BYTES...|write INT_NUM_BYTES characters at SRC_CHAR_PTR to this stream
addMethod putString|SRC_CHAR_PTR ...|write string at SRC_CHAR_PTR to this stream
addMethod putLine|SRC_CHAR_PTR ...|write string at SRC_CHAR_PTR plus a linefeed to this stream
addMethod printf|FORMAT_STR_PTR ARGS NUM_ARGS ...|write formatted string with NUM_ARGS arguments to this stream
addMember userData|int|scratch for derived class use

addClass FileOutStream|OutStream|file output stream
addMethod open|FILENAME_PTR ACCESS_STRING_PTR ... BOOL|open named file for writing by this stream, return true if open succeed
addMethod close|...|close file for this stream
addMethod setFile|FILE_PTR ...|set the output file this stream uses
addMethod getFile|... FILE_PTR|get the output file this stream uses
addMethod getSize|...INT_FILESIZE|return size of file this stream uses
addMethod tell|... LONG_POSITION|return current position in file of this stream
addMethod seek|INT_SEEKTYPE LONG_POSITION ...|seek to LONG_POSITION, INT_SEEKTYPE is 0:start, 1:end, 2:relative
addMember outFile|int|file pointer for this stream

addClass StringOutStream|OutStream|string output stream
addMethod setString|STRING_OBJECT ...|set string object that stream output will be written to
addMethod getString|... STRING_OBJECT|get string object that stream output will be written to
addMember outString|...|string object that stream output will be written to

addClass ConsoleOutStream|FileOutStream|console output stream

addClass FunctionOutStream|OutStream|function output stream (don't use!)
addMethod init|CHAR_OUT BYTES_OUT STRING_OUT ...|set C functions for character, block and string output

addClass TraceOutStream|OutStream|trace output stream (for debugging)

addClass Block|Object|manager for old-style blocks
addMethod init|BLOCKFILE_NAME_PTR NUM_BLOCK_BUFFERS BYTES_PER_BLOCK ...|init block file manager - use 0 for NUM_BLOCK_BUFFERS and BYTES_PER_BLOCK to use system defaults (8 buffers with 1024 bytes/block)
addMethod blk|... BLOCKNUM_PTR|return pointer to block system's blk number (look at blk forth op)
addMethod block|BLOCK_NUMBER ... BLOCKDATA_PTR|get block specified by BLOCK_NUMBER into a buffer, read file if needed
addMethod buffer|BLOCK_NUMBER ... BLOCKDATA_PTR|allocate a buffer to block specified by BLOCK_NUMBER
addMethod emptyBuffers|...|mark all block buffers as unused
addMethod flush|...|write any updated block buffers to block file, then mark all block buffers as unused
addMethod saveBuffers|...|write any updated block buffers to block file
addMethod update|...|mark block most recently used in 'block' or 'buffer' methods as updated (needs to be written out)
addMethod thru|FIRST_BLOCK_NUMBER LAST_BLOCK_NUMBER ...|process all text in specified blocks
addMethod bytesPerBlock|... BYTES_PER_BLOCK|return the size of each block buffer
addMethod numBuffers|... NUM_BLOCK_BUFFERS|return the number of block buffers

addClass Int|Object|integer wrapper object
addMethod get|... INT_VALUE|get the integer value
addMethod set|INT_VALUE ...|set the integer value
addMethod getByte|... SIGNED_BYTE_VALUE|return lowest order byte of integer, signed
addMethod getUByte|... UNSIGNED_BYTE_VALUE|return lowest order byte of integer, unsigned
addMethod getShort|... SIGNED_SHORT_VALUE|return lower short of integer, signed
addMethod getUShort|... UNSIGNED_SHORT_VALUE|return lower short of integer, unsigned
addMember value|... INT_VALUE|holds the integer value

addClass Long|Object|long integer wrapper object
addMethod get|... LONG_VALUE|get the long integer value
addMethod set|LONG_VALUE ...|set the long integer value
addMember value|... LONG_VALUE|holds the long integer value

addClass SFloat|Object|single-precision floating point number wrapper object
addMethod get|... FLOAT_VALUE|get the sfloat value
addMethod set|FLOAT_VALUE ...|set the sfloat value
addMember value|... FLOAT_VALUE|holds the sfloat value

addClass Float|Object|double-precision floating point number wrapper object
addMethod get|... DOUBLE_VALUE|get the double value
addMethod set|DOUBLE_VALUE ...|set the double value
addMember value|... DOUBLE_VALUE|holds the double value

addClass Vocabulary|Object|vocabulary object
addMethod getName|... NAME_PTR|return name of vocabulary
addMethod headIter|... VOCABITER_OBJECT|create a new VocabularyIter object
addMethod findEntryByName|OP_NAME_PTR ... ENTRY_PTR|return pointer to vocabulary entry for named op, null if not found
addMethod getNewestEntry|... ENTRY_PTR|return pointer to vocabulary entry for newest defined op, null if vocabulary is empty
addMethod getNumEntries|... INT_NUM_ENTRIES|return the number of entries in vocabulary
addMethod getValueLength|... INT_NUM_INTS|return the size of entry value field in integers
addMethod addSymbol|NAME_PTR OP_TYPE OP_VALUE ADD_TO_ENGINE_BOOL ...|add a symbol to this vocabulary
addMember vocabulary|int (really a pointer)|

addClass VocabularyIter|Iterator|vocabulary iterator object
addMethod seekHead|...|seek to the newest entry in vocabulary
addMethod next|... ENTRY_PTR true   OR   false|return vocabulary entry pointer at iterator cursor and true, return false if already at last entry, advance to next element
addMethod findEntryByName|NAME_PTR ... ENTRY_PTR|seek to named entry, return entry pointer or null if not found
addMethod findNextEntryByName|NAME_PTR ... ENTRY_PTR|seek to next named entry, return entry pointer or null if not found
addMethod findEntryByValue|INT_VALUE ... ENTRY_PTR|seek to entry with specified value, return entry pointer or null if not found
addMethod findNextEntryByValue|INT_VALUE ... ENTRY_PTR|seek to next entry with specified value, return entry pointer or null if not found
addMethod removeEntry|ENTRY_PTR ...|remove entry specified by ENTRY_PTR from vocabulary
addMember parent|... VOCAB_OBJECT|associated Vocabulary object
addMember cursor|... ENTRY_PTR|current position of iterator

addClass Fiber|Object|synchronous thread object
addMethod start|...|mark this fiber as ready to run
addMethod startWithArgs|ARGS INT_NUM_ARGS ... BOOL|push specified arguments onto fiber param stack and start running it, returns if start worked (always true for now)
addMethod exit|...|exit this fiber
addMethod stop|...|stop this fiber from running
addMethod join|...|stop calling fiber and wait for this fiber to finish running
addMethod sleep|INT_MILLISECONDS ...|sleep this fiber for specified number of milliseconds
addMethod wake|...|wake this fiber from a sleep
addMethod push|INT_VALUE ...|push INT_VALUE onto this fibers parameter stack
addMethod pop|... INT_VALUE|pop INT_VALUE from this fibers parameter stack
addMethod rpush|INT_VALUE ...|push INT_VALUE onto this fibers return stack
addMethod rpop|... INT_VALUE|pop INT_VALUE from this fibers return stack
addMethod getRunState|... INT_RUNSTATE|get this fibers run state
addMethod step|... INT_RESULT|single step this fiber, see eInterpreterResult in forth_internals.txt for INT_RESULT meaning
addMethod reset|...|reset this fiber - clear fibers stacks, set its IP back to its starting IP
addMethod resetIP|...|set this fibers IP back to its starting IP
addMethod getCore|... CORE_PTR|return this fibers core pointer
addMember id|...|this fibers thread id

addClass Thread|Object|asynchronous thread - runs multiple Fibers (synchronous threads)
addMethod start|...|start this thread running asynchronously
addMethod startWithArgs|ARGS INT_NUM_ARGS ... BOOL|push specified arguments onto thread param stack and start running it, returns if start worked
addMethod exit|...|exit this thread
addMethod join|...|stop current fiber and wait for this thread to exit
addMethod reset|...|reset all the fibers which this thread is running
addMethod createFiber|MAINLOOP_OP INT_PSTACK_SIZE INT_RSTACK_SIZE ... SYNC_THREAD_OBJECT|create a Fiber with specified stacks and main loop op
addMethod getRunState|...INT_RUN_STATE|
addMember id|int|this threads thread id

addClass AsyncLock|Object|asynchronous lock object
addMethod grab|...|attempt to grab this lock - grabbing thread will block if lock is already grabbed
addMethod tryGrab|... BOOL|attempt to grab this lock - result is true if lock was grabbed
addMethod ungrab|...|release this lock
addMember id|...|this locks id

addClass Lock|Object|synchronous lock object
addMethod grab|...|attempt to grab this lock - grabbing fiber will block if lock is already grabbed
addMethod tryGrab|... BOOL|attempt to grab this lock - result is true if lock was grabbed
addMethod ungrab|...|release this lock - may cause this fiber to yield to another fiber waiting on this lock
addMember id|...|this locks id
addMember lockDepth|...|how many times this fiber has redundantly grabbed this lock

addClass AsyncSemaphore|Object|asynchronous semaphore object
addMethod init|INT_COUNT ...|initialize this semaphore with count INT_COUNT
addMethod wait|...|decrement the semaphore count, block this thread if semaphore count is zero
addMethod post|...|increment the semaphore count
addMember id|int|this semaphores id

addClass Semaphore|Object|synchronous semaphore object
addMethod init|INT_COUNT ...|initialize this semaphore with count INT_COUNT
addMethod wait|...|decrement the semaphore count, block this fiber if semaphore count is zero
addMethod post|...|increment the semaphore count
addMember id|int|this semaphores id
addMember count|int|this semaphores count value

addClass System|Object|system object that gives access to vocabularies and async objects
addMethod stats|...|prints system statistics
addMethod getDefinitionsVocab|... VOCAB_OBJECT|get the vocabulary that new ops are added to
addMethod setDefinitionsVocab|VOCAB_OBJECT ...|set the vocabulary that new ops are added to
addMethod clearSearchVocab|...|clear the forth search vocabulary stack
addMethod getSearchVocabDepth|... INT_DEPTH|return the number of vocaularies in the forth search stack
addMethod getSearchVocabAt|INT_DEPTH ... VOCAB_OBJECT|return the vocabulary at specified depth on forth search stack
addMethod getSearchVocabTop|... VOCAB_OBJECT|return the vocabulary on top of forth search stack
addMethod setSearchVocabTop|VOCAB_OBJECT ...|replace the top of search vocabulary with specified vocabulary
addMethod pushSearchVocab|VOCAB_OBJECT ...|push VOCAB_OBJECT on top of forth search vocabulary
addMethod getVocabByName|NAME_PTR ... VOCAB_OBJECT|return named vocabulary object, null if not found
addMethod getOpsTable|... OPTABLE_PTR|
addMethod getClassByIndex|INT_TYPE_INDEX ... CLASS_OBJECT|return class object for specified type index, null if index out of range or not a class
addMethod getNumClasses|... INT_TYPE_INDEX|return 1 more than highest class index

addMethod createThread|MAINLOOP_OP INT_PSTACK_SIZE INT_RSTACK_SIZE ... SYNC_THREAD_OBJECT|create an asynchronous thread with specified stacks and main loop op
addMethod createAsyncLock|... ASYNC_LOCK_OBJECT|create an asynchronous lock object
addMethod createAsyncSemaphore|... ASYNC_SEMAPHORE_OBJECT|create an asynchronous semaphore object
addMember namedObjects|StringMap|system named objects map
addMember args|Array of String|array of startup argument strings
addMember env|StringMap|system environment variables

\ Socket help descriptions need a lot of love

addClass Socket|Object|simple socket object
addMethod open|INT_DOMAIN INT_TYPE INT_PROTOCOL ... INT_SOCKET_FD|open a socket, return file descriptor, -1 if open fails
addMethod close|... INT_RESULT|close this socket, return result of close (0 means success)
addMethod bind|SOCKADDR_PTR INT_SOCKADDR_LEN ... INT_RESULT|bind an address to this socket, return 0 or greater for success
addMethod listen|INT_BACKLOG ... INT_RESULT|listen to this socket, return 0 or greater for success
addMethod accept|DST_SOCKADDR_PTR INT_SOCKADDR_LEN ... SOCKET_OBJECT|accept a connection on this socket, returns connection socket
addMethod connect|SOCKADDR_PTR INT_SOCKADDR_LEN ... INT_RESULT|connect this socket to specified remote address
addMethod send|BUFFER_PTR INT_NUM_BYTES INT_FLAGS ... INT_RESULT|write bytes to this socket
addMethod sendTo|BUFFER_PTR INT_NUM_BYTES INT_FLAGS SOCKADDR_PTR INT_SOCKADDR_LEN... INT_RESULT|write bytes to this socket and sends them to specified address
addMethod recv|BUFFER_PTR INT_NUM_BYTES INT_FLAGS ... INT_RESULT|read bytes from this socket, can be connectionless
addMethod recvFrom|BUFFER_PTR INT_NUM_BYTES INT_FLAGS SOCKADDR_PTR INT_SOCKADDR_LEN... INT_RESULT|read bytes from this socket using recvfrom, gets address that bytes came from
addMethod write|BUFFER_PTR INT_NUM_BYTES ... INT_RESULT|write bytes to this socket
addMethod read|BUFFER_PTR INT_NUM_BYTES ... INT_RESULT|read bytes from this socket
\ addClassOp inetPToN|INT_FAMILY SRC_TEXT_PTR DST_NETADDRESS_PTR ... INT_RESULT|convert network address string ac SRC_TEXT_PTR to network address in DST_NETADDRESS_PTR
\ addClassOp inetNToP|INT_FAMILY SRC_NETADDRESS_PTR DST_TEXT_PTR INT_DST_LEN ... INT_RESULT|convert network address at SRC_NETADDRESS_PTR to printable form in buffer at DST_TEXT_PTR, at most INT_DST_LEN characters
\ addClassOp htonl|INT_VAL ... INT_RESULT|convert integer from host byte order to network byte order
\ addClassOp htons|SHORT_VAL ... SHORT_RESULT|convert short from host byte order to network byte order
\ addClassOp ntohl|INT_VAL ... INT_RESULT|convert integer from network byte order to host byte order
\ addClassOp ntohs|SHORT_VAL ... SHORT_RESULT|convert short from network byte order to host byte order
addMember fd|uint|socket file descriptor
addMember domain|uint|socket domain
addMember type|uint|socket type
addMember protocol|uint|socket protocol

\ NoteDeck, Note and supporting classes
addClass NoteDef|Object|just a base class for flags, tags and schemas
addMember id|int|index into tags/flags/schemas/params arrays
addMember name|String|name of this definition (for searching)
addMember description|String|text for this definition
addMethod init|NAME DESCRIPTION ID ...|initialize this definition

addClass NoteTagDef|NoteDef|defines a binary attribute for Notes
addMember displayName|String|title of this tag definition for display
addMember mask|long|bitmask for this attribute
addMethod init|NAME DISPLAY_NAME DESCRIPTION ID ...|initialize a tag definition

addClass NoteSchema|NoteDef|define a schema for notes
addMember requiredParams|IntArray|array of required param ids for a note with this schema
addMember optionalParams|IntArray|array of optional param ids for a note with this schema
addMethod init|NAME DISPLAY_NAME DESCRIPTION ID ...|initialize a schema definition
addMethod addOptionalParam|ID ...|add an optional param to this schema
addMethod addRequiredParam|ID ...|add a required param to this schema

addClass NoteParam|Object|parameter values for a note
addMethod getInt|... INT_VALUE|get integer param value
addMethod setInt|INT_VALUE ...|set integer param value
addMethod getFloat|... INT_VALUE|get sfloat param value
addMethod setFloat|INT_VALUE ...|set sfloat param value
addMethod getLong|... INT_VALUE|get long param value
addMethod setLong|INT_VALUE ...|set long param value
addMethod getDouble|... INT_VALUE|get double param value
addMethod setDouble|INT_VALUE ...|set double param value
addMethod getString|... STRING_OBJ|get string param value
addMethod setString|STRING_OBJ ...|set string param value

addClass Note|Object|virtual notecard
addMember id|int|index into containing NoteDeck notes array
addMember schema|int|index into containing NoteDeck schemas array, 0 for default schema
addMember name|String|name of this note
addMember body|String|text of this note
addMember children|StringMap of Note|map from note names to children notes of this note
addMember params|IntMap of NoteParam|
addMember tags|long|binary attributes of this note
addMember links|IntArray|array of links for this note
addMethod init|NAME BODY ID ...|initialize this note
addMethod getParam|PARAM_ID ...   false   OR   PARAM_OBJ true|get a parameter for this note

addClass NoteDeck|Object|Collection of Notes
addMember name|String|name of the note deck
addMember notes|Array of Note|notes in this deck
addMember noteMap|StringMap|map from a global notes name to the Note
addMember tags|Array of NoteTagDef|tag definitions for this deck
addMember tagMap|StringMap|map from name to tag definition
addMember schemas|Array of NoteSchema|schemas for this deck
addMember schemaMap|StringMap|map from name to schema
addMember params|Array of NoteDef|parameter definitions for this deck
addMember paramMap|StringMap|map from name to parameter definition
addMethod clearAll||
addMethod addTagDef|NAME DISPLAY_NAME DESCRIPTION ... NEW_TAG_DEF|
addMethod getTag|NAME ...    false   OR   TAG_DEF_OBJ true|
addMethod addSchema|NAME DESCRIPTION ... NEW_SCHEMA|
addMethod getSchema|NAME ...    false   OR   SCHEMA_OBJ true|
addMethod addParamDef|NAME DESCRIPTION ... NEW_PARAM_DEF|
addMethod getParam|NAME ...    false   OR   PARAM_DEF_OBJ true|
addMethod addNote|NAME BODY ... NEW_NOTE|
addMethod getChild|NAME NOTE ...   false   OR   NOTE_OBJ true|get a named child of this note
addMethod addChild|NAME BODY PARENT_NOTE ... NEW_NOTE|
addMethod getNote|NAME ...   false   OR   NOTE_OBJ true|
addMethod init|NOTEBOOK_NAME ...|
addMethod newestNote|... NOTE_OBJ|

addClass HelpDeck|NoteDeck|help system deck
addMember opSchema|NoteSchema|
addMember stackParamDef|NoteDef|
addMember memberTypeParamDef|NoteDef|
addMember classSchema|NoteSchema|
addMember methodSchema|NoteSchema|
addMember memberSchema|NoteSchema|
addMember opsNote|Note|note for "help ops"
addMember classesNote|Note|note for "help classes"
addMember newestClassNote|Note|
addMember curTags|long|
addMember kTagOp|long|
addMember kTagMethod|long|
addMember kTagClass|long|
addMember kTagMember|long|
addMethod init||initialize help deck
addMethod setTags||set current flags to immediately following names
addMethod getOpNote|NAME ...   false   OR   NOTE_OBJ true|get op selected by name
addMethod getClassNote|NAME ...   false   OR   NOTE_OBJ true|get class selected by name

\ use slash for separator so description can have '|' in it
helpDeck.separator
'/' -> helpDeck.separator
addMethod addOp \ add help for op, immediately followed by name|stack behavior|description|extra flags
addMethod addClass \ add help for class, immediately followed by className|baseClass|description|flags
addMethod addMethod \ add help for class method, immediately followed by methodName|stack behavior|description|extra flags
addMethod addMember \ add help for class member, immediately followed by memberName|type|description|extra flags
-> helpDeck.separator

addMethod listOperators||list all operators
addMethod listClasses||list all classes
addMethod help||show help for immediately following symbol
addMethod check||show forth symbols which there is no help defined

loaddone

TODO:
- add addClassOp and display class ops when showing a class
- add base class methods and members when class is defined
- add removeMethods which takes list of inherited methods to remove
  ? just remove them from the class card, or keep them marked as removed and display as such?
