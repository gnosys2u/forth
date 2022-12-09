//
// support for appending files to a forth executable
//

// a temporary archive file named fortharc.bin is created

autoforget make_archive

: make_archive ;

// the format is:
//
// PER FILE:
// ... file1 databytes...
// length of file1 as int
// 0xDEADBEEF
// ... file1name ...
// length of file1name as int
//
// ... fileZ databytes...
// length of fileZ as int
// 0xDEADBEEF
// ... fileZname ...
// length of fileZname as int
//
// number of appended files as int
// 0x34323137

: chekit
  -> int fname
  -> int msg
  0= if
    "FAILURE: " %s msg %s %bl fname %s %nl "makeExe" error
  endif
;

: write32
  -> int msg
  -> int val
  -> int outFile
  fwrite( ref val 4 1 outFile ) 1 = "Writing longword" msg chekit
;
  
: appendFile
  -> int exeFile
  -> int srcName
  
  int buff
  int srcLen
  int srcFile
  
  fopen( srcName "rb" ) dup -> srcFile "Opening" srcName chekit
  flen( srcFile ) -> srcLen
  "Appending " %s srcLen %d " bytes from " %s srcName %s " at 0x" %s ftell( exeFile ) %x %nl
  malloc( srcLen ) -> buff
  fread( buff srcLen 1 srcFile ) 1 = "Reading" srcName chekit
  fclose( srcFile ) drop
  fwrite( buff srcLen 1 exeFile ) 1 = "Writing" srcName chekit
  write32( exeFile srcLen "file length" )
  write32( exeFile 0xDEADBEEF "magic1" )
  strlen( srcName ) -> srcLen
  fwrite( srcName srcLen 1 exeFile ) 1 = "Writing filename of" srcName chekit
  write32( exeFile srcLen "filename length" )
  free( buff )
;

// r[ ...SOURCEFILES... ]r EXE_FILENAME makeExe
: makeExe
  -> 128 string exeName
  -> int fileCount
  128 string srcName
  200 string errMsg
  null -> int srcFile
  null -> int exeFile
  int buff
  int srcLen
  
  fopen( exeName "ab" ) dup -> exeFile "Opening" exeName chekit
  fseek( exeFile 0 2 ) 0= "Seek to end" exeName chekit
  fileCount 0 do
    exeFile appendFile
  loop
  fwrite( ref fileCount 4 1 exeFile ) 1 = "Writing filecount to" exeName chekit
  0x37313234 -> buff
  fwrite( ref buff 4 1 exeFile ) 1 = "Writing magic2 to" exeName chekit
  fclose( exeFile ) drop
;

: testArchive
  system( "copy ForthMain.exe test.exe" ) drop
  r[ "forth_autoload.txt" "asm_pentium.txt" ]r "test.exe" makeExe
;

: testArchive2
  system( "copy ForthMain.exe test.exe" ) drop
  system( "copy lineend_autoload.txt app_autoload.txt" ) drop
  r[ "lineend.txt" "app_autoload.txt" ]r "test.exe" makeExe
  system( "erase app_autoload.txt" ) drop
;


loaddone

: testArchive
  system( "copy ForthMain.exe test.exe" ) drop
  r[ "forth_autoload.txt" "asm_pentium.txt" ]r "test.exe" makeExe
;

: makeExe
  -> 128 string exeName
  -> int fileCount
  128 string srcName
  200 string errMsg
  null -> int srcFile
  null -> int exeFile
  int buff
  int srcLen
  
  fopen( exeName "ab" ) dup -> exeFile "Opening" exeName chekit
  fseek( exeFile 0 2 ) 0= "Seek to end" exeName chekit
  fileCount 0 do
#if 0
    -> srcName
    "Appending " %s srcName %s " at 0x" %s ftell( exeFile ) %x %nl
    fopen( srcName "rb" ) dup -> srcFile "Opening" srcName chekit
    flen( srcFile ) -> srcLen
    malloc( srcLen ) -> buff
    fread( buff srcLen 1 srcFile ) 1 = "Reading" srcName chekit
    fclose( srcFile ) drop
    fwrite( buff srcLen 1 exeFile ) 1 = "Writing" srcName chekit
    srcLen buff !
    fwrite( buff 4 1 exeFile ) 1 = "Writing length of" srcName chekit
    0xDEADBEEF buff !
    fwrite( buff 4 1 exeFile ) 1 = "Writing magic after" srcName chekit
    srcName strlen -> srcLen
    fwrite( srcName srcLen 1 exeFile ) 1 = "Writing filename of" srcName chekit
    fwrite( ref srcLen 4 1 exeFile ) 1 = "Writing filename length of" srcName chekit
    free( buff )
#endif
  loop
  fwrite( ref fileCount 4 1 exeFile ) 1 = "Writing filecount to" exeName chekit
  0x37313234 buff !
  fwrite( buff 4 1 exeFile ) 1 = "Writing magic2 to" exeName chekit
  fclose( exeFile ) drop
;



: write32
  -> int msg
  -> int val
  -> int outFile
  fwrite( ref val 4 1 outFile ) 1 = "Writing longword" msg chekit
;
  
: appendFile
  -> int exeFile
  -> int srcName
  
  int buff
  int srcLen
  int srcFile
  
  fopen( srcName "rb" ) dup -> srcFile "Opening" srcName chekit
  flen( srcFile ) -> srcLen
  "Appending " %s srcLen %d " bytes from " %s srcName %s " at 0x" %s ftell( exeFile ) %x %nl
  malloc( srcLen ) -> buff
  fread( buff srcLen 1 srcFile ) 1 = "Reading" srcName chekit
  fclose( srcFile ) drop
  fwrite( buff srcLen 1 exeFile ) 1 = "Writing" srcName chekit
  write32( exeFile srcLen "file length" )
  write32( exeFile 0xDEADBEEF "magic1" )
  strlen( srcName ) -> srcLen
  fwrite( srcName srcLen 1 exeFile ) 1 = "Writing filename of" srcName chekit
  write32( exeFile srcLen "filename length" )
  //free( buff )
;
