#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthCore.h: definitions shared by C++ and assembler code
//
//////////////////////////////////////////////////////////////////////

#ifndef ulong
#define ulong   unsigned long
#endif

// these are opcode types, they are held in the top byte of an opcode, and in
// a vocabulary entry value field
typedef enum
{
    kOpBuiltIn = 0,
    kOpUserDef,

    kOpBranch,          // low 24 bits is signed branch offset
    kOpBranchNZ,
    kOpBranchZ,
    kOpCaseBranch,

    kOpConstant,        // low 24 bits is signed symbol value
    kOpOffset,          // low 24 bits is signed offset value
    kOpArrayOffset,     // low 24 bits is array element size

    kOpConstantString,

    kOpAllocLocals,     // low 24 bits is frame size in longs
    kOpLocalStringInit,     // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
    kOpLocalRef,

    kOpLocalByte,
    kOpLocalShort,
    kOpLocalInt,
    kOpLocalFloat,
    kOpLocalDouble,
    kOpLocalString,
    kOpLocalOp,

    kOpFieldByte,
    kOpFieldShort,
    kOpFieldInt,
    kOpFieldFloat,
    kOpFieldDouble,
    kOpFieldString,
    kOpFieldOp,

    kOpLocalByteArray,
    kOpLocalShortArray,
    kOpLocalIntArray,
    kOpLocalFloatArray,
    kOpLocalDoubleArray,
    kOpLocalStringArray,
    kOpLocalOpArray,

    kOpFieldByteArray,
    kOpFieldShortArray,
    kOpFieldIntArray,
    kOpFieldFloatArray,
    kOpFieldDoubleArray,
    kOpFieldStringArray,
    kOpFieldOpArray,

    kOpMethodWithThis,  // low 24 bits is method number

    kOpMemberByte,
    kOpMemberShort,
    kOpMemberInt,
    kOpMemberFloat,
    kOpMemberDouble,
    kOpMemberString,
    kOpMemberOp,

    kOpLocalUserDefined,             // user can add more optypes starting with this one
    kOpMaxLocalUserDefined = 127,    // maximum user defined optype

    kOpUserMethods  = 128
    // optypes from 128...255 are used to select class methods    
} forthOpType;

// forth native data types
// NOTE: the order of these have to match the order of op type definitions above which
//  are related to native types (kOpLocalByte, kOpMemberFloat, kOpLocalIntArray, ...)
//  as well as the order of actual opcodes used to implement native types (OP_DO_INT, OP_DO_FLOAT, OP_DO_INT_ARRAY, ...)
typedef enum
{
    kBaseTypeByte,
    kBaseTypeShort,
    kBaseTypeInt,
    kBaseTypeFloat,
    kBaseTypeDouble,
    kBaseTypeString,
    kBaseTypeOp,
    kBaseTypeObject,
    kBaseTypeLong
} forthBaseType;

typedef enum {
    kVarDefaultOp = 0,
    kVarFetch,
    kVarRef,
    kVarStore,
    kVarPlusStore,
    kVarMinusStore
} varOperation;

typedef enum {
    kResultOk,          // no need to exit
    kResultDone,        // exit because of "done" opcode
    kResultExitShell,   // exit because of a "bye" opcode
    kResultError,       // exit because of error
    kResultFatalError,  // exit because of fatal error
    kResultException,   // exit because of uncaught exception
    kResultShutdown,    // exit because of a "shutdown" opcode
} eForthResult;

typedef enum {
    kForthErrorNone,
    kForthErrorBadOpcode,
    kForthErrorBadOpcodeType,
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
    kForthErrorBadSyntax,
    // NOTE: if you add errors, make sure that you update ForthEngine::GetErrorString
    kForthNumErrors
} eForthError;

// how sign should be handled while printing integers
typedef enum {
    kPrintSignedDecimal,
    kPrintAllSigned,
    kPrintAllUnsigned
} ePrintSignedMode;

#define ulong  unsigned long
#define ForthEngine void
#define ForthFiberState void

// there is an action routine with this signature for each forthOpType
// user can add new optypes with ForthEngine::AddOpType
typedef void (*optypeActionRoutine)( void *pCore, ulong theData );

typedef void  (*ForthCOp)(void *);

struct ForthCoreState
{
    optypeActionRoutine  optypeAction[ 256 ];

    //ForthCOp             *builtinOps;
    ulong               numBuiltinOps;

    long                **ops;
    ulong               numOps;
    ulong               maxOps;     // current size of table at pUserOps

    ForthEngine         *pEngine;

    // *** beginning of stuff which is per thread ***
    ForthFiberState     *pThread;       // pointer to current thread state

    long                *IPtr;          // interpreter pointer

    long                *SPtr;          // parameter stack pointer
    
    long                *RPtr;          // return stack pointer

    long                *FPtr;           // frame pointer
    
    long                *TPtr;           // this pointer

    varOperation        varMode;        // operation to perform on variables

    eForthResult        state;          // inner loop state - ok/done/error

    eForthError         error;

    long                *STPtr;         // empty parameter stack pointer

    ulong               SLen;           // size of param stack in longwords

    long                *RTPtr;          // empty return stack pointer

    ulong               RLen;           // size of return stack in longwords

    // *** end of stuff which is per thread ***

    // user dictionary stuff
    long                *DP;            // dictionary pointer
    long                *DBase;         // base of dictionary
    ulong               DLen;           // max size of dictionary memory segment
};

