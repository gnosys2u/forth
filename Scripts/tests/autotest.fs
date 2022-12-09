autoforget _testfile

int _testfile
// run the next line when more tests have been added and you need to regenerate the "known good" output file
// do this only if you are sure that forth isn't broken and more tests have been added
// "good_test_output.txt" "w" fopen -> _testfile _testfile outToFile load forthtest.txt outToScreen _testfile fclose drop

: checktest
  -> 100 string fname
  fname "good_test_output.txt" compareTextFiles
  not if
    fname %s " didn't match known good output!\n" %s
  endif
;

"testing slow mode... " %s
ms@
"slow_test_output.txt" "w" fopen -> _testfile
_testfile outToFile
load forthtest.txt
outToScreen
_testfile fclose drop
ms@ swap - i2f 1000.0 f/ "test completed in " %s %f " seconds\n" %s
"slow_test_output.txt" checktest

"testing turbo mode... " %s
ms@
turbo "turbo_test_output.txt" "w" fopen -> _testfile
_testfile outToFile
load forthtest.txt
outToScreen
_testfile fclose drop turbo
ms@ swap - i2f 1000.0 f/ "test completed in " %s %f " seconds\n" %s
"turbo_test_output.txt" checktest

loaddone

ms@ dup %d %bl
"slow_test_output.txt" "w" fopen -> _testfile _testfile outToFile load forthtest.txt outToScreen _testfile fclose drop
ms@ dup %d %bl swap - i2f 1000.0 f/ "test completed in " %s %f " seconds\n" %s


