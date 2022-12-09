
autoforget forth_optype
: forth_optype ;

base @
features
kFFRegular -> features

vocabulary opType
also opType
definitions

decimal

// OPTYPE8 OPVALUE24 --- OPCODE32
: makeOpcode
  0xFFFFFF and
  swap 24 lshift
  or
;

: getOptype
  24 rshift 0xFF and
;

enum: _opType
  native
  nativeImmediate
  userDef					// low 24 bits is op number (index into ForthCoreState userOps table)
  userDefImmediate
  CCode					// low 24 bits is op number (index into ForthCoreState userOps table)
  CCodeImmediate
  dllEntryPoint				// bits 0:18 are index into ForthCoreState userOps table 19:23 are arg count
  
  // 7 - 9 are unused
     
  10 branch					// low 24 bits is signed branch offset
  branchNZ
  branchZ
  caseBranch
  
  // 14 - 19 are unused
     
  20 litInt					// low 24 bits is signed symbol value
  litString					// low 24 bits is number of longwords to skip over
  addOffset					// low 24 bits is signed offset value
  arrayOffset				// low 24 bits is array element size TOS is array base NTOS is index
  allocLocals				// low 24 bits is frame size in longs
  localRef					// low 24 bits is offset in bytes
  initLocalString			// bits 0:11 are string length in bytes bits 12:23 are frame offset in longs
  localStructArray			// bits 0:11 are padded struct size in bytes bits 12:23 are frame offset in longs
  offsetFetch				// low 24 bits is signed offset in longs TOS is long ptr
  memberRef					// low 24 bits is offset in bytes
     
  30 localByte				// low 24 bits is offset in bytes
  localUByte
  localShort
  localUShort
  localInt
  localUInt
  localLong
  localULong
  localFloat
  localDouble
  localString
  localOp
  localObject

  43 localByteArray			// low 24 bits is offset in bytes, TOS is index
  localUByteArray
  localShortArray
  localUShortArray
  localIntArray
  localUIntArray
  localLongArray
  localULongArray
  localFloatArray
  localDoubleArray
  localStringArray
  localOpArray
  localObjectArray
     
  56 fieldByte				// low 24 bits is offset in bytes
  fieldUByte
  fieldShort
  fieldUShort
  fieldInt
  fieldUInt
  fieldLong
  fieldULong
  fieldFloat
  fieldDouble
  fieldString
  fieldOp
  fieldObject
     
  69 fieldByteArray			// low 24 bits is offset in bytes, TOS is index
  fieldUByteArray
  fieldShortArray
  fieldUShortArray
  fieldIntArray
  fieldUIntArray
  fieldLongArray
  fieldULongArray
  fieldFloatArray
  fieldDoubleArray
  fieldStringArray
  fieldOpArray
  fieldObjectArray
     
  82 memberByte				// low 24 bits is offset in bytes
  memberUByte
  memberShort
  memberUShort
  memberInt
  memberUInt
  memberLong
  memberULong
  memberFloat
  memberDouble
  memberString
  memberOp
  memberObject
     
  95 memberByteArray		// low 24 bits is offset in bytes, TOS is index
  memberUByteArray
  memberShortArray
  memberUShortArray
  memberIntArray
  memberUIntArray
  memberLongArray
  memberULongArray
  memberFloatArray
  memberDoubleArray
  memberStringArray
  memberOpArray
  memberObjectArray
     
  108 methodWithThis		// low 24 bits is method number
  methodWithTOS				// low 24 bits is method number
  memberStringInit			// bits 0:11 are string length in bytes, bits 12:23 are frame offset in longs
  nvoCombo					// NUM VAROP OP combo - bits 0:10 are signed integer, bits 11:12 are varop-2, bit 13 is builtin/userdef, bits 14-23 are opcode
  nvCombo					// NUM VAROP combo - bits 0:21 are signed integer, bits 22:23 are varop-2
  noCombo					// NUM OP combo - bits 0:12 are signed integer, bit 13 is builtin/userdef, bits 14:23 are opcode
  voCombo					// VAROP OP combo - bits 0:1 are varop-2, bit 2 is builtin/userdef, bits 3:23 are opcode
	 
  // 115 is unused
  116 localUserDefined		// user can add more optypes starting with this one
  127 maxLocalUserDefined	// maximum user defined optype

  // 128 userMethods - I don't think these are used
  // optypes from 128:.255 are used to select class methods    
;enum

: isImmediate
  opType:nativeImmediate over =  swap opType:userDefImmediate over =  swap opType:CCodeImmediate =  or or
;

previous
-> features
base !

