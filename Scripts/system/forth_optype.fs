
autoforget forth_optype
: forth_optype ;

base @
features
kFFRegular -> features

vocabulary opType
also opType
definitions

decimal

\ OPTYPE8 OPVALUE24 --- OPCODE32
: makeOpcode
  $FFFFFF and
  swap 24 lshift
  or
;

: getOptype
  24 rshift $FF and
;

enum: _opType
  native
  nativeImmediate
  userDef					\ low 24 bits is op number (index into ForthCoreState userOps table)
  userDefImmediate
  cCode					\ low 24 bits is op number (index into ForthCoreState userOps table)
  cCodeImmediate
  relativeDef               \ low 24 bits is offset from dictionary base
  relativeDefImmediate
  dllEntryPoint				\ bits 0:18 are index into ForthCoreState userOps table 19:23 are arg count
  
  \ 9 is unused
     
  10 branch					\ low 24 bits is signed branch offset
  branchNZ
  branchZ
  caseBranchT
  caseBranchF
  pushBranch
  relativeDefBranch
  relativeData              \ low 24 bits is offset from dictionary base
  relativeString
  
  \ 19 is unused
     
  20 litInt					\ low 24 bits is signed symbol value
  litString					\ low 24 bits is number of longwords to skip over
  addOffset					\ low 24 bits is signed offset value
  arrayOffset				\ low 24 bits is array element size, TOS is array base, NTOS is index
  allocLocals				\ low 24 bits is frame size in longs
  localRef					\ low 24 bits is offset in bytes
  initLocalString			\ bits 0:11 are string length in bytes bits 12:23 are frame offset in longs
  localStructArray			\ bits 0:11 are padded struct size in bytes bits 12:23 are frame offset in longs
  offsetFetch				\ low 24 bits is signed offset in longs TOS is long ptr
  memberRef					\ low 24 bits is offset in bytes
     
  30 localByte				\ low 24 bits is offset in bytes
  localUByte
  localShort
  localUShort
  localInt
  localUInt
  localLong
  localULong
  localSFloat
  localSFloat
  localString
  localOp
  localObject

  43 localByteArray			\ low 24 bits is offset in bytes, TOS is index
  localUByteArray
  localShortArray
  localUShortArray
  localIntArray
  localUIntArray
  localLongArray
  localULongArray
  localSFloatArray
  localFloatArray
  localStringArray
  localOpArray
  localObjectArray
     
  56 fieldByte				\ low 24 bits is offset in bytes
  fieldUByte
  fieldShort
  fieldUShort
  fieldInt
  fieldUInt
  fieldLong
  fieldULong
  fieldSFloat
  fieldFloat
  fieldString
  fieldOp
  fieldObject
     
  69 fieldByteArray			\ low 24 bits is offset in bytes, TOS is index
  fieldUByteArray
  fieldShortArray
  fieldUShortArray
  fieldIntArray
  fieldUIntArray
  fieldLongArray
  fieldULongArray
  fieldSFloatArray
  fieldFloatArray
  fieldStringArray
  fieldOpArray
  fieldObjectArray
     
  82 memberByte				\ low 24 bits is offset in bytes
  memberUByte
  memberShort
  memberUShort
  memberInt
  memberUInt
  memberLong
  memberULong
  memberSFloat
  memberFloat
  memberString
  memberOp
  memberObject
     
  95 memberByteArray		\ low 24 bits is offset in bytes, TOS is index
  memberUByteArray
  memberShortArray
  memberUShortArray
  memberIntArray
  memberUIntArray
  memberLongArray
  memberULongArray
  memberSFloatArray
  memberFloatArray
  memberStringArray
  memberOpArray
  memberObjectArray
     
  108 methodWithThis		\ low 24 bits is method number
  methodWithTOS				\ low 24 bits is method number
  memberStringInit			\ bits 0:11 are string length in bytes, bits 12:23 are frame offset in longs
  nvoCombo					\ NUM VAROP OP combo - bits 0:10 are signed integer, bits 11:12 are varop-2, bit 13 is builtin/userdef, bits 14-23 are opcode
  nvCombo					\ NUM VAROP combo - bits 0:21 are signed integer, bits 22:23 are varop-2
  noCombo					\ NUM OP combo - bits 0:12 are signed integer, bit 13 is builtin/userdef, bits 14:23 are opcode
  voCombo					\ VAROP OP combo - bits 0:1 are varop-2, bit 2 is builtin/userdef, bits 3:23 are opcode
  oZBCombo                  \ OP ZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
  oNZBCombo                 \ OP NZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs

  squishedFloat             \ low 24 bits is float as sign bit, 5 exponent bits, 18 mantissa bits
  squishedDouble            \ low 24 bits is double as sign bit, 5 exponent bits, 18 mantissa bits
  squishedLong              \ low 24 bits is value to be sign extended to 64-bits (only used on 32-bit systems)

  120 localRefOpCombo       \ LOCAL_REF OP - bits 0:11 are local var offset in longs, bits 12:23 are opcode
  memberRefOpCombo          \ MEMBER_REF OP - bits 0:11 are local var offset in longs, bits 12:23 are opcode

  methodWithSuper           \ low 24 bits is method number

  \ 123-127 nativeOp + immediately following 32 or 64 bit literal - literal is pushed before op is executed
  nativeU32         \ 32-bit unsigned int
  nativeS32         \ 32-bit signed int
  nativeSF32        \ 32-bit float
  nativeS64         \ 64-bit signed long
  nativeF64         \ 64-bit float (previously AKA double)
  
  \ 128-132 C code op + immediately following 32 or 64 bit literal - literal is pushed before op is executed
  cCodeU32
  cCodeS32
  cCodeSF32
  cCodeS64
  cCodeF64
  
  \ 128-132 user define op + immediately following 32 or 64 bit literal - literal is pushed before op is executed
  \ userDefXXX are currently not being compiled, are untested and may not even be implemented at all
  userDefU32
  userDefS32
  userDefSF32
  userDefS64
  userDefF64
  
  methodWithLocalObject     \ bits 0..11 are method index, bits 12..23 are frame offset in longs
  methodWithMemberObject    \ bits 0..11 are method index, bits 12..23 are object offset in longs

  methodWithMemberObject lastBaseDefinedOptype    

  192 localUserDefined		\ user can add more optypes starting with this one
  255 maxLocalUserDefined	\ maximum user defined optype

;enum

: isImmediate
  opType:nativeImmediate over =  swap opType:userDefImmediate over =  swap opType:cCodeImmediate =  or or
;

previous definitions
-> features
base !

