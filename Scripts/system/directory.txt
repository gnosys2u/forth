
autoforget Directory

class: Directory extends Object

  cell dirHandle
  dirent entry
  cell position
  String path

  m: close
    if(dirHandle)
      closedir(dirHandle)
      drop
    endif
    
    oclear path
  ;m
  
  m: delete
    close
  ;m
  
  // PATH_STRING ... OPEN_SUCCEEDED
  m: open
    new String -> path
    path.set(dup)
    if(dirHandle)
      closedir(dirHandle)
      drop
    endif
    opendir dup -> dirHandle
    0<>
  ;m
  
  m: openHere
    open(".")
  ;m
  
  // ... STRING_PTR IS_DIRECTORY true      if there is an entry
  // ... false                             if no entries left
  m: nextEntry
    if(dirHandle)
      if(readdir(dirHandle entry))
        entry.d_name(0 ref)
        and(entry.d_type DIRENT_IS_DIR) 0<>
        true
      else
        false
      endif
    else
      false
    endif
  ;m

  m: rewind
    if(dirHandle)
      rewinddir(dirHandle)
    endif
  ;m
  
  // ... ARRAY_OF_FILENAME_STRINGS
  m: getFilenames returns Array
    ptrTo byte pName
    mko Array of String filenames
  
    if(dirHandle)
      rewind
      
      begin
      while(nextEntry)
        // TOS: isDirectory pathString
        drop
        -> pName
        //format(pEntry.d_type "%08x ") %s pName %s %nl ds
        if(pName c@ `.` <>)
          new String -> String filename
          filename.append(pName)
          filenames.push(filename)
          oclear filename
        endif
      repeat
    endif
    unref filenames
  ;m
  
  m: listFilenames
    getFilenames -> Array of String filenames
    filenames.headIter -> ArrayIter iter
    begin
    while(iter.next)
      <String>.get %s %nl
    repeat
    oclear filenames
  ;m
  
;class


loaddone

desired directory operations:
- open/init - takes relative path
- descend - takes subdirectory name
- ascend - 
- getAbsolutePath
- createDirectory(leafName)
OIterable interface:

        METHOD(     "seekNext",             unimplementedMethodOp ),
        METHOD(     "seekPrev",             unimplementedMethodOp ),
        METHOD(     "seekHead",             unimplementedMethodOp ),
        METHOD(     "seekTail",             unimplementedMethodOp ),
        METHOD_RET( "next",					unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "prev",                 unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "current",				unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD(     "remove",				unimplementedMethodOp ),
        METHOD_RET( "unref",				unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeObject) ),
        METHOD_RET( "findNext",				unimplementedMethodOp, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt) ),
        METHOD_RET( "clone",                unimplementedMethodOp, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIIter) ),
        
// "DIRECTORY_PATH" ... Array of String
: getFilesInDirectory
  int dirHandle
  ptrTo dirent pEntry
  int pName
  new Array -> Array of String filenames
  
  opendir -> dirHandle
  if( dirHandle )
    begin
      readdir(dirHandle) -> pEntry
    while( pEntry )
      pEntry.d_name(0 ref) -> pName
      if(pName c@ `.` <>)
        new String -> String filename
        filename.set(pName)
        filenames.push(filename)
        oclear filename
      endif
    repeat
    closedir(dirHandle) drop
  endif
  unref filenames
;

