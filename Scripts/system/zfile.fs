
requires zlib

: zfile ;

also zlib definitions

class: ZFileInStream extends OFileInStream

  m: open
    -> ptrTo byte pFilename
    gzopen(pFilename "r") -> userData
    if(userData)
      gzbuffer(userData 0x20000) drop
    else
      "ZFileInStream:open - could not open " %s pFilename %s %nl
    endif
  ;m

  m: close
    if(userData)
      gzclose(userData) drop
      0 -> userData
    endif
  ;m
  
  m: atEOF
    if(userData)
      gzeof(userData)
    else
      -1
    endif
  ;m
  
  m: delete
    close
    super.delete
  ;m
  
  m: getChar
    if(userData)
      gzgetc(userData)
    else
      -1
    endif
  ;m
  
  m: getBytes
    -> int numToRead
    -> ptrTo byte pBuffer
    if(userData)
      gzread(userData pBuffer numToRead)
    else
      0
    endif
  ;m
  
  m: getLine
    -> int bufferSize
    -> ptrTo byte pBuffer
    0 -> int resultRead
    0 pBuffer c!
    if(userData)
      gzgets(userData pBuffer bufferSize) -> ptrTo byte pResult
      if(pResult)
        strlen(pResult) -> int numRead
        if(numRead bufferSize >=)
          "ZFileInStream:getLine - read " %s numRead %d " bytes, buffer can only hold " %s bufferSize %d %nl
          bufferSize 1- -> numRead
        endif
        if(pResult pBuffer <>)
          move(pResult pBuffer numRead)
          0 pBuffer numRead + c!
        endif
      else
        0
      endif
    endif
    resultRead
  ;m
  
;class


class: ZFileOutStream extends OFileOutStream

  m: open
    -> ptrTo byte pFilename
    gzopen(pFilename "wb") -> userData
    if(userData)
      gzbuffer(userData 0x20000) drop
    else
      "ZFileOutStream:open - could not open " %s pFilename %s %nl
    endif
  ;m

  m: close
    if(userData)
      gzclose(userData) drop
      0 -> userData
    endif
  ;m
  
  m: delete
    close
    super.delete
  ;m
  
  m: putChar
    if(userData)
      gzputc(userData swap)
    endif
    drop
  ;m
  
  m: putBytes
    -> int numToRead
    -> ptrTo byte pBuffer
    if(userData)
      gzwrite(userData pBuffer numToRead) drop
    endif
  ;m
  
  m: putString
    -> ptrTo byte pBuffer
    if(userData)
      gzputs(userData pBuffer) drop
    endif
  ;m
  
;class


loaddone


		METHOD("getChar", unimplementedMethodOp),			// derived classes must define getChar
		METHOD("getBytes", oInStreamGetBytesMethod),
		METHOD("getLine", unimplementedMethodOp),			// derived classes must define getLine (for now)
		METHOD("atEOF", unimplementedMethodOp),				// derived classes must define atEOF

dll_2   gzopen        // gzFile gzopen OF((const char *, const char *));
dll_1   gzclose       // int    gzclose OF((gzFile file));
dll_3   gzread        // int gzread OF((gzFile file, voidp buf, unsigned len));

dll_3   gzgets        // char * gzgets OF((gzFile file, char *buf, int len));
dll_1   gzgetc        // int gzgetc OF((gzFile file));
dll_3   gzwrite       // int gzwrite OF((gzFile file, voidpc buf, unsigned len));

dll_1   gzeof         // int gzeof OF((gzFile file));

// Set the internal buffer size used by this library's functions.  The default buffer size is 8192 bytes.  64K or more is recommended.
dll_2   gzbuffer      // int gzbuffer OF((gzFile file, unsigned size));

dll_3   gzwrite       // int gzwrite OF((gzFile file, voidpc buf, unsigned len));
dll_2   gzputs        // int gzputs OF((gzFile file, const char *s));
dll_2   gzputc        // int gzputc OF((gzFile file, int c));
