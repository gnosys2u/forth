
: hersheyfont ;

class: HersheyGlyph
  int mLeftPos
  int mRightPos
  int mTopDY
  int mBottomDY
  
  // mCoords has 2 bytes per vertex, first byte is (y coord + 64) with move flag in hibit, second byte is (x coord + 64)
  ByteArray mCoords

  // the coordinates in hershey glyphs are offsets from a position in the rough center of the character.
  // mLeftPos is the offset from the center to the left edge of the character, and should be negative.
  // mRightPos is the offset from the center to the right edge of the character, and should be positive.
  // Assuming that your character draw position is specified as the left edge of the character box, you
  // should start by subtracting mLeftPos from the draw position X to get the character center X.
  // After you are done drawing and want to move to the next character, the new draw position is gotten by
  // adding mRightPos to the character center X.
  
  m: init      // leftPos rightPos
    -> mRightPos
    -> mLeftPos
    new ByteArray -> mCoords
  ;m
  
  m: delete
    oclear mCoords
  ;m

  m: addVertex  // x y isMove ...
    // x and y are in range -64...63
    if
      64+ 0x80 or
    else
      64+
    endif
    mCoords.push      // add y+isMove
    64+ mCoords.push  // add x
  ;m
  
  m: numVertices
    mCoords.count 2/
  ;m
  
  m: getVertex // index ... x y isMove
    2* dup 1+ mCoords.get 64-   // x

    swap mCoords.get
    if(dup 0x80 and)
      0x7F and 64- true
    else
      64- false
    endif
  ;m
  
  m: getWidth
    mRightPos mLeftPos -
  ;m
  
;class


class: HersheyFont

  IntMap mGlyphs
  String mFilename
  int mMaxWidth     // widest glyph
  int mMinY         // 
  int mMaxY
  // these are the top and bottom vertical offsets for capital M
  int mTopDY
  int mBottomDY

  m: delete
    oclear mGlyphs
    oclear mFilename
  ;m
    
  m: loadFromFile   // FILENAME ... SUCCESS
    0 -> mMaxWidth
    0 -> mMinY
    0 -> mMaxY
    new String -> mFilename
    mFilename.set
    fopen(mFilename.get "r") -> int inFile
    if(inFile)
      new IntMap -> mGlyphs
      new ByteArray -> ByteArray buffer
      buffer.resize(2048) // biggest number of verts is 999, biggest string should be 1998
      int glyphIndex
      int numVerts
      32 -> int nextGlyphIndex
      byte x
      byte y
      true -> int keepGoing
      begin
      while(keepGoing)
        fscanf(inFile "%5d%3d" r[ ref glyphIndex ref numVerts ]r) -> int numResults
        if(numResults 2 =)
          fgets(buffer.base buffer.count 4- inFile) -> int numCharsRead
          if(numCharsRead 2/ numVerts >=)
          
            // index 12345 means assign next sequential index
            if(glyphIndex 12345 =)
              nextGlyphIndex -> glyphIndex
              1 ->+ nextGlyphIndex
            endif
            1 ->- numVerts
            
            buffer.base -> int buffptr
            ref buffptr -> int pSrc
            true -> int isMove
            
            new HersheyGlyph -> HersheyGlyph glyph
            mGlyphs.set(glyph glyphIndex)
            // set leftPos and rightPos
            glyph.init(pSrc b@@++ `R` -   pSrc b@@++ `R` -)
            glyph.mRightPos glyph.mLeftPos - -> int glyphWidth
            if(glyphWidth mMaxWidth >)
              glyphWidth -> mMaxWidth
            endif
            fprintf(stdout "glyph %d with %d verts  l:%d r:%d w:%d\n" r[ glyphIndex numVerts glyph.mLeftPos glyph.mRightPos glyphWidth ]r) drop
            ?do(numVerts 0)
              pSrc b@@++ -> byte cx
              pSrc b@@++ -> byte cy
              if(and(cx ` ` = cy `R` =))
                true -> isMove
              else
                // offset original -64..63 range to 0..127 so we can use high bit of x to flag move mode
                `R` ->- cx   `R` ->- cy
                if(cy mMinY <)
                  cy -> mMinY
                endif
                if(cy mMaxY >)
                  cy -> mMaxY
                endif
                fprintf(stdout "%s to %d,%d\n" r[ if(isMove) "Move" else "Draw" endif cx cy ]r) drop
                glyph.addVertex(cx cy isMove)
                false -> isMove
              endif
            loop
          else
            false -> keepGoing
            fprintf(stdout "Only read %d characters, expected at least %d\n" r[ numCharsRead numVerts 2* ]r) drop
          endif
          if(feof(inFile))
            false -> keepGoing
          endif
        else
          false -> keepGoing
        endif
      repeat
      fclose(inFile) drop
      oclear buffer
      true
    else
      mFilename.get %s " not found!\n" %s
      false
    endif
  ;m
  
;class
  

loaddone

-------------------
// Most .jhf files use 12345 for all glyphnums, this means the glyphs are in ASCII order starting at ascii 33.
// rowmans.jhf is a file which doesn't use 12345, I don't know how these glyph codes are supposed to be used.
12345 94G]IHKFMFOGQF RLGNG RIHKGMHOHQF RNKMLLNLOJOIPIRJQLQLW RMMMU RJPMP RNKNTMVLW RQMRJSHTGVFXF[G RTHVGXGZH RRJSIUHWHYI[G RQURRSPTOVOXP RTPVPWQ RRRSQUQVRXP RK[NYRXWX[Y RMZPYWYZZ RK[OZVZY[[Y RQMQX

12345 	glyphnum, 12345 is special meaning use ascending ASCII ordering, starting at ASCII 33
 94	  	# vertices (3 digit field)
G		leftPos
]		rightPos
IHKFMFOGQF RLGNG RIHKGMHOHQF RNKMLLNLOJOIPIRJQLQLW RMMMU RJPMP RNKNTMVLW RQMRJSHTGVFXF[G RTHVGXGZH RRJSIUHWHYI[G RQURRSPTOVOXP RTPVPWQ RRRSQUQVRXP RK[NYRXWX[Y RMZPYWYZZ RK[OZVZY[[Y RQMQX
coord pairs are relative to 'R'
" R"	next coord pair is a move
