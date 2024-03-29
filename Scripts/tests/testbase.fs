autoforget testbase
: testbase ;

false -> int beNoisy
0 -> int numFailedTests

: testFailed  numFailedTests 0<> ;

String testBuff
String testBuff2
StringOutStream testOut
Array testStreamStack

SplitOutStream splitErrorOut
StringOutStream testSummaryOut
String summaryBuffer
Object oldErrorOut

: setupTestBase
  new String -> testBuff
  new String -> testBuff2
  new StringOutStream -> testOut
  new Array -> testStreamStack

  \ turn error out into a split that writes to both error stream and testSummaryOut
  \  so we can dump testSummaryOut at end of all tests so failures don't get
  \  lost in all the noise
  \ this does require that tests wrap error output in err[ ... ]err
  new SplitOutStream -> splitErrorOut

  new StringOutStream -> testSummaryOut
  new String -> summaryBuffer
  testSummaryOut.setString(summaryBuffer)

  getErrorOut -> oldErrorOut
  splitErrorOut.init(oldErrorOut testSummaryOut)
  setErrorOut(splitErrorOut)
;

: cleanupTestBase
  oclear testBuff  oclear testBuff2
  oclear testOut
  oclear testStreamStack

  setErrorOut(oldErrorOut)
  oclear oldErrorOut

  oclear testSummaryOut
  \ oclear summaryBuffer
  oclear splitErrorOut
;

: dumpTestSummary
  "\nSUMMARY:\n" %s
  summaryBuffer.get %s %nl
;

: clearBuff testBuff.clear ;
: addBuff testBuff.append ;

: outToTestBuffer
  -> String stringBuff
  stringBuff.clear
  testOut.setString( stringBuff )
  setConsoleOut( testOut )
  oclear stringBuff
;

: startTest outToTestBuffer( testBuff ) ;

: checkResult
  -> ptrTo byte expected
  outToScreen
  if( strcmp( testBuff.get expected ) )
    err[ "Expected |" %s expected %s "| got |" %s testBuff.get %s "|" %s %nl ]err
    "--------------------------------" %s %nl
    testBuff.get dup strlen dump
    "--------------------------------" %s %nl
    expected dup strlen dump
    "--------------------------------" %s %nl
    false
    1 ->+ numFailedTests
  else
    if( beNoisy )
      expected %s " passed\n" %s
	endif
    true
  endif
;

: 2=
  rot = if
    =
  else
    2drop false
  endif
;

\ test[ LINE_OF_STUFF ]
\ LINE_OF_STUFF must evaluate to one or more true (-1) values
: test[
  sp -> ptrTo int oldSP
  ']' $word string( 250 ) ops!
  \ ops %s %nl
  ops $evaluate
  sp oldSP swap - cell/ -> int numItems
  false -> int failed
  do( numItems 0 )
    if( not( pick( i ) ) )
      true -> failed
    endif
  loop

  if( failed )
    ptrTo byte inputLine
    ptrTo byte inputPath
    cell lineNumber
    cell lineOffset
    system.getInputInfo -> lineOffset -> lineNumber -> inputPath -> inputLine
    
    err[
      "FAILED: " %s inputPath %s ':' %c lineNumber %d " { " %s
      ops %s " } => " %s
      do( numItems 0 )
        i %d "=" %s pick( numItems 1- i - ) . %bl %nl
      loop
    ]err
    
    1 ->+ numFailedTests
  else
    if( beNoisy )
      ops %s " passed\n" %s
	endif
  endif
  
  do( numItems 0 )
    drop
  loop
;

\ FILENAME N_VALUES N ...
: makeShortFile
  mko ByteArray buffer
  mko FileOutStream outFile
  buffer.load
  -> ptrTo byte outFileName
  
  if( outFile.open(outFileName "wb") )
    outFile.putBytes(buffer.base buffer.count)
    outFile.close
  else
    addErrorText(outFileName)
    error("makeShortFile: failed to open ")
  endif
  
  oclear outFile
  oclear buffer
;

\ FILENAME FILE_CONTENTS ...
: makeFileFromString
  mko FileOutStream outFile
  -> ptrTo byte contents
  -> ptrTo byte outFileName
  
  if(outFile.open(outFileName "wb"))
    outFile.putBytes(contents strlen(contents))
    outFile.close
  else
    addErrorText(outFileName)
    error("makeFileFromString: failed to open ")
  endif
  oclear outFile
;

setupTestBase

: showPassFail
  if(numFailedTests)
    " - " %s numFailedTests %d " failed\n" %s
  else
    " - all tests succeeded\n" %s
  endif
;

loaddone
