#pragma once
//////////////////////////////////////////////////////////////////////
//
// Forth engine definitions
//   Pat McElhatton   September '00
//
//////////////////////////////////////////////////////////////////////

//#include "pch.h"

#include <atomic>
#include "ForthMemoryManager.h"

struct CoreState;

#define ATOMIC_REFCOUNTS 1

#if defined(WIN32) || defined(_WIN64)
#define WINDOWS_BUILD
#endif

// forthop is the type of forth opcodes
// cell/ucell is the type of parameter stack elements
#ifdef FORTH64
#define forthop uint32_t
#define cell int64_t
#define ucell uint64_t
#else
#define forthop uint32_t
#define cell int32_t
#define ucell uint32_t
#endif

#define MAX_STRING_SIZE (8 * 1024)

#define DEFAULT_BASE 10


// these are opcode types, they are held in the top byte of an opcode, and in
// a vocabulary entry value field
// NOTE: if you add or reorder op types, make sure that you update Engine::opTypeNames
typedef enum
{
    kOpNative = 0,
    kOpNativeImmediate,
    kOpUserDef,         // low 24 bits is op number (index into CoreState userOps table)
    kOpUserDefImmediate,
    kOpCCode,         // low 24 bits is op number (index into CoreState userOps table)
    kOpCCodeImmediate,
    kOpRelativeDef,         // low 24 bits is offset from dictionary base
    kOpRelativeDefImmediate,
    kOpDLLEntryPoint,   // bits 0:18 are index into CoreState userOps table, 19:23 are arg count
    // 9 is unused

    kOpBranch = 10,          // low 24 bits is signed branch offset
    kOpBranchNZ,
    kOpBranchZ,
    kOpCaseBranchT,
    kOpCaseBranchF,
    kOpPushBranch,
	kOpRelativeDefBranch,
    kOpRelativeData,
    kOpRelativeString,
    // 19 is unused

    kOpConstant = 20,   // low 24 bits is signed symbol value
    kOpConstantString,  // low 24 bits is number of longwords to skip over
    kOpOffset,          // low 24 bits is signed offset value, TOS is number to add it to
    kOpArrayOffset,     // low 24 bits is array element size, TOS is array base, NTOS is index
    kOpAllocLocals,     // low 24 bits is frame size in cells
    kOpLocalRef,        // low 24 bits is offset in longs
    kOpLocalStringInit,     // bits 0:11 are string length in bytes, bits 12:23 are frame offset in longs
    kOpLocalStructArray,   // bits 0:11 are padded struct size in bytes, bits 12:23 are frame offset in longs
    kOpOffsetFetch,          // low 24 bits is signed offset in longs, TOS is int32_t ptr
    kOpMemberRef,		// low 24 bits is offset in bytes

    kOpLocalByte = 30,	// low 24 bits is offset in bytes
    kOpLocalUByte,
    kOpLocalShort,
    kOpLocalUShort,
    kOpLocalInt,
    kOpLocalUInt,
    kOpLocalLong,
    kOpLocalULong,
    kOpLocalFloat,
    kOpLocalDouble,
    kOpLocalString,
    kOpLocalOp,
    kOpLocalObject,

    kOpLocalByteArray = 43,	// low 24 bits is offset in bytes, TOS is index
    kOpLocalUByteArray,
    kOpLocalShortArray,
    kOpLocalUShortArray,
    kOpLocalIntArray,
    kOpLocalUIntArray,
    kOpLocalLongArray,
    kOpLocalULongArray,
    kOpLocalFloatArray,
    kOpLocalDoubleArray,
    kOpLocalStringArray,
    kOpLocalOpArray,
    kOpLocalObjectArray,

    kOpFieldByte = 56,
    kOpFieldUByte,
    kOpFieldShort,
    kOpFieldUShort,
    kOpFieldInt,
    kOpFieldUInt,
    kOpFieldLong,
    kOpFieldULong,
    kOpFieldFloat,
    kOpFieldDouble,
    kOpFieldString,
    kOpFieldOp,
    kOpFieldObject,

    kOpFieldByteArray = 69,
    kOpFieldUByteArray,
    kOpFieldShortArray,
    kOpFieldUShortArray,
    kOpFieldIntArray,
    kOpFieldUIntArray,
    kOpFieldLongArray,
    kOpFieldULongArray,
    kOpFieldFloatArray,
    kOpFieldDoubleArray,
    kOpFieldStringArray,
    kOpFieldOpArray,
    kOpFieldObjectArray,

    kOpMemberByte = 82,
    kOpMemberUByte,
    kOpMemberShort,
    kOpMemberUShort,
    kOpMemberInt,
    kOpMemberUInt,
    kOpMemberLong,
    kOpMemberULong,
    kOpMemberFloat,
    kOpMemberDouble,
    kOpMemberString,
    kOpMemberOp,
    kOpMemberObject,

    kOpMemberByteArray = 95,
    kOpMemberUByteArray,
    kOpMemberShortArray,
    kOpMemberUShortArray,
    kOpMemberIntArray,
    kOpMemberUIntArray,
    kOpMemberLongArray,
    kOpMemberULongArray,
    kOpMemberFloatArray,
    kOpMemberDoubleArray,
    kOpMemberStringArray,
    kOpMemberOpArray,
    kOpMemberObjectArray,

    kOpMethodWithThis = 108,                // low 24 bits is method number
    kOpMethodWithTOS,                       // low 24 bits is method number
    kOpMemberStringInit,                    // bits 0:11 are string length in bytes, bits 12:23 are memeber offset in longs
	kOpNVOCombo,							// NUM VAROP OP combo - bits 0:10 are signed integer, bits 11:12 are varop-2, bit 13 is builtin/userdef, bits 14-23 are opcode
	kOpNVCombo,								// NUM VAROP combo - bits 0:21 are signed integer, bits 22:23 are varop-2
	kOpNOCombo,								// NUM OP combo - bits 0:12 are signed integer, bit 13 is builtin/userdef, bits 14:23 are opcode
	kOpVOCombo,								// VAROP OP combo - bits 0:1 are varop-2, bit 2 is builtin/userdef, bits 3:23 are opcode
	kOpOZBCombo,							// OP ZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
	kOpONZBCombo,							// OP NZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs

	kOpSquishedFloat,						// low 24 bits is float as sign bit, 5 exponent bits, 18 mantissa bits
	kOpSquishedDouble,						// low 24 bits is double as sign bit, 5 exponent bits, 18 mantissa bits
	kOpSquishedLong,						// low 24 bits is value to be sign extended to 64-bits

	kOpLocalRefOpCombo = 120,				// LOCAL_REF OP - bits 0:11 are local var offset in longs, bits 12:23 are opcode
	kOpMemberRefOpCombo,					// MEMBER_REF OP - bits 0:11 are local var offset in longs, bits 12:23 are opcode

    kOpMethodWithSuper,                     // low 24 bits is method number

    // 123 - 127
    kOpNativeU32,                           // low 24 bits is native op number, next longword is 32-bit unsigned int immediate value
    kOpNativeS32,                           // low 24 bits is native op number, next longword is 32-bit signed int immediate value
    kOpNativeF32,                           // low 24 bits is native op number, next longword is 32-bit float immediate value
    kOpNativeS64,                           // low 24 bits is native op number, next 2 longwords is 64-bit signed int immediate value
    kOpNativeF64,                           // low 24 bits is native op number, next 2 longwords is 64-bit float immediate value

    // 128 - 132
    kOpCCodeU32,                            // low 24 bits is CCode op number, next longword is 32-bit unsigned int immediate value
    kOpCCodeS32,                            // low 24 bits is CCode op number, next longword is 32-bit signed intimmediate value
    kOpCCodeF32,                            // low 24 bits is CCode op number, next longword is 32-bit float immediate value
    kOpCCodeS64,                            // low 24 bits is CCode op number, next 2 longwords is 64-bit signed int immediate value
    kOpCCodeF64,                            // low 24 bits is CCode op number, next 2 longwords is 64-bit float immediate value

    // 133 - 137
    kOpUserDefU32,                          // low 24 bits is UserDef op number, next longword is 32-bit unsigned int immediate value
    kOpUserDefS32,                          // low 24 bits is UserDef op number, next longword is 32-bit signed intimmediate value
    kOpUserDefF32,                          // low 24 bits is UserDef op number, next longword is 32-bit float immediate value
    kOpUserDefS64,                          // low 24 bits is UserDef op number, next 2 longwords is 64-bit signed int immediate value
    kOpUserDefF64,                          // low 24 bits is UserDef op number, next 2 longwords is 64-bit float immediate value

    kOpMethodWithLocalObject,
    kOpMethodWithMemberObject,
    
    kOpLastBaseDefined = kOpMethodWithMemberObject,
    kOpLocalUserDefined = 192,              // user can add more optypes starting with this one
    kOpMaxLocalUserDefined = 255,           // maximum user defined optype

#if defined(FORTH64)
    kOpLocalCell = kOpLocalLong,
    kOpLocalUCell = kOpLocalULong,
    kOpLocalCellArray = kOpLocalLongArray,
    kOpMemberCell = kOpMemberLong,
    kOpMemberCellArray = kOpMemberLongArray,
    kOpFieldCell = kOpFieldLong,
    kOpFieldCellArray = kOpFieldLongArray,
#else
    kOpLocalCell = kOpLocalInt,
    kOpLocalUCell = kOpLocalUInt,
    kOpLocalCellArray = kOpLocalIntArray,
    kOpMemberCell = kOpMemberInt,
    kOpMemberCellArray = kOpMemberIntArray,
    kOpFieldCell = kOpFieldInt,
    kOpFieldCellArray = kOpFieldIntArray,
#endif

    kOpUserMethods  = 128
    // optypes from 128:.255 are used to select class methods    
} forthOpType;

#ifdef ASM_INNER_INTERPRETER
#define NATIVE_OPTYPE kOpNative
#else
#define NATIVE_OPTYPE kOpCCode
#endif

// there is an action routine with this signature for each forthOpType
// user can add new optypes with Engine::AddOpType
typedef void (*optypeActionRoutine)( CoreState *pCore, forthop theData );

typedef void  (*ForthCOp)( CoreState * );

// user will also have to add an external interpreter with Engine::SetInterpreterExtension
// to compile/interpret these new optypes
// return true if the extension has recognized and processed the symbol
typedef bool (*interpreterExtensionRoutine)( char *pToken );

// traceOutRoutine is used when overriding builtin trace routines
typedef void (*traceOutRoutine) ( void *pData, const char* pFormat, va_list argList );

// the varMode state makes variables do something other
//  than their default behaviour (fetch)
enum class VarOperation:ucell {
    kVarDefaultOp = 0,
    kVarGet,
    kVarRef,
    kVarSet,

    kVarSetPlus,
    kVarSetMinus,
    kVarClear,
    kVarPlus,

    kVarInc,
    kVarMinus,
    kVarDec,
    kVarIncGet,
    
    kVarDecGet,
    kVarGetInc,
    kVarGetDec,
    kVarUnused1,

    kPtrAtGet,
    kPtrAtSet,
    kPtrAtSetPlus,
    kPtrAtSetMinus,

    kPtrAtGetInc,
    kPtrAtGetDec,
    kPtrAtSetInc,
    kPtrAtSetDec,

    kPtrIncAtGet,
    kPtrDecAtGet,
    kPtrIncAtSet,
    kPtrDecAtSet,

    kNumVarops,

    kVarUnref = kVarSetMinus,

    kNumBasicVarops = kVarGetDec + 1,
    kUnchecked = kNumVarops
};

#define DEFAULT_INPUT_BUFFER_LEN   (16 * 1024)

// these are the results of running the inner interpreter
enum class OpResult:ucell
{
    kOk,          // no need to exit
    kDone,        // exit because of "done" opcode
    kExitShell,   // exit because of a "bye" opcode
    kError,       // exit because of error
    kFatalError,  // exit because of fatal error
    kException,   // exit because of uncaught exception
    kShutdown,    // exit because of a "shutdown" opcode
	kYield,		// exit because of a stopThread/yield/sleepThread opcode
};

// run state of ForthFibers
enum class FiberState:ucell
{
	kStopped,		// initial state, or after executing stop, needs another thread to Start it
	kReady,			// ready to continue running
	kSleeping,		// sleeping until wakeup time is reached
	kBlocked,		// blocked on a soft lock
	kExited,		// done running - executed exitThread
};

enum class ForthError:ucell
{
	kNone,
	kBadOpcode,
    kBadOpcodeType,
    kBadParameter,
    kBadVarOperation,
    kParamStackUnderflow,
    kParamStackOverflow,
    kReturnStackUnderflow,
    kReturnStackOverflow,
    kUnknownSymbol,
    kFileOpen,
    kAbort,
    kForgetBuiltin,
    kBadMethod,
    kException,
    kMissingSize,
    kStruct,
    kUserDefined,
    kBadSyntax,
    kBadPreprocessorDirective,
    kUnimplementedMethod,
    kIllegalMethod,
    kShellStackUnderflow,
    kShellStackOverflow,
	kBadReferenceCount,
	kIO,
	kBadObject,
    kStringOverflow,
	kBadArrayIndex,
    kIllegalOperation,
    kOSException,
	// NOTE: if you add errors, make sure that you update Engine::GetErrorString
    kForthNumErrors
};

enum class ExceptionState:ucell
{
    kTry,
    kExcept,
    kFinally,
    kForthNumExceptionStates
};

// exception handler IP offsets (compiled just after _doTry opcode)
//  these are offsets from pHandlerOffsets
// 0    exceptIPOffset
// 1    finallyIPOffset

// exception frame on rstack:
struct ForthExceptionFrame
{
    ForthExceptionFrame*    pNextFrame;
    cell*                   pSavedSP;
    forthop*                pHandlerOffsets;
    cell*                   pSavedFP;
    cell                    exceptionNumber;
    ExceptionState          exceptionState;
};

// how sign should be handled while printing integers
typedef enum {
    kPrintSignedDecimal,
    kPrintAllSigned,
    kPrintAllUnsigned
} ePrintSignedMode;

typedef enum {
    kFFParenIsComment           = 0x001,
    kFFCCharacterLiterals       = 0x002,
    kFFMultiCharacterLiterals   = 0x004,
    kFFCStringLiterals          = 0x008,
    kFFCHexLiterals             = 0x010,
    kFFDoubleSlashComment       = 0x020,
    kFFIgnoreCase               = 0x040,
    kFFDollarHexLiterals        = 0x080,
    kFFCFloatLiterals           = 0x100,
    kFFParenIsExpression        = 0x200,
    kFFAllowContinuations       = 0x400,
    kFFAllowVaropSuffix         = 0x800
} ForthFeatureFlags;


typedef struct {
    // user dictionary stuff
    forthop*            pCurrent;
    forthop*            pBase;
    ucell               len;
} MemorySection;

#if defined(ATOMIC_REFCOUNTS)
#define REFCOUNTER      std::atomic<ucell>
#else
#define REFCOUNTER      ucell
#endif

// this is the beginning of each Forth object
// this points to a number of longwords, the first longword is the methods pointer,
// the second longword is the reference count, followed by class dependant data
struct oObjectStruct
{
    forthop*            pMethods;
    REFCOUNTER          refCount;
};

typedef oObjectStruct* ForthObject;

// oInterfaceStruct is a wrapper around a Forth object, which is used to implement
// all the non-primary interfaces an object supports.
// When you do getInterface on an object, it creates and returns an Interface.
// The first instruction of all non-primary interface methods is _devo, which takes
// the object pointer in pWrappedObject and overwrites the current this pointer.
struct oInterfaceObjectStruct
{
    forthop* pMethods;
    REFCOUNTER refCount;
    ForthObject pWrappedObject;
};

typedef oInterfaceObjectStruct* InterfaceObject;

// this godawful mess is here because the ANSI Forth standard defines that the top item
// on the parameter stack for 64-bit ints is the highword, which is opposite to the c++/c
// standard (at least for x86 architectures).
typedef union
{
    int32_t     s32[2];
    uint32_t    u32[2];
    int64_t     s64;
    uint64_t    u64;
} stackInt64;


// stream character output routine type
typedef void (*streamCharOutRoutine) ( CoreState* pCore, void *pData, char ch );

// stream block output routine type
typedef void (*streamBytesOutRoutine) ( CoreState* pCore, void *pData, const char *pBuff, int numChars );

// stream string output routine type
typedef void (*streamStringOutRoutine) ( CoreState* pCore, void *pData, const char *pBuff );

// stream character input routine type - returns 1 for char gotten, 0 for EOF
typedef int (*streamCharInRoutine) (CoreState* pCore, void *pData, int& ch);

// stream block input routine type - returns number of chars gotten
typedef int (*streamBytesInRoutine) (CoreState* pCore, void *pData, char *pBuff, int numChars);

// stream string input routine type - returns number of chars gotten
typedef int(*streamStringInRoutine) (CoreState* pCore, void *pData, ForthObject& dstString);

// stream line input routine type - returns number of chars gotten
typedef int(*streamLineInRoutine) (CoreState* pCore, void *pData, char *pBuff, int maxChars);

// these routines allow code external to forth to redirect the forth output stream
extern void GetForthConsoleOutStream( CoreState* pCore, ForthObject& outObject );
extern void CreateForthFileOutStream( CoreState* pCore, ForthObject& outObject, FILE* pOutFile );
extern void CreateForthFunctionOutStream( CoreState* pCore, ForthObject& outObject, streamCharOutRoutine outChar,
											  streamBytesOutRoutine outBlock, streamStringOutRoutine outString, void* pUserData );
extern void GetForthErrorOutStream(CoreState* pCore, ForthObject& outObject);

extern void ForthConsoleCharOut( CoreState* pCore, char ch );
extern void ForthConsoleBytesOut( CoreState* pCore, const char* pBuffer, int numChars );
extern void ForthConsoleStringOut(CoreState* pCore, const char* pBuffer);
extern void ForthErrorStringOut(CoreState* pCore, const char* pBuffer);

// the bottom 24 bits of a forth opcode is a value field
// the top 8 bits is the type field
#define OPCODE_VALUE_MASK   0xFFFFFF
#define FORTH_OP_TYPE( OP )  ( (forthOpType) (((OP) >> 24) & 0xFF) )
#define FORTH_OP_VALUE( OP ) ( (OP) & OPCODE_VALUE_MASK )

#define DISPATCH_FORTH_OP( _pCore, _op ) 	_pCore->optypeAction[ (int) FORTH_OP_TYPE( _op ) ]( _pCore, FORTH_OP_VALUE( _op ) )


#define NEEDS(A)
#define RNEEDS(A)

class ForthThread;
class ForthFiber;

#define COMPILED_OP( OP_TYPE, VALUE ) ((forthop)(((OP_TYPE) << 24) | ((VALUE) & OPCODE_VALUE_MASK)))
// These are opcodes that built-in ops must compile directly
// NOTE: the index field of these opcodes must agree with the
//  order of builtin dictionary entries in the ForthOps.cpp file
enum {
	OP_ABORT = 0,
	OP_DROP,
	OP_DO_DOES,
	OP_INT_VAL,
    OP_UINT_VAL,
    OP_FLOAT_VAL,
	OP_DOUBLE_VAL,
	OP_LONG_VAL,

	OP_DO_VAR,
	OP_DO_CONSTANT,
	OP_DO_DCONSTANT,
	OP_DONE,
	OP_DO_BYTE,
	OP_DO_UBYTE,
	OP_DO_SHORT,
	OP_DO_USHORT,

    OP_DO_INT,			// 0x10
	OP_DO_UINT,
	OP_DO_LONG,
	OP_DO_ULONG,
	OP_DO_FLOAT,
	OP_DO_DOUBLE,
	OP_DO_STRING,
	OP_DO_OP,

	OP_DO_OBJECT,
	OP_DO_EXIT,
    OP_DO_EXIT_L,
	OP_DO_EXIT_M,
	OP_DO_EXIT_ML,
	OP_DO_BYTE_ARRAY,
	OP_DO_UBYTE_ARRAY,
	OP_DO_SHORT_ARRAY,

    OP_DO_USHORT_ARRAY,	// 0x20
	OP_DO_INT_ARRAY,
    OP_DO_UINT_ARRAY,
	OP_DO_LONG_ARRAY,
	OP_DO_ULONG_ARRAY,
	OP_DO_FLOAT_ARRAY,
	OP_DO_DOUBLE_ARRAY,
	OP_DO_STRING_ARRAY,

	OP_DO_OP_ARRAY,
	OP_DO_OBJECT_ARRAY,
    OP_INIT_STRING,
	OP_PLUS,
	OP_IFETCH,
	OP_DO_STRUCT,
	OP_DO_STRUCT_ARRAY,
	OP_DO_DO,

	OP_DO_LOOP,		// 0x30
	OP_DO_LOOPN,
	OP_FETCH,
	OP_REF,
	OP_INTO,
	OP_INTO_PLUS,
	OP_INTO_MINUS,
	OP_OCLEAR,

    OP_SETVAROP,
    OP_DO_CHECKDO,
	OP_DO_VOCAB,
	OP_GET_CLASS_BY_INDEX,
	OP_INIT_STRING_ARRAY,
	OP_BAD_OP,
	OP_DO_STRUCT_TYPE,
	OP_DO_CLASS_TYPE,

	OP_DO_ENUM,              // 0x40
    OP_DO_NEW,
	OP_ALLOC_OBJECT,
	OP_END_BUILDS,
    OP_COMPILE,
	OP_INIT_STRUCT_ARRAY,
	OP_DUP,
	OP_OVER,

    OP_DO_TRY,
    OP_DO_FINALLY,
    OP_DO_ENDTRY,
    OP_RAISE,
    OP_RDROP,
    OP_NOOP,
    OP_DEVOLVE,
    OP_UNIMPLEMENTED,
    OP_EXECUTE_METHOD,
    OP_THIS,

	NUM_COMPILED_OPS,

#ifdef FORTH64
    OP_DO_CELL = OP_DO_LONG,
    OP_DO_CELL_ARRAY = OP_DO_LONG_ARRAY,
#else
    OP_DO_CELL = OP_DO_INT,
    OP_DO_CELL_ARRAY = OP_DO_INT_ARRAY,
#endif

};

extern forthop gCompiledOps[];

typedef struct
{
   const char*      name;
   void*            value;
   uint32_t         flags;
} baseDictionaryEntry;

#if defined(ASM_INNER_INTERPRETER)
#define NATIVE_OPTYPE kOpNative
#else
#define NATIVE_OPTYPE kOpCCode
#endif

// helper macro for built-in op entries in baseDictionary
#define OP_DEF( func, funcName )  { funcName, (void *)func, kOpCCode }
// helper macro for ops which have precedence (execute at compile time)
#define PRECOP_DEF( func, funcName )  { funcName, (void *)func, kOpCCodeImmediate }
// helper macro for built-in op entries in baseDictionary which are defined in assembler
#define NATIVE_DEF( func, funcName )  { funcName, (void *)func, NATIVE_OPTYPE }

typedef struct {
    const char*     name;
    void*           value;
    uint32_t        flags;
    uint32_t        index;
} baseDictionaryCompiledEntry;

// these are ops which may be compiled by forth itself
#define OP_COMPILED_DEF( func, funcName, index )  { funcName, (void *)func, kOpCCode, index }
#define PRECOP_COMPILED_DEF( func, funcName, index )  { funcName, (void *)func, kOpCCodeImmediate, index }
#define NATIVE_COMPILED_DEF( func, funcName, index ) { funcName, (void *)func, NATIVE_OPTYPE, index }




typedef struct
{
    const char*     name;
    void*           value;
    uint32_t        returnType;
} baseMethodEntry;

#define OP_DEF_RETURNS( func, funcName, retType )  { funcName, (void *)func, retType }

// trace output flags
#ifdef INCLUDE_TRACE
//#define TRACE_PRINTS
#define TRACE_OUTER_INTERPRETER
#define TRACE_INNER_INTERPRETER
#define TRACE_SHELL
#define TRACE_VOCABULARY
#define TRACE_STRUCTS
#define TRACE_ENGINE
//#define TRACE_COMPILATION
#endif

enum
{
	kLogStack					= 1,
	kLogOuterInterpreter		= 2,
	kLogInnerInterpreter		= 4,
	kLogShell					= 8,
	kLogStructs					= 16,
	kLogVocabulary				= 32,
	kLogIO						= 64,
	kLogEngine					= 128,
    kLogToFile                  = 256,
	kLogToConsole				= 512,
	kLogCompilation				= 1024,
	kLogProfiler				= 2048
};

#ifdef TRACE_PRINTS
#define SPEW_PRINTS TRACE
#else
#define SPEW_PRINTS TRACE
//#define SPEW_PRINTS(...)
#endif

#ifdef TRACE_OUTER_INTERPRETER
#if defined(WINDOWS_BUILD)
#define SPEW_OUTER_INTERPRETER(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogOuterInterpreter) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_OUTER_INTERPRETER(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogOuterInterpreter) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_OUTER_INTERPRETER(...)
#endif

#ifdef TRACE_INNER_INTERPRETER
#if defined(WINDOWS_BUILD)
#define SPEW_INNER_INTERPRETER(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogInnerInterpreter) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_INNER_INTERPRETER(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogInnerInterpreter) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_INNER_INTERPRETER(...)
#endif

#ifdef TRACE_SHELL
#if defined(WINDOWS_BUILD)
#define SPEW_SHELL(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogShell) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_SHELL(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogShell) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_SHELL(...)
#endif

#ifdef TRACE_VOCABULARY
#if defined(WINDOWS_BUILD)
#define SPEW_VOCABULARY(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogVocabulary) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_VOCABULARY(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogVocabulary) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_VOCABULARY(...)
#endif

#ifdef TRACE_STRUCTS
#if defined(WINDOWS_BUILD)
#define SPEW_STRUCTS(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogStructs) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_STRUCTS(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogStructs) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_STRUCTS(...)
#endif

#ifdef TRACE_IO
#if defined(WINDOWS_BUILD)
#define SPEW_IO(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogIO) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_IO(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogIO) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_IO(...)
#endif

#ifdef TRACE_ENGINE
#if defined(WINDOWS_BUILD)
#define SPEW_ENGINE(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogEngine) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_ENGINE(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogEngine) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_ENGINE(...)
#endif

#ifdef TRACE_COMPILATION
#if defined(WINDOWS_BUILD)
#define SPEW_COMPILATION(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogCompilation) { Engine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_COMPILATION(FORMAT, ...)  if (Engine::GetInstance()->GetTraceFlags() & kLogCompilation) { Engine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_COMPILATION(...)
#endif

// user-defined ops vocab entries have the following value fields:
// - opcode
// - struct type
#define NUM_FORTH_VOCAB_VALUE_LONGS 2

// a locals vocab entry has the following value fields:
// - opcode (which contains frame offset)
// - struct type
#define NUM_LOCALS_VOCAB_VALUE_LONGS 2

// a struct vocab entry has the following value fields:
// - field offset in bytes
// - field type
// - element count (valid only for array fields)
#define NUM_STRUCT_VOCAB_VALUE_LONGS 3

//////////////////////////////////////////////////////////////////////
////
///     built-in forth ops are implemented with static C-style routines
//      which take a pointer to the ForthFiber they are being run in
//      the thread is accessed through "pCore->" in the code

#define FORTHOP(NAME) void NAME( CoreState *pCore )
// GFORTHOP is used for forthops which are defined outside of the dictionary source module
#define GFORTHOP(NAME) void NAME( CoreState *pCore )

//////////////////////////////////////////////////////////////////////
////
///     user-defined structure support
//      
//      

#if defined(FORTH64)
#define CELL_SHIFT 3
#define CELL_BYTES 8
#define CELL_MASK 7
#define CELL_LONGS 2
#define CELL_BITS 64
#else
#define CELL_SHIFT 2
#define CELL_BYTES 4
#define CELL_MASK 3
#define CELL_LONGS 1
#define CELL_BITS 32
#endif
//#define CELL_BYTES (1 << CELL_SHIFT)
//#define CELL_BITS (CELL_BYTES << 3)
//#define CELL_MASK (CELL_BYTES - 1)
//#define CELL_LONGS (1 << (CELL_SHIFT - 2))

#define BYTES_TO_CELLS(NBYTES)     ((NBYTES + CELL_MASK) & ~CELL_MASK) >> CELL_SHIFT;

// forth native data types
// NOTE: the order of these have to match the order of forthOpType definitions above which
//  are related to native types (kOpLocalByte, kOpMemberFloat, kOpLocalIntArray, ...)
//  as well as the order of actual opcodes used to implement native types (, OP_DO_FLOAT, OP_DO_INT_ARRAY, ...)
enum class BaseType:ucell
{
    kByte,              // 0 - byte
    kUByte,             // 1 - ubyte
    kShort,             // 2 - short
    kUShort,            // 3 - ushort
    kInt,               // 4 - int
    kUInt,              // 5 - uint
    kLong,              // 6 - long
    kULong,             // 7 - ulong
    kFloat,             // 8 - float
    kDouble,            // 9 - double
    kString,            // 10 - string
    kOp,                // 11 - op
    kObject,            // 12 - object
    kStruct,            // 13 - struct
    kUserDefinition,    // 14 - user defined forthop
    kVoid,				// 15 - void
    kUnknown,
    kNumBaseTypes = kUnknown,
    kNumNativeTypes = kObject,
#ifdef FORTH64
    kCell = kLong,
    kUCell = kULong,
#else
    kCell = kInt,
    kUCell = kUInt,
#endif
};

// kDTIsPtr can be combined with anything
#define kDTIsPtr        16
#define kDTIsArray      32
#define kDTIsMethod     64
#define kDTIsFunky      128
// kDTIsFunky meaning depends on context

// this is the bottom 6-bits, baseType + ptr and array flags
#define STORAGE_DESCRIPTOR_TYPE_MASK (((cell)BaseType::kNumBaseTypes - 1) | kDTIsPtr | kDTIsArray)

// user-defined structure fields have a 32-bit descriptor with the following format:
// 3...0        base type
//   4          is field a pointer
//   5          is field an array
//   6          is this a method
//   7          unused (except for class precedence ops)
// 31...8       depends on base type:
//      string      length
//      struct      typeIndex
//      object      classId

// when kDTArray and kDTIsPtr are both set, it means the field is an array of pointers
#define NATIVE_TYPE_TO_CODE( ARRAY_FLAG, NATIVE_TYPE )      ((ARRAY_FLAG) | (ucell)(NATIVE_TYPE))
#define STRING_TYPE_TO_CODE( ARRAY_FLAG, MAX_BYTES )        ((ARRAY_FLAG) | (ucell)BaseType::kString | ((MAX_BYTES) << 8))
#define STRUCT_TYPE_TO_CODE( ARRAY_FLAG, STRUCT_INDEX )     ((ARRAY_FLAG) | (ucell)BaseType::kStruct | ((STRUCT_INDEX) << 8))
#define OBJECT_TYPE_TO_CODE( ARRAY_FLAG, STRUCT_INDEX )     ((ARRAY_FLAG) | (ucell)BaseType::kObject | ((STRUCT_INDEX) << 8))
#define CONTAINED_TYPE_TO_CODE( ARRAY_FLAG, CONTAINER_INDEX, CONTAINED_INDEX )  \
   ((ARRAY_FLAG) | BaseType::kObject | (((CONTAINED_INDEX) | ((CONTAINER_INDEX) << 16)) << 8))
#define RETURNS_NATIVE(NATIVE_TYPE)                         NATIVE_TYPE_TO_CODE(kDTIsMethod, (NATIVE_TYPE))
#define RETURNS_NATIVE_PTR(NATIVE_TYPE)                     NATIVE_TYPE_TO_CODE(kDTIsMethod, ((ucell)NATIVE_TYPE) | kDTIsPtr)
#define RETURNS_OBJECT(OBJECT_TYPE)                         OBJECT_TYPE_TO_CODE(kDTIsMethod, (OBJECT_TYPE))

#define VOCABENTRY_TO_FIELD_OFFSET( PTR_TO_ENTRY )          (*(PTR_TO_ENTRY))
#define VOCABENTRY_TO_TYPECODE( PTR_TO_ENTRY )              ((PTR_TO_ENTRY)[1])
#define VOCABENTRY_TO_ELEMENT_SIZE( PTR_TO_ENTRY )          ((PTR_TO_ENTRY)[2])

#define BASE_TYPE_TO_CODE( BASE_TYPE )                      (BASE_TYPE)

#define CODE_IS_SIMPLE( CODE )                              (((CODE) & (kDTIsArray | kDTIsPtr)) == 0)
#define CODE_IS_ARRAY( CODE )                               (((CODE) & kDTIsArray) != 0)
#define CODE_IS_PTR( CODE )                                 (((CODE) & kDTIsPtr) != 0)
#define CODE_IS_NATIVE( CODE )                              ((BaseType)((CODE) & 0xF) < BaseType::kNumNativeTypes)
#define CODE_IS_METHOD( CODE )                              (((CODE) & kDTIsMethod) != 0)
#define CODE_IS_FUNKY( CODE )                               (((CODE) & kDTIsFunky) != 0)
#define CODE_TO_BASE_TYPE( CODE )                           (BaseType)((CODE) & 0x0F)
#define CODE_TO_STRUCT_INDEX( CODE )                        ((CODE) >> 8)
#define CODE_TO_CONTAINED_CLASS_INDEX( CODE )               (((CODE) >> 8) & 0xFFFF)
#define CODE_TO_CONTAINER_CLASS_INDEX( CODE )               (((CODE) >> 24) & 0xFF)
#define CODE_TO_STRING_BYTES( CODE )                        ((CODE) >> 8)
#define CODE_IS_USER_DEFINITION( CODE )                     ((BaseType)((CODE) & 0x0F) == BaseType::kUserDefinition)

// bit fields for kOpDLLEntryPoint
#define DLL_ENTRY_TO_CODE( INDEX, NUM_ARGS, FLAGS )    (((NUM_ARGS) << 19) | FLAGS | INDEX )
#define CODE_TO_DLL_ENTRY_INDEX( VAL )          (VAL & 0x0000FFFF)
#define CODE_TO_DLL_ENTRY_NUM_ARGS( VAL)        (((VAL) & 0x00F80000) >> 19)
#define CODE_TO_DLL_ENTRY_FLAGS( VAL)        (((VAL) & 0x00070000) >> 16)
#define DLL_ENTRY_FLAG_RETURN_VOID		0x10000
#define DLL_ENTRY_FLAG_RETURN_64BIT		0x20000
#define DLL_ENTRY_FLAG_STDCALL			0x40000

