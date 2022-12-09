
class: Blob

  int numBytes
  ptrTo byte blobData
  bool needsFreeing

  : safeFree
    if(and(needsFreeing  blobData 0<>))
      free(blobData)
    endif
  ;

  m: free
    safeFree
    0 -> numBytes
    null -> blobData
    false -> needsFreeing
  ;m
  
  m: allocate
    safeFree
    dup -> numBytes
    malloc -> blobData
    true -> needsFreeing
    // TODO: report malloc failure
  ;m
  
  m: reallocate
    -> numBytes
    if(needsFreeing)
      realloc(blobData numBytes)
    else
      // TODO: error
    endif
  ;m
    
  m: delete
    safeFree
  ;m
  
;class

loaddone
