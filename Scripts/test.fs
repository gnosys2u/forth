
autoforget TEST
: TEST ;

decimal

\ FileOutStream testFileOut

scSetShowID(false)

cd tests
lf testbase
cd ..

: runtest
  depth -> int oldDepth
  mko String testName
  mko String testScriptName
  testName.set(blword)
  testScriptName.set(testName.get) testScriptName.append(".fs")
  "##################### run " %s testScriptName.get %s " #####################\n" %s
  $runFile(testScriptName.get)
  0 $word $evaluate
  if(depth oldDepth <>)
    err[
      "!!!!!!!!!!!!!!!!!!!!!! stack depth changed after " %s
      testScriptName.get %s " !!!!!!!!!!!!!!!!!!!!!!\n" %s
      ds
    ]err
  endif
  oclear testName
  oclear testScriptName
  \ showMemory
;

runtest helpdeck   help classes   forget HELPDECK
runtest evil forget EVIL
runtest philosophers   go   goAsync   forget PHILOSOPHERS
runtest pizzashop   go   goAsync   forget PIZZASHOP
200 -> int numElems
runtest sort_examples   go   forget SORT_EXAMPLES
runtest tree_examples   go   forget TREE_EXAMPLES
runtest directory   mko Directory dd   test[ dd.openHere -1 = ]  dd.listFilenames
   oclear dd   forget Directory
cd adv runtest adv   restore full350   forget ADV   cd ..
cd atc runtest atc   forget ATC   cd ..

: _TEST_ ;
cd system
runtest hersheyfont
runtest file
runtest rbtree
cd ..
runtest maze  forget _TEST_

cd tests
runtest biclasstest   cleanup   forget BUILTIN_CLASS_TEST
runtest rctest   test   cleanup   forget RCTEST
runtest structtest   forget STRUCTTEST
runtest misctest   forget MISCTEST
runtest forthtest
runtest vartest
\ runtest ansitest

dumpTestSummary
cleanupTestBase

forget TEST
cd ..

