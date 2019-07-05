#pragma once
//////////////////////////////////////////////////////////////////////
//
// Forth engine definitions
//   Pat McElhatton   September '00
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"

#include "ForthMemoryManager.h"

struct ForthCoreState;

typedef unsigned int uint32;
typedef int int32;
typedef unsigned long long uint64;
typedef long long int64;

#ifndef ulong
#define ulong   unsigned long
#endif

#define MAX_STRING_SIZE (8 * 1024)

// these are opcode types, they are held in the top byte of an opcode, and in
// a vocabulary entry value field
// NOTE: if you add or reorder op types, make sure that you update ForthEngine::opTypeNames
typedef enum
{
    kOpNative = 0,
    kOpNativeImmediate,
    kOpUserDef,         // low 24 bits is op number (index into ForthCoreState userOps table)
    kOpUserDefImmediate,
    kOpCCode,         // low 24 bits is op number (index into ForthCoreState userOps table)
    kOpCCodeImmediate,
    kOpRelativeDef,         // low 24 bits is offset from dictionary base
    kOpRelativeDefImmediate,
    kOpDLLEntryPoint,   // bits 0:18 are index into ForthCoreState userOps table, 19:23 are arg count
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
    kOpAllocLocals,     // low 24 bits is frame size in longs
    kOpLocalRef,        // low 24 bits is offset in longs
    kOpLocalStringInit,     // bits 0:11 are string length in bytes, bits 12:23 are frame offset in longs
    kOpLocalStructArray,   // bits 0:11 are padded struct size in bytes, bits 12:23 are frame offset in longs
    kOpOffsetFetch,          // low 24 bits is signed offset in longs, TOS is long ptr
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

    kOpLocalUserDefined = 123,             // user can add more optypes starting with this one
    kOpMaxLocalUserDefined = 127,    // maximum user defined optype

    kOpUserMethods  = 128
    // optypes from 128:.255 are used to select class methods    
} forthOpType;

// there is an action routine with this signature for each forthOpType
// user can add new optypes with ForthEngine::AddOpType
typedef void (*optypeActionRoutine)( ForthCoreState *pCore, ulong theData );

typedef void  (*ForthOp)( ForthCoreState * );

// user will also have to add an external interpreter with ForthEngine::SetInterpreterExtension
// to compile/interpret these new optypes
// return true if the extension has recognized and processed the symbol
typedef bool (*interpreterExtensionRoutine)( char *pToken );

// traceOutRoutine is used when overriding builtin trace routines
typedef void (*traceOutRoutine) ( void *pData, const char* pFormat, va_list argList );

// the varMode state makes variables do something other
//  than their default behaviour (fetch)
typedef enum {
    kVarDefaultOp = 0,
    kVarFetch,
    kVarRef,
    kVarStore,
    kVarPlusStore,
    kVarMinusStore,
	kVarObjectClear,
    kNumVarops
} varOperation;

#define DEFAULT_INPUT_BUFFER_LEN   (16 * 1024)

// these are the results of running the inner interpreter
typedef enum {
    kResultOk,          // no need to exit
    kResultDone,        // exit because of "done" opcode
    kResultExitShell,   // exit because of a "bye" opcode
    kResultError,       // exit because of error
    kResultFatalError,  // exit because of fatal error
    kResultException,   // exit because of uncaught exception
    kResultShutdown,    // exit because of a "shutdown" opcode
	kResultYield,		// exit because of a stopThread/yield/sleepThread opcode
} eForthResult;

// run state of ForthThreads
typedef enum
{
	kFTRSStopped,		// initial state, or after executing stop, needs another thread to Start it
	kFTRSReady,			// ready to continue running
	kFTRSSleeping,		// sleeping until wakeup time is reached
	kFTRSBlocked,		// blocked on a soft lock
	kFTRSExited,		// done running - executed exitThread
} eForthThreadRunState;

typedef enum {
	kForthErrorNone,
	kForthErrorBadOpcode,
    kForthErrorBadOpcodeType,
    kForthErrorBadParameter,
    kForthErrorBadVarOperation,
    kForthErrorParamStackUnderflow,
    kForthErrorParamStackOverflow,
    kForthErrorReturnStackUnderflow,
    kForthErrorReturnStackOverflow,
    kForthErrorUnknownSymbol,
    kForthErrorFileOpen,
    kForthErrorAbort,
    kForthErrorForgetBuiltin,
    kForthErrorBadMethod,
    kForthErrorException,
    kForthErrorMissingSize,
    kForthErrorStruct,
    kForthErrorUserDefined,
    kForthErrorBadSyntax,
    kForthErrorBadPreprocessorDirective,
    kForthErrorUnimplementedMethod,
    kForthErrorIllegalMethod,
    kForthErrorShellStackUnderflow,
    kForthErrorShellStackOverflow,
	kForthErrorBadReferenceCount,
	kForthErrorIO,
	kForthErrorBadObject,
    kForthErrorStringOverflow,
	kForthErrorBadArrayIndex,
    kForthErrorIllegalOperation,
    kForthOSException,
	// NOTE: if you add errors, make sure that you update ForthEngine::GetErrorString
    kForthNumErrors
} eForthError;

typedef enum
{
    kForthExceptionStateTry,
    kForthExceptionStateExcept,
    kForthExceptionStateFinally,
    kForthNumExceptionStates
} eForthExceptionState;

// exception handler IPs (compiled just after _doTry opcode)
// 0    exceptIP
// 1    finallyIP

// exception frame on rstack:
struct ForthExceptionFrame
{
    ForthExceptionFrame*    pNextFrame;
    long*                   pSavedSP;
    long**                  pHandlerIPs;
    long*                   pSavedFP;
    long                    exceptionNumber;
    eForthExceptionState    exceptionState;
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
} ForthFeatureFlags;


typedef struct {
    // user dictionary stuff
    long*               pCurrent;
    long*               pBase;
    ulong               len;
} ForthMemorySection;

// this is what is placed on the stack to represent a forth object
// this points to a number of longwords, the first longword is the methods pointer,
// the second longword is the reference count, followed by class dependant data
struct oObjectStruct
{
    long*               pMethods;
    ulong               refCount;
};

typedef oObjectStruct* ForthObject;

// this godawful mess is here because the ANSI Forth standard defines that the top item
// on the parameter stack for 64-bit ints is the highword, which is opposite to the c++/c
// standard (at least for x86 architectures).
typedef union
{
    int s32[2];
    unsigned int u32[2];
    long long s64;
    unsigned long long u64;
} stackInt64;


// stream character output routine type
typedef void (*streamCharOutRoutine) ( ForthCoreState* pCore, void *pData, char ch );

// stream block output routine type
typedef void (*streamBytesOutRoutine) ( ForthCoreState* pCore, void *pData, const char *pBuff, int numChars );

// stream string output routine type
typedef void (*streamStringOutRoutine) ( ForthCoreState* pCore, void *pData, const char *pBuff );

// stream character input routine type - returns 1 for char gotten, 0 for EOF
typedef int (*streamCharInRoutine) (ForthCoreState* pCore, void *pData, int& ch);

// stream block input routine type - returns number of chars gotten
typedef int (*streamBytesInRoutine) (ForthCoreState* pCore, void *pData, char *pBuff, int numChars);

// stream string input routine type - returns number of chars gotten
typedef int(*streamStringInRoutine) (ForthCoreState* pCore, void *pData, ForthObject& dstString);

// stream line input routine type - returns number of chars gotten
typedef int(*streamLineInRoutine) (ForthCoreState* pCore, void *pData, char *pBuff, int maxChars);

// these routines allow code external to forth to redirect the forth output stream
extern void GetForthConsoleOutStream( ForthCoreState* pCore, ForthObject& outObject );
extern void CreateForthFileOutStream( ForthCoreState* pCore, ForthObject& outObject, FILE* pOutFile );
extern void CreateForthFunctionOutStream( ForthCoreState* pCore, ForthObject& outObject, streamCharOutRoutine outChar,
											  streamBytesOutRoutine outBlock, streamStringOutRoutine outString, void* pUserData );
extern void ReleaseForthObject( ForthCoreState* pCore, ForthObject& inObject );

extern void ForthConsoleCharOut( ForthCoreState* pCore, char ch );
extern void ForthConsoleBytesOut( ForthCoreState* pCore, const char* pBuffer, int numChars );
extern void ForthConsoleStringOut( ForthCoreState* pCore, const char* pBuffer );

// the bottom 24 bits of a forth opcode is a value field
// the top 8 bits is the type field
#define OPCODE_VALUE_MASK   0xFFFFFF
#define FORTH_OP_TYPE( OP )  ( (forthOpType) (((OP) >> 24) & 0xFF) )
#define FORTH_OP_VALUE( OP ) ( (OP) & OPCODE_VALUE_MASK )

#define DISPATCH_FORTH_OP( _pCore, _op ) 	_pCore->optypeAction[ (int) FORTH_OP_TYPE( _op ) ]( _pCore, FORTH_OP_VALUE( _op ) )


#define NEEDS(A)
#define RNEEDS(A)

class ForthThread;

#define COMPILED_OP( OP_TYPE, VALUE ) (((OP_TYPE) << 24) | ((VALUE) & OPCODE_VALUE_MASK))
// These are opcodes that built-in ops must compile directly
// NOTE: the index field of these opcodes must agree with the
//  order of builtin dictionary entries in the ForthOps.cpp file
enum {
	OP_ABORT = 0,
	OP_DROP,
	OP_DO_DOES,
	OP_INT_VAL,
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
	OP_DO_INT,

	OP_DO_UINT,			// 0x10
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
	OP_DO_USHORT_ARRAY,

	OP_DO_INT_ARRAY,	// 0x20
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
	OP_DO_LOOP,

	OP_DO_LOOPN,		// 0x30
	OP_FETCH,
	OP_REF,
	OP_INTO,
	OP_INTO_PLUS,
	OP_INTO_MINUS,
	OP_OCLEAR,
	OP_DO_CHECKDO,

	OP_DO_VOCAB,
	// below this line are ops defined in C
	OP_GET_CLASS_BY_INDEX,
	OP_INIT_STRING_ARRAY,
	OP_BAD_OP,
	OP_DO_STRUCT_TYPE,
	OP_DO_CLASS_TYPE,
	OP_DO_ENUM,
	OP_DO_NEW,

	OP_ALLOC_OBJECT, 		// 0x40
	OP_SUPER,
	OP_END_BUILDS,
    OP_COMPILE,
	OP_INIT_STRUCT_ARRAY,
	OP_DUP,
	OP_OVER,
    OP_DO_TRY,
    OP_DO_FINALLY,
    OP_DO_ENDTRY,
    OP_RAISE,
    OP_UNSUPER,
    OP_RDROP,

	NUM_COMPILED_OPS
};

extern long gCompiledOps[];

typedef struct
{
   const char       *name;
   ulong            flags;
   ulong            value;
} baseDictionaryEntry;

// helper macro for built-in op entries in baseDictionary
#define OP_DEF( func, funcName )  { funcName, kOpCCode, (ulong) func }

// helper macro for ops which have precedence (execute at compile time)
#define PRECOP_DEF( func, funcName )  { funcName, kOpCCodeImmediate, (ulong) func }


typedef struct
{
    const char      *name;
    ulong           value;
    ulong           returnType;
} baseMethodEntry;


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
	kLogToConsole				= 256,
	kLogCompilation				= 512,
	kLogProfiler				= 1024
};

#ifdef TRACE_PRINTS
#define SPEW_PRINTS TRACE
#else
#define SPEW_PRINTS TRACE
//#define SPEW_PRINTS(...)
#endif

#ifdef TRACE_OUTER_INTERPRETER
#ifdef WIN32
#define SPEW_OUTER_INTERPRETER(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogOuterInterpreter) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_OUTER_INTERPRETER(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogOuterInterpreter) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_OUTER_INTERPRETER(...)
#endif

#ifdef TRACE_INNER_INTERPRETER
#ifdef WIN32
#define SPEW_INNER_INTERPRETER(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogInnerInterpreter) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_INNER_INTERPRETER(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogInnerInterpreter) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_INNER_INTERPRETER(...)
#endif

#ifdef TRACE_SHELL
#ifdef WIN32
#define SPEW_SHELL(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogShell) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_SHELL(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogShell) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_SHELL(...)
#endif

#ifdef TRACE_VOCABULARY
#ifdef WIN32
#define SPEW_VOCABULARY(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogVocabulary) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_VOCABULARY(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogVocabulary) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_VOCABULARY(...)
#endif

#ifdef TRACE_STRUCTS
#ifdef WIN32
#define SPEW_STRUCTS(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogStructs) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_STRUCTS(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogStructs) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_STRUCTS(...)
#endif

#ifdef TRACE_IO
#ifdef WIN32
#define SPEW_IO(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogIO) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_IO(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogIO) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_IO(...)
#endif

#ifdef TRACE_ENGINE
#ifdef WIN32
#define SPEW_ENGINE(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogEngine) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_ENGINE(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogEngine) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
#endif
#else
#define SPEW_ENGINE(...)
#endif

#ifdef TRACE_COMPILATION
#ifdef WIN32
#define SPEW_COMPILATION(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogCompilation) { ForthEngine::GetInstance()->TraceOut(FORMAT, __VA_ARGS__); }
#else
#define SPEW_COMPILATION(FORMAT, ...)  if (ForthEngine::GetInstance()->GetTraceFlags() & kLogCompilation) { ForthEngine::GetInstance()->TraceOut(FORMAT, ##__VA_ARGS__); }
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
//      which take a pointer to the ForthThread they are being run in
//      the thread is accesed through "g->" in the code

#define FORTHOP(NAME) void NAME( ForthCoreState *pCore )
// GFORTHOP is used for forthops which are defined outside of the dictionary source module
#define GFORTHOP(NAME) void NAME( ForthCoreState *pCore )

//////////////////////////////////////////////////////////////////////
////
///     user-defined structure support
//      
//      

// forth native data types
// NOTE: the order of these have to match the order of forthOpType definitions above which
//  are related to native types (kOpLocalByte, kOpMemberFloat, kOpLocalIntArray, ...)
//  as well as the order of actual opcodes used to implement native types (, OP_DO_FLOAT, OP_DO_INT_ARRAY, ...)
typedef enum
{
    kBaseTypeByte,          // 0 - byte
    kBaseTypeUByte,         // 1 - ubyte
    kBaseTypeShort,         // 2 - short
    kBaseTypeUShort,        // 3 - ushort
    kBaseTypeInt,           // 4 - int
    kBaseTypeUInt,          // 5 - uint
    kBaseTypeLong,          // 6 - long
    kBaseTypeULong,         // 7 - ulong
    kBaseTypeFloat,         // 8 - float
    kBaseTypeDouble,        // 9 - double
    kBaseTypeString,        // 10 - string
    kBaseTypeOp,            // 11 - op
    kNumNativeTypes,
    kBaseTypeObject = kNumNativeTypes,      // 12 - object
    kBaseTypeStruct,                        // 13 - struct
    kBaseTypeUserDefinition,                // 14 - user defined forthop
    kBaseTypeVoid,							// 15 - void
    kNumBaseTypes,
    kBaseTypeUnknown = kNumBaseTypes,
} forthBaseType;

typedef enum
{
    // kDTIsPtr and kDTIsNative can be combined with anything
    kDTIsPtr        = 16,
    kDTIsArray      = 32,
    kDTIsMethod     = 64,
    kDTIsFunky      = 128       // use depends on context
} storageDescriptor;

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
#define NATIVE_TYPE_TO_CODE( ARRAY_FLAG, NATIVE_TYPE )      ((ARRAY_FLAG) | (NATIVE_TYPE))
#define STRING_TYPE_TO_CODE( ARRAY_FLAG, MAX_BYTES )        ((ARRAY_FLAG) | kBaseTypeString | ((MAX_BYTES) << 8))
#define STRUCT_TYPE_TO_CODE( ARRAY_FLAG, STRUCT_INDEX )     ((ARRAY_FLAG) | kBaseTypeStruct | ((STRUCT_INDEX) << 8))
#define OBJECT_TYPE_TO_CODE( ARRAY_FLAG, STRUCT_INDEX )     ((ARRAY_FLAG) | kBaseTypeObject | ((STRUCT_INDEX) << 8))
#define CONTAINED_TYPE_TO_CODE( ARRAY_FLAG, CONTAINER_INDEX, CONTAINED_INDEX )  \
   ((ARRAY_FLAG) | kBaseTypeObject | (((CONTAINED_INDEX) | ((CONTAINER_INDEX) << 16)) << 8))
#define RETURNS_NATIVE(NATIVE_TYPE)                         NATIVE_TYPE_TO_CODE(kDTIsMethod, (NATIVE_TYPE))
#define RETURNS_OBJECT(OBJECT_TYPE)                         OBJECT_TYPE_TO_CODE(kDTIsMethod, (OBJECT_TYPE))

#define VOCABENTRY_TO_FIELD_OFFSET( PTR_TO_ENTRY )          (*(PTR_TO_ENTRY))
#define VOCABENTRY_TO_TYPECODE( PTR_TO_ENTRY )              ((PTR_TO_ENTRY)[1])
#define VOCABENTRY_TO_ELEMENT_SIZE( PTR_TO_ENTRY )          ((PTR_TO_ENTRY)[2])

#define BASE_TYPE_TO_CODE( BASE_TYPE )                      (BASE_TYPE)

#define CODE_IS_SIMPLE( CODE )                              (((CODE) & (kDTIsArray | kDTIsPtr)) == 0)
#define CODE_IS_ARRAY( CODE )                               (((CODE) & kDTIsArray) != 0)
#define CODE_IS_PTR( CODE )                                 (((CODE) & kDTIsPtr) != 0)
#define CODE_IS_NATIVE( CODE )                              (((CODE) & 0xF) < kNumNativeTypes)
#define CODE_IS_METHOD( CODE )                              (((CODE) & kDTIsMethod) != 0)
#define CODE_IS_FUNKY( CODE )                               (((CODE) & kDTIsFunky) != 0)
#define CODE_TO_BASE_TYPE( CODE )                           ((CODE) & 0x0F)
#define CODE_TO_STRUCT_INDEX( CODE )                        ((CODE) >> 8)
#define CODE_TO_CONTAINED_CLASS_INDEX( CODE )               (((CODE) >> 8) & 0xFFFF)
#define CODE_TO_CONTAINER_CLASS_INDEX( CODE )               (((CODE) >> 24) & 0xFF)
#define CODE_TO_STRING_BYTES( CODE )                        ((CODE) >> 8)
#define CODE_IS_USER_DEFINITION( CODE )                     (((CODE) & 0x0F) == kBaseTypeUserDefinition)

// bit fields for kOpDLLEntryPoint
#define DLL_ENTRY_TO_CODE( INDEX, NUM_ARGS, FLAGS )    (((NUM_ARGS) << 19) | FLAGS | INDEX )
#define CODE_TO_DLL_ENTRY_INDEX( VAL )          (VAL & 0x0000FFFF)
#define CODE_TO_DLL_ENTRY_NUM_ARGS( VAL)        (((VAL) & 0x00F80000) >> 19)
#define CODE_TO_DLL_ENTRY_FLAGS( VAL)        (((VAL) & 0x00070000) >> 16)
#define DLL_ENTRY_FLAG_RETURN_VOID		0x10000
#define DLL_ENTRY_FLAG_RETURN_64BIT		0x20000
#define DLL_ENTRY_FLAG_STDCALL			0x40000

