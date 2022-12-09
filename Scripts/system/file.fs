
autoforget File

class: File extends Object

  int fp
  String path
  ByteArray buffer		// line buffer for getLine
  byte readOnly
  byte unixEol			// end-of-line for putLine
  byte noTrimEol		// defined bass-ackwards so it will default to 'trim eol' IE remove trailing CR and/or LF in getLine
  byte stdHandle		// true if file is stdin, stdout or stderr, used to prevent mistakenly closing stdout
  
  : error
    "error in file." %s %s %nl
  ;

  m: delete
    if( fp )
      fclose( fp )
    endif
    oclear buffer
    oclear path
    super.delete
  ;m
    
  // ...
  m: close
    if( fp )
      if( not( stdHandle ) )
        fclose( fp )
        if
          error( "close" )
        endif
      endif
      null -> fp
    else
      error( "close - not open" )
    endif
  ;m
  
  // "inpath" "mode" ... t/f  
  m: open
    -> ptrTo byte mode
    -> ptrTo byte inpath
    
    if( fp )
      close
    endif
    mode c@ `r` = -> readOnly   // yeah, this is probably wrong

    false -> stdHandle
    if( inpath c@ `<` = )
      if( strcmp( "<stdin>" inpath ) 0= )
        if( not( readOnly ) )
          error( "open - can't open stdin for write" )
          exit
        endif
        stdin -> fp
        true -> stdHandle
      elseif( strcmp( "<stdout>" inpath ) 0= )
        if( readOnly )
          error( "open - can't open stdout for read" )
          exit
        endif
        stdout -> fp
        true -> stdHandle
      elseif( strcmp( "<stderr>" inpath ) 0= )
        if( readOnly )
          error( "open - can't open stderr for read" )
          exit
        endif
        stderr -> fp
        true -> stdHandle
      endif
    endif
    
    if( fp 0= )
      fopen( inpath mode ) -> fp
    endif
    
    if( fp )
      if(path objIsNull)
        new String -> path
      endif
      path.set(inpath)
      true
    else
      error( "open" )
      false
    endif
  ;m

  // "path" ... t/f  
  m: openRead    "r" open  ;m
  
  // "path" ... t/f  
  m: openWrite    "w" open  ;m
  
  // "path" ... t/f  
  m: openAppend    "a" open  ;m
  
  // "path" ... t/f  
  m: openReadBinary    "rb" open  ;m
  
  // "path" ... t/f  
  m: openWriteBinary    "wb" open  ;m
  
  // "path" ... t/f  
  m: openAppendBinary    "ab" open  ;m
  
  // ... BYTE_VALUE
  m: getByte
    if( fp )
      fgetc( fp )
      if( dup -1 = )
        error( "getByte" )
      endif
    else
      error( "getByte - not open" )
      0
    endif
  ;m

  // ... SHORT_VALUE
  m: getShort
    0 -> short val
    if( fp )
      fread( ref val 2 1 fp )
      if( 1 <> )
        error( "getShort" )
      endif
    else
      error( "getShort - not open" )
    endif
    val
  ;m
  
  // ... INT_VALUE
  m: getInt
    0 -> int val
    if( fp )
      fread( ref val 4 1 fp )
      if( 1 <> )
        error( "getInt" )
      endif
    else
      error( "getInt - not open" )
    endif
    val
  ;m

  // ... LONG_VALUE
  m: getLong
    dnull -> long val
    if( fp )
      fread( ref val 8 1 fp )
      if( 1 <> )
        error( "getLong" )
      endif
    else
      error( "getLong - not open" )
    endif
    val
  ;m

  // BYTE_VALUE ...
  m: putByte
    if( fp )
      if( readOnly )
        error( "putByte - readOnly" )
      else
        fputc( fp )
        if( -1 = )
          error( "putByte" )
        endif
      endif
    else
      error( "putByte - not open" )
      drop
    endif
  ;m

  // SHORT_VALUE ...
  m: putShort
    -> short val
    if( fp )
      if( readOnly )
        error( "putShort - readOnly" )
      else
        fwrite( ref val 2 1 fp )
        if( 1 <> )
          error( "putShort" )
        endif
      endif
    else
      error( "putShort - not open" )
    endif
  ;m

  // INT_VALUE ...
  m: putInt
    -> int val
    if( fp )
      if( readOnly )
        error( "putInt - readOnly" )
      else
        fwrite( ref val 4 1 fp )
        if( 1 <> )
          error( "putInt" )
        endif
      endif
    else
      error( "putInt - not open" )
    endif
  ;m

  // LONG_VALUE ...
  m: putLong
    -> long val
    if( fp )
      if( readOnly )
        error( "putLong - readOnly" )
      else
        fwrite( ref val 8 1 fp )
        if( 1 <> )
          error( "putLong" )
        endif
      endif
    else
      error( "putLong - not open" )
    endif
  ;m

  // CHAR_PTR ...  
  m: putLine
    if( fp )
      if( readOnly )
        error( "putLine - readOnly" )
      else
        fputs( fp )
        if( -1 = )
          error( "putLine" )
        else
          if( unixEol )
            putByte( 0x0A )
          else
            putShort( 0x0A0D )
          endif
        endif
      endif
    else
      error( "putLine - not open" )
      drop
    endif
  ;m

  // ... CHAR_PTR
  m: getLine
    null -> ptrTo byte result
    if( fp )
      ftell( fp ) -> int pos
      
      if( buffer objIsNull )
        d[ "file.getLine allocating buffer\n" %s ]d
        new ByteArray -> buffer
        buffer.resize( 256 )
        //buffer.resize( 32000 )
      endif
      //buffer.resize( 1 )			// just for testing
      
      // loop until an end-of-line is read (or eof)
      false -> int done
      buffer.base -> ptrTo byte pDst
      buffer.count -> int charsToGet
      begin
        if( fgets( pDst charsToGet fp )  )
          buffer.base -> pDst
          pDst -> result
          d[ t[ "file.getLine got {" %s pDst %s "}\n" %s ]t ]d
          do( buffer.count 0 )
            pDst c@ -> byte c
            if( c 0= )
              // buffer didn't have newline, resize buffer and try again
              buffer.resize( buffer.count 5 * 2 rshift )	// increase to 5/4 current size
              buffer.base i + -> pDst
              buffer.count i - -> charsToGet
              leave
            else
              if( c 0x0D = )
                if( not( noTrimEol ) )
                  0 pDst c!
                endif
              else
                if( c 0x0A = )
                  if( not( noTrimEol ) )
                    0 pDst c!
                  endif
                  true -> done
                  leave
                endif
              endif
            endif
            1 ->+ pDst
          loop
        else
          //error( "getLine fgets" )
          true -> done
        endif
      until( or( done feof( fp ) ) )
    else
      error( "getLine - not open" )
    endif
    
    result
  ;m

  // BOOL ...
  m: setUnixEol
    -> unixEol
  ;m
  
  // ... BOOL
  m: getUnixEol
    unixEol
  ;m
  
  // BOOL ...
  m: setTrimEol
    not -> noTrimEol
  ;m

  // ... BOOL
  m: getTrimEol
    noTrimEol not
  ;m
    
  // ... BOOL
  m: eof
    feof( fp )
  ;m

  // ... FILE_LENGTH
  m: len
    flen( fp )
  ;m

  // ... FILE_POSITION  
  m: tell
    ftell( fp )
  ;m

  // FILE_POSITION ...  
  m: seekAbs
    if( fseek( fp swap 0 ) )
      error( "seekAbs" )
    endif
  ;m
  
  // FILE_POSITION ...  
  m: seekRel
    if( fseek( fp swap 1 ) )
      error( "seekAbs" )
    endif
  ;m

  m: seekEnd
    if( fseek( fp swap 2 ) )
      error( "seekAbs" )
    endif
  ;m
  
  // BUFFER_SIZE_BYTES ...
  m: setBufferSize
    if( buffer objIsNull )
      new ByteArray -> buffer
    endif
    buffer.resize
  ;m
    
;class



loaddone

