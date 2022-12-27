#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthInner.h: inner interpreter state
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

//class ForthEngine;

extern "C" {

// VAR_ACTIONs are subops of a variable op (store/fetch/incStore/decStore)
#define VAR_ACTION(NAME) static void NAME( ForthCoreState *pCore )
typedef void (*VarAction)( ForthCoreState *pCore );

#define OPTYPE_ACTION(NAME) static void NAME( ForthCoreState *pCore, forthop opVal )

// right now there are about 250 builtin ops, allow for future expansion
#define MAX_BUILTIN_OPS 2048

struct ForthFileInterface
{
    FILE*               (*fileOpen)( const char* pPath, const char* pAccess );
    int                 (*fileClose)( FILE* pFile );
    size_t              (*fileRead)( void* data, size_t itemSize, size_t numItems, FILE* pFile );
    size_t              (*fileWrite)( const void* data, size_t itemSize, size_t numItems, FILE* pFile );
    int                 (*fileGetChar)( FILE* pFile );
    int                 (*filePutChar)( int val, FILE* pFile );
    int                 (*fileAtEnd)( FILE* pFile );
    int                 (*fileExists)( const char* pPath );
    int                 (*fileSeek)( FILE* pFile, long offset, int ctrl );
    long                (*fileTell) ( FILE* pFile );
    int32_t             (*fileGetLength)( FILE* pFile );
    char*               (*fileGetString)( char* buffer, int bufferLength, FILE* pFile );
    int                 (*filePutString)( const char* buffer, FILE* pFile );
    int                 (*fileRemove)( const char* buffer );
    int                 (*fileDup)( int fileHandle );
    int                 (*fileDup2)( int srcFileHandle, int dstFileHandle );
    int                 (*fileNo)( FILE* pFile );
    int                 (*fileFlush)( FILE* pFile );
    int                 (*renameFile)( const char* pOldName, const char* pNewName );
    int                 (*runSystem)( const char* pCmdline );
    int                 (*setWorkDir)(const char* pPath);
    int                 (*getWorkDir)(char* pDstPath, int dstPathMax);
    int                 (*makeDir)( const char* pPath, int mode );
    int                 (*removeDir)( const char* pPath );
	FILE*				(*getStdIn)();
	FILE*				(*getStdOut)();
	FILE*				(*getStdErr)();
	void*				(*openDir)( const char* pPath );	// returns DIR*, which is pDir in readDir, closeDir, rewindDir
	void*				(*readDir)( void* pDir, void* pDstEntry );	// return is a struct dirent*
    int                 (*closeDir)( void* pDir );
	void				(*rewindDir)( void* pDir );
};

#define NUM_CORE_SCRATCH_CELLS 8

struct ForthCoreState
{
    ForthCoreState(int paramStackLongs, int returnStackLongs);
    void InitializeFromEngine(void* pEngine);

    optypeActionRoutine  *optypeAction;

    ucell               numBuiltinOps;

    forthop**           ops;
    ucell               numOps;
    ucell               maxOps;     // current size of table at pUserOps

    void*               pEngine;        // ForthEngine*

    forthop*            IP;            // interpreter pointer

    cell*               SP;            // parameter stack pointer
    
    cell*               RP;            // return stack pointer

    cell*               FP;            // frame pointer
    
    ForthObject         TP;             // this pointer

    VarOperation        varMode;        // operation to perform on variables

    OpResult            state;          // inner loop state - ok/done/error

    ForthError          error;

    cell*               SB;            // param stack base
    cell*               ST;            // empty parameter stack pointer

    ucell               SLen;           // size of param stack in longwords

    cell*               RB;            // return stack base
    cell*               RT;            // empty return stack pointer

    ucell               RLen;           // size of return stack in longwords

    void                *pFiber;		// actually a ForthFiber

    ForthMemorySection* pDictionary;
    ForthFileInterface* pFileFuncs;

    void				*innerLoop;		// inner loop reentry point for assembler inner interpreter
    void				*innerExecute;	// inner loop entry point for assembler inner interpreter for 'execute' op

	ForthObject			consoleOutStream;

    int32_t                base;               // output base
    ucell               signedPrintMode;   // if numers are printed as signed/unsigned
    int32_t                traceFlags;

    ForthExceptionFrame* pExceptionFrame;  // points to current exception handler frame in rstack
    ucell               scratch[NUM_CORE_SCRATCH_CELLS];
};


extern OpResult InnerInterpreter( ForthCoreState *pCore );
extern OpResult InterpretOneOp( ForthCoreState *pCore, forthop op );
#ifdef ASM_INNER_INTERPRETER
extern OpResult InnerInterpreterFast( ForthCoreState *pCore );
extern void InitAsmTables( ForthCoreState *pCore );
extern OpResult InterpretOneOpFast( ForthCoreState *pCore, forthop op );
#endif

void InitDispatchTables( ForthCoreState* pCore );
void CoreSetError( ForthCoreState *pCore, ForthError error, bool isFatal );
void _doIntVarop(ForthCoreState* pCore, int* pVar);
void SpewMethodName(ForthObject obj, forthop opVal);

// DLLRoutine is used for any external DLL routine - it can take any number of arguments
typedef int32_t (*DLLRoutine)();
// CallDLLRoutine is an assembler routine which:
// 1) moves arguments from the forth parameter stack to the real stack in reverse order
// 2) calls the DLL routine
// 3) leaves the DLL routine result on the forth parameter stack
extern void CallDLLRoutine( DLLRoutine function, int32_t argCount, uint32_t flags, ForthCoreState *pCore );

inline forthop GetCurrentOp( ForthCoreState *pCore )
{
    forthop* pIP = pCore->IP - 1;
    return *pIP;
}


#define GET_IP                          (pCore->IP)
#define SET_IP( A )                     (pCore->IP = (A))

#define GET_SP                          (pCore->SP)
#define SET_SP( A )                     (pCore->SP = (A))

#define GET_RP                          (pCore->RP)
#define SET_RP( A )                     (pCore->RP = (A))

#define GET_FP                          (pCore->FP)
#define SET_FP( A )                     (pCore->FP = (A))

#define GET_TP                          (pCore->TP)
#define SET_TP( A )                     (pCore->TP = (A))
#define GET_TP_PTR                      ((ForthObject *)&(pCore->TP))

#define GET_DP                          (pCore->pDictionary->pCurrent)
#define SET_DP( A )                     (pCore->pDictionary->pCurrent = (A))

#define SPOP                            (*pCore->SP++)
#define SPUSH( A )                      (*--pCore->SP = (A))
#define SDROP                           (pCore->SP++)

#define FPOP                            (*(float *)pCore->SP++)
#define FPUSH( A )                      --pCore->SP; *(float *)pCore->SP = A

#if defined(FORTH64)
#define DPOP                            *((double *)(pCore->SP)); pCore->SP += 1
#define DPUSH(A)                        pCore->SP -= 1; *((double *)(pCore->SP)) = A

#define POP64                           SPOP
#define PUSH64(A)                       SPUSH(A)

// LPOP takes a stackInt64
#define LPOP( _SI64 )                   _SI64.s64 = SPOP
#define LPUSH( _SI64 )                  SPUSH(_SI64.s64)
#else
#define DPOP                            *((double *)(pCore->SP)); pCore->SP += 2
#define DPUSH( A )                      pCore->SP -= 2; *((double *)(pCore->SP)) = A

#define POP64                            *((int64_t *)(pCore->SP)); pCore->SP += 2
#define PUSH64(A)                        pCore->SP -= 2; *((int64_t *)(pCore->SP)) = A

// LPOP takes a stackInt64
#define LPOP( _SI64 )                   _SI64.s32[1] = *(pCore->SP); _SI64.s32[0] = (pCore->SP)[1]; pCore->SP += 2
#define LPUSH( _SI64 )                  pCore->SP -= 2; pCore->SP[1] = _SI64.s32[0]; pCore->SP[0] = _SI64.s32[1]
#endif


#define RPOP                            (*pCore->RP++)
#define RPUSH( A )                      (*--pCore->RP = (A))

#define GET_SDEPTH                      (pCore->ST - pCore->SP)
#define GET_RDEPTH                      (pCore->RT - pCore->RP)

#define GET_STATE                       (pCore->state)
#define SET_STATE( A )                  (pCore->state = (A))

#define GET_ENGINE                      ((ForthEngine *) (pCore->pEngine))

#define GET_VAR_OPERATION               (pCore->varMode)
#define SET_VAR_OPERATION( A )          (pCore->varMode = (A))
#define CLEAR_VAR_OPERATION             (pCore->varMode = VarOperation::kVarDefaultOp)

#define GET_NUM_OPS		                (pCore->numOps)

#define GET_CURRENT_OP                  GetCurrentOp( pCore )

#define OP_TABLE                        (pCore->ops)

#define SET_ERROR( A )                  CoreSetError( pCore, A, false )
#define SET_FATAL_ERROR( A )            CoreSetError( pCore, A, true )

#define CONSOLE_CHAR_OUT( CH )          (ForthConsoleCharOut( pCore, CH ))
#define CONSOLE_BYTES_OUT( BUFF, N )    (ForthConsoleBytesOut( pCore, BUFF, N ))
#define CONSOLE_STRING_OUT( BUFF )      (ForthConsoleStringOut( pCore, BUFF ))
#define ERROR_STRING_OUT( BUFF )        (ForthErrorStringOut( pCore, BUFF ))

#define GET_BASE_REF                    (&pCore->base)

#define GET_PRINT_SIGNED_NUM_MODE       (pCore->signedPrintMode)
#define SET_PRINT_SIGNED_NUM_MODE( A )  (pCore->signedPrintMode = (A))

};      // end extern "C"

