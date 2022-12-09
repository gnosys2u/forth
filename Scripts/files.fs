//
// something to display info from titan quest .que files
//

int _infile
0 -> _infile

int _inbuff
here 4096 allot -> _inbuff

// open for read
: openr
  "rb" fopen -> _infile
  _infile 0= if
    "open failure" %s %nl
  endif
;

: closer
  _infile 0<> if
    fclose drop
    0 -> _infile
  endif
;

// get stuff from open file
: getc
  _infile fgetc
;

: getw
  _inbuff 1 2 _infile fread
  0= if
    "getw read failure" %nl
  endif
  _inbuff w@
;

: getl
  _inbuff 1 4 _infile fread
  0= if
    "getl read failure" %nl
  endif
  _inbuff @
;


: charArray builds allot align does + ;
: longArray builds 4* allot does swap 2 lshift + ;

1000 charArray specialNames
int specialNamesNext   0 specialNames -> specialNamesNext
20 longArray nameIndex
int nameIndexNext   0 -> nameIndexNext

: addName
  specialNamesNext nameIndexNext nameIndex !
  1 ->+ nameIndexNext
  specialNamesNext swap strcpy
  specialNamesNext dup strlen 1+ + -> specialNamesNext
;

"baseName" addName
"prefixName" addName
"suffixName" addName

: findName
  vars
    int foundIt
    int name
  endvars
  -> name
  0 -> foundIt
  nameIndexNext 0 do
    i nameIndex @ name strcmp
    0= if
      1 -> foundIt
      leave
    endif
  loop
  foundIt
;


// something to display info from titan quest .que files

: getq
  _infile ftell %x %bl
  getl dup %d %bl
  _inbuff over 1 _infile fread
  0= if
    "getq read failure" %nl
  endif
  0 swap _inbuff + c!
  _inbuff %s %bl
  _inbuff findName
  if
    getl
    _inbuff over 1 _infile fread
    0= if
      "getq read failure" %nl
    endif
    0 swap _inbuff + c!
    _inbuff %s %bl
  else
    getl %x %nl
  endif
;

