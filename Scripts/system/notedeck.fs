
autoforget notedeck

: notedeck ;

\ tags are binary properties any note can have
\ params are properties of a note, the params a note can have are defined by its schema

\ ----------------------------------------------
class: NoteDef  \ just a base class for tags and schemas
  int id
  String name
  String description
  
  m: delete
    oclear name
    oclear description
    super.delete
  ;m
  
  m: init  \ NAME DESCRIPTION ID ...
    -> id
    new String -> description
    description.set
    
    new String -> name
    name.set
  ;m
  
;class


\ ----------------------------------------------
class: NoteTagDef extends NoteDef
  String displayName
  long mask
  
  m: delete
    oclear displayName
    super.delete
  ;m
  
  m: init  \ NAME DISPLAY_NAME DESCRIPTION ID ...
    new String -> displayName
    displayName.set(rot)
    super.init
    
    if(id 64 <)
      2lshift(1L id) -> mask
    endif
  ;m
  
;class


\ ----------------------------------------------
class: NoteSchema extends NoteDef
  IntArray requiredParams
  IntArray optionalParams
  
  m: init   \ NAME DISPLAY_NAME DESCRIPTION ID ...
    new IntArray -> requiredParams
    new IntArray -> optionalParams
    super.init
  ;m
  
  m: addOptionalParam   \ ID ...
    optionalParams.push(<NoteDef>.id)
  ;m
  
  m: addRequiredParam   \ ID ...
    requiredParams.push(<NoteDef>.id)
  ;m
  
  m: delete
    oclear requiredParams
    oclear optionalParams
    super.delete
  ;m
  
;class


\ ----------------------------------------------
class: NoteParam
  int id
  String sval
  long nval
  
  m: delete
    oclear sval
    super.delete
  ;m
  
  m: getInt     ref nval @      ;m
  m: setInt     ref nval !      ;m
  m: getFloat   ref nval @      ;m
  m: setFloat   ref nval !      ;m
  m: getLong    nval            ;m
  m: setLong    -> nval         ;m
  m: getDouble  nval            ;m
  m: setDouble  swap -> nval    ;m
  m: getString  sval            ;m
  m: setString  sval.set        ;m
  
;class


\ ----------------------------------------------
class: Note
  int id            \ index into containing Notebook notes array
  int schema        \ 0 for default schema
  String name
  String body
  IntMap of NoteParam params
  long tags
  IntArray links

  m: init           \ NAME BODY ID ...
    -> id
    new String -> body
    body.set
    new String -> name
    name.set
    0 -> schema
    new IntMap -> params
    new IntArray -> links
  ;m
  
  m: delete
    oclear name
    oclear body
    oclear params
    oclear links
    super.delete
  ;m

  m: getParam    \ PARAM_ID ...   false   OR   PARAM_OBJ true 
    params.grab
  ;m
  
  m: addTag             \ TAG_ID ...
    -> int tagId
    if(tagId 64 <)
      lshift(1 tagId) tags l+ -> tags
    else
      \ TODO: deal with when extended tags are added
    endif
  ;m
  
  m: hasTag             \ TAG_ID ... true/false
    -> int tagId
    if(tagId 64 <)
      0
      if(lshift(1 tagId) tags and)
        1-
      endif
    else
      \ TODO: deal with when extended tags are added
    endif
  ;m
  
  m: isNamed        \ NAME ... true/false
    if(name)
      name.equals
    else
      0
    endif
  ;m
  
  : link2type
    26 rshift
  ;
  
  : linkSplit           \ LINK ... NODE_ID LINK_TYPE
    dup $3FFFFFF and
    swap 26 rshift
  ;
  
  m: addLink            \ NODE_ID LINK_TYPE ...
    links.push(26 lshift or)
  ;m
  
  m: findNextLink       \ START_INDEX LINK_TYPE ...    false   OR    NODE_ID NEXT_INDEX true
    -> int linkType
    -> int index
    \ "looking for type " %s linkType %d " starting at " %s index %d %nl
    begin
    while(index links.count <)
      \ index %d %bl %bl links.get(index) dup %x %bl %bl linkSplit "type:" %s %x " value:" %s %x %nl
      if(links.get(index) linkSplit linkType =)
        \ "bailing at index " %s index %d " returning nodeId " %s dup %x %nl
        index 1+ -1
        exit
      else
        drop
      endif
      1 ->+ index
    repeat
    0
  ;m

;class

class: NoteLinkIter

  Note note
  int index
  int limit
  
  m: init   \ NOTE ...
    -> note
    note.links.count -> limit
    0 -> index
  ;m

  m: next  \ LINK_TYPE ... NOTE_ID true  OR   false
    index swap note.findNextLink
    if
      -> index
      -1
    else
      limit -> index
      0
    endif
  ;m
  
  m: delete
    oclear note
  ;m

  m: seek  \ INDEX ...
    if(dup 0>)
      if(dup limit <)
        -> index
      else
        drop
      endif
    else
      drop
    endif
  ;m

  m: tell  \ ... INDEX
    index
  ;m

  m: current \ ... NODE_ID
    note.links.get(index) Note:linkSplit drop
  ;m

  m: atTail \ ... TRUE/FALSE
    index limit =
  ;m

;class



\ ----------------------------------------------
class: NoteDeck

  String name
  
  Array of Note notes
  StringMap noteMap
  
  Array of NoteTagDef tags
  StringMap tagMap
  
  Array of NoteSchema schemas
  StringMap schemaMap
  
  Array of NoteDef params
  StringMap paramMap

  Array of NoteDef linkTypes
  StringMap linkTypeMap

  \ predefined link types
  int kLTParent
  int kLTChild
  int kLTTag
  int kLTLink
  
  m: clearAll
    oclear name
    oclear notes
    oclear noteMap
    oclear tags
    oclear tagMap
    oclear schemas
    oclear schemaMap
    oclear params
    oclear paramMap
  ;m
  
  m: delete
    clearAll
  ;m
  
  m: addTagDef returns NoteTagDef      \ NAME DISPLAY_NAME DESCRIPTION ... NEW_TAG_DEF
    mko NoteTagDef def
    def.init(tags.count)
    tags.push(def)
    tagMap.set(def def.name.get)
    unref def
  ;m

  m: getTag         \ NAME ...    false   OR   TAG_DEF_OBJ true
    tagMap.grab
  ;m
  
  m: addSchema returns NoteSchema     \ NAME DESCRIPTION ... NEW_SCHEMA
    mko NoteSchema def
    def.init(schemas.count)
    schemas.push(def)
    schemaMap.set(def def.name.get)
    unref def
  ;m
  
  m: getSchema         \ NAME ...    false   OR   SCHEMA_OBJ true
    schemaMap.grab
  ;m
  
  m: addParamDef returns NoteDef     \ NAME DESCRIPTION ... NEW_PARAM_DEF
    mko NoteDef def
    def.init(params.count)
    params.push(def)
    paramMap.set(def def.name.get)
    unref def
  ;m
  
  m: getParam        \ NAME ...    false   OR   PARAM_DEF_OBJ true
    paramMap.grab
  ;m
  
  m: addLinkType returns NoteDef     \ NAME DESCRIPTION ... NEW_LINKTYPE
    mko NoteDef def
    def.init(linkTypes.count)
    linkTypes.push(def)
    linkTypeMap.set(def def.name.get)
    def.id
    oclear def
  ;m
  
  m: getLinkType        \ NAME ...    false   OR   LINKTYPE_OBJ true
    linkTypeMap.grab
  ;m

  \ creates a parentless note, doesn't add it to global named note map  
  m: addNote returns Note      \ NAME BODY ... NEW_NOTE
    mko Note note
    note.init(notes.count)
    notes.push(note)
    unref note
  ;m
  
  \ creates a parentless note and adds it to global named note map  
  m: addGlobalNote returns Note      \ NAME BODY ... NEW_NOTE
    addNote ->o Note note
    \ TODO: check for note name already in map
    noteMap.set(note note.name.get)
    note
  ;m
  
  m: getNamedLink   \ NAME LINK_TYPE NOTE ...    false   OR    INDEX true
    ->o Note note
    -> int linkType
    -> ptrTo byte name
    note.links.headIter -> IntArrayIter iter
    begin
    while(iter.next)
      Note:linkSplit
      if(linkType =)
        -> int childId
        if(notes.get(childId).isNamed(name))
          childId -1
          oclear iter
          exit
        endif
      else
        drop
      endif
    repeat
    oclear iter

    0
  ;m
  
  m: getChild            \ NAME NOTE ...   false   OR   NOTE_OBJ true
    kLTChild swap getNamedLink
    if
      notes.get -1
    else
      0
    endif
  ;m
  
  m: addChild returns Note     \ NAME BODY PARENT_NOTE ... NEW_NOTE
    ->o Note parent
    addNote -> Note note
    note.addLink(parent.id kLTParent)
    parent.addLink(note.id kLTChild)
    unref note
  ;m
  
  m: getNote            \ NAME ...   false   OR   NOTE_OBJ true
    noteMap.grab
  ;m
  
  m: init   \ NOTEBOOK_NAME ...
    clearAll
    
    new String -> name
    name.set
    new Array -> notes
    new StringMap -> noteMap

    new Array -> tags
    new StringMap -> tagMap

    new Array -> schemas
    new StringMap -> schemaMap

    new Array -> params
    new StringMap -> paramMap

    new Array -> linkTypes
    new StringMap -> linkTypeMap
    
    addLinkType("parent" "Parent Note") -> kLTParent
    addLinkType("child" "Child Note") -> kLTChild
    addLinkType("tag" "Extended Tag") -> kLTTag
    addLinkType("link" "Generic Link") -> kLTLink

    addSchema("defaultSchema" "Default Schema - notecard with no params") drop
  ;m
  
  m: newestNote returns Note
    if(notes.count)
      notes.get(notes.count 1-)
    else
      null
    endif
  ;m

  m: getLinkIter returns NoteLinkIter       \ NOTE ... NOTE_LINK_ITER
    mko NoteLinkIter iter
    iter.init
    unref iter
  ;m
  
;class

loaddone

=============================================
TODOs for Notes:

  int id            \ index into containing Notebook notes array
  int schema        \ 0 for default schema
x  int parent        \ -1 for none
  String name
  String body
x  StringMap of Note children
  IntMap of NoteParam params
  long flags
x Object obj        \ optional associated object

- replace children StringMap with links IntArray, where the elements are a combination of
  26-bit note id and 6-bit link type
  -> make the noteId/linkType specified per notebook
  o link types are specified by the notebook, but some are predefined
    like 'parent' and 'child'
  o tags just become notes which describe the tag
  o replace params with a StructArray, the param struct is an 7-char name,
    1 byte type code and an 8-byte body
  o not all Notes have names, change NoteDeck:addNote to addNamedNote,
    add an addNote member that doesn't set a name
  ? remove parent member, replace with parent link
  o remove obj member, use params instead
  
Hmm, helpdeck uses multiple levels of name maps, which can have the same
symbol used multiple times, in particular for method names.  I guess the
change to one global name map will still work, but then there needs to be
a way for a Note to have a name without appearing in the global name map,
method names don't need to be in the global map, but the code uses the Note
name field for the method name.
? is there a general 'index' needed to replace Note:children StringMap?
? maybe NoteDeck has StringMap of StringIntMap indices, which is map of named indices?
? replace Note:params with a Bag?

Maybe a Note can have a "children" param, which if it exists will always
be (by convention) the first element in params and be a StringIntMap.

Should there be a way of defining ops which are specific to a notebook?
Note and NoteDeck seem like a flexible way of defining data without having
to define new structs and classes, it seems like there should be a way of
defining actions related to notes without having to define new ops or class
methods to do them.

What I am heading towards is two scenarios:

1) the help command
2) collosal cave adventure can be implemented almost entirely inside NoteDeck
=============================================
collosal cave adventure

adv.txt
  is_forced has_light has_water has_oil - these should be tags
  IGame:init sets R_LIMBO -> last_knife_loc
game.txt
  Game:build_object_table puts objects in locations
  Game:initDwarves
  Game:is_treasure 
=============================================
There is always a default 'incoming' notebook
other notebooks are projects/interestAreas for user
forth
music
bass
media
=============================================
addtags dailyWorkLog forth python c++ math reddit metafilter torrents arm

add a tagged note object/file/db
nodes have
  o ID (32-bit)
  o schema/type
  o list of linked nodes
  o list of tags
  o title
  o text body
  o creation date
  o target date
  o optional message handlers (forth code as strings)
? maybe start with a general graph of nodes
? maybe a 'named object' blob - how would that be different from a string map?
? maybe an InfoNode is just an id, String, Array
? should linked nodes be an array of objects, or of nodeIDs
  o nodeIDs would take half the space
  o nodeIDs are more easily serializable, and more easy to convert to a database representation
  o objects have the advantage of refcount being meaningful (but is that really a plus for this application?)
  
What exactly is a tag?

6   seconds
6   minutes
5   hours
15  days

32000 days -> 100 years

60
*60 -> 3600
*24 -> 86400
*365 -> 31536000
136.19 years

---
NoteCard:
  body - string
  tags - array of ids
  links - array of tag,cardId pairs
  params - array of tag,string pairs
  ? should tags/links/params just be combined
    not combining them is probably better since they have different data and meaning

NoteDeck
  title - string
  cards - array of NoteCard
  tags - array of NoteTag
  tagMap - map from string to NoteTag id

NoteTag
  name - string
  id - int

First cut params:
  title
  creationTime
  completedTime
  status
  priority
  parentCard
  blockedBy/requires
  
First cut tags:
  forth (or is this ubiquitous to a forthTBD NoteDeck?)
  tbd
  tests
  windows osx arm linux
  objects strings streams threads
  language
  documentation
  devaid
