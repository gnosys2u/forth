//////////////////////////////////////////////////////////////////////
//
// Engine.cpp: implementation of the Engine class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(LINUX) || defined(MACOSX)
#include <ctype.h>
#include <stdarg.h>
#include <sys/mman.h>
#endif

#include "Engine.h"
#include "Thread.h"
#include "Shell.h"
#include "Vocabulary.h"
#include "ForthInner.h"
#include "Extension.h"
#include "StructVocabulary.h"
#include "OpcodeCompiler.h"
#include "ForthPortability.h"
#include "BuiltinClasses.h"
#include "BlockFileManager.h"
#include "ParseInfo.h"
#include "OuterInterpreter.h"
#include "ClassVocabulary.h"

extern "C"
{

    extern baseDictionaryEntry baseDictionary[];
	extern void AddForthOps( Engine* pEngine );
#ifdef ASM_INNER_INTERPRETER
    extern void InitAsmTables(  CoreState *pCore );
#endif
    extern OpResult InnerInterp( CoreState *pCore );
    extern void consoleOutToFile( CoreState   *pCore,  const char       *pMessage );
};

extern void OutputToLogger(const char* pBuffer);
// default trace output in non-client/server mode
void defaultTraceOutRoutine(void *pData, const char* pFormat, va_list argList)
{
	(void)pData;
#if defined(LINUX) || defined(MACOSX)
	char buffer[1000];
#else
	TCHAR buffer[1000];
#endif

	Engine* pEngine = Engine::GetInstance();
    int32_t traceFlags = pEngine->GetTraceFlags();
	if ((traceFlags & kLogToConsole) != 0)
	{
#if defined(LINUX) || defined(MACOSX)
		vsnprintf(buffer, sizeof(buffer), pFormat, argList);
#elif defined(_WIN64) || defined(WIN32)
        StringCchVPrintfA(buffer, sizeof(buffer), pFormat, argList);
#else
		wvnsprintf(buffer, sizeof(buffer), pFormat, argList);
#endif

        pEngine->ConsoleOut(buffer);
	}
    else if ((traceFlags & kLogToFile) != 0)
    {
#if defined(LINUX) || defined(MACOSX)
        vsnprintf(buffer, sizeof(buffer), pFormat, argList);
#elif defined(_WIN64) || defined(WIN32)
        StringCchVPrintfA(buffer, sizeof(buffer), pFormat, argList);
#else
        wvnsprintf(buffer, sizeof(buffer), pFormat, argList);
#endif
        FILE* logfile = fopen("_forthlog.txt", "a");
        fwrite(buffer, strlen(buffer), 1, logfile);
        fclose(logfile);
    }
    else
	{
#if defined(LINUX) || defined(MACOSX)
		vsnprintf(buffer, sizeof(buffer), pFormat, argList);
#elif defined(_WIN64) || defined(WIN32)
        StringCchVPrintfA(buffer, sizeof(buffer), pFormat, argList);
#else
		wvnsprintf(buffer, sizeof(buffer), pFormat, argList);
#endif
        OutputToLogger(buffer);
	}
}

extern "C"
{
	void traceOp(CoreState* pCore, forthop* pIP, forthop op)
	{
		if (pCore->traceFlags != 0)
		{
		    Engine* pEngine = (Engine *)(pCore->pEngine);
			if ((pCore->traceFlags & kLogStack) != 0)
			{
				pEngine->TraceStack(pCore);
			}

            if (pCore->traceFlags & (kLogStack | kLogInnerInterpreter))
            {
                pEngine->TraceOut("\n");
            }

            if (pCore->traceFlags & kLogInnerInterpreter)
            {
                pEngine->TraceOp(pIP, op);
                forthOpType opType = FORTH_OP_TYPE(op);
                if (opType == kOpMethodWithThis)
                {
                    ForthObject thisObject = GET_TP;
                    int32_t opVal = FORTH_OP_VALUE(op);
                    SpewMethodName(thisObject, opVal);
                }
                else if (opType == kOpMethodWithTOS)
                {
                    ForthObject thisObject = (ForthObject)*(pCore->SP);
                    int32_t opVal = FORTH_OP_VALUE(op);
                    SpewMethodName(thisObject, opVal);
                }
                else if (opType == kOpMethodWithSuper)
                {
                    ForthObject thisObject = GET_TP;
                    int32_t opVal = FORTH_OP_VALUE(op);
                    SpewMethodName(thisObject, opVal);
                }
            }

            if (pCore->traceFlags & kLogProfiler)
            {
                pEngine->AddOpExecutionToProfile(op);
            }
        }
	}
};

//#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS MAX_BUILTIN_OPS
static const char *gOpNames[ NUM_TRACEABLE_OPS ];

//#endif

Engine* Engine::mpInstance = nullptr;

#define ERROR_STRING_MAX    256

///////////////////////////////////////////////////////////////////////
//
// opTypeNames must be kept in sync with forthOpType enum in forth.h
//

static const char *opTypeNames[] =
{
    "Native", "NativeImmediate", "UserDefined", "UserDefinedImmediate", "CCode", "CCodeImmediate", "RelativeDef", "RelativeDefImmediate", "DLLEntryPoint", 0,
    "Branch", "BranchTrue", "BranchFalse", "CaseBranchT", "CaseBranchF", "PushBranch", "RelativeDefBranch", "RelativeData", "RelativeString", 0,
	"Constant", "ConstantString", "Offset", "ArrayOffset", "AllocLocals", "LocalRef", "LocalStringInit", "LocalStructArray", "OffsetFetch", "MemberRef",
    "LocalByte", "LocalUByte", "LocalShort", "LocalUShort", "LocalInt", "LocalUInt", "LocalLong", "LocalULong", "LocalFloat", "LocalDouble",
	"LocalString", "LocalOp", "LocalObject", "LocalByteArray", "LocalUByteArray", "LocalShortArray", "LocalUShortArray", "LocalIntArray", "LocalUIntArray", "LocalLongArray",
	"LocalULongArray", "LocalFloatArray", "LocalDoubleArray", "LocalStringArray", "LocalOpArray", "LocalObjectArray", "FieldByte", "FieldUByte", "FieldShort", "FieldUShort",
	"FieldInt", "FieldUInt", "FieldLong", "FieldULong", "FieldFloat", "FieldDouble", "FieldString", "FieldOp", "FieldObject", "FieldByteArray",
	"FieldUByteArray", "FieldShortArray", "FieldUShortArray", "FieldIntArray", "FieldUIntArray", "FieldLongArray", "FieldULongArray", "FieldFloatArray", "FieldDoubleArray", "FieldStringArray",
	"FieldOpArray", "FieldObjectArray", "MemberByte", "MemberUByte", "MemberShort", "MemberUShort", "MemberInt", "MemberUInt", "MemberLong", "MemberULong",
	"MemberFloat", "MemberDouble", "MemberString", "MemberOp", "MemberObject", "MemberByteArray", "MemberUByteArray", "MemberShortArray", "MemberUShortArray", "MemberIntArray",
	"MemberUIntArray", "MemberLongArray", "MemberULongArray", "MemberFloatArray", "MemberDoubleArray", "MemberStringArray", "MemberOpArray", "MemberObjectArray", "MethodWithThis", "MethodWithTOS",
	"MemberStringInit", "NumVaropOpCombo", "NumVaropCombo", "NumOpCombo", "VaropOpCombo", "OpBranchFalseCombo", "OpBranchTrueCombo", "SquishedFloat", "SquishedDouble", "SquishedLong",
	"LocalRefOpCombo", "MemberRefOpCombo", "MethodWithSuper",
    "NativeU32", "NativeS32", "NativeF32", "NativeS64", "NativeF64",
    "CCodeU32", "CCodeS32", "CCodeF32", "CCodeS64", "CCodeF64",
    "UserDefU32", "UserDefS32", "UserDefF32", "UserDefS64", "UserDefF64",
    "MethodWithLocalObject", "MethodWithMemberObject"
};


///////////////////////////////////////////////////////////////////////
//
// pErrorStrings must be kept in sync with ForthError enum in forth.h
//
static const char *pErrorStrings[] =
{
    "No Error",
    "Bad Opcode",
    "Bad OpcodeType",
    "Bad Parameter",
    "Bad Variable Operation",
    "Parameter Stack Underflow",
    "Parameter Stack Overflow",
    "Return Stack Underflow",
    "Return Stack Overflow",
    "Unknown Symbol",
    "File Open Failed",
    "Aborted",
    "Can't Forget Builtin Op",
    "Bad Method Number",
    "Exception",
    "Missing Preceeding Size Constant",
    "Error In Struct Definition",
    "Error In User-Defined Op",
    "Syntax error",
    "Bad Preprocessor Directive",
	"Unimplemented Method",
	"Illegal Method",
    "Shell Stack Underflow",
    "Shell Stack Overflow",
	"Bad Reference Count",
	"IO error",
	"Bad Object",
    "StringOverflow",
	"Bad Array Index",
    "Illegal Operation",
    "System Exception"
};

//////////////////////////////////////////////////////////////////////
////
///
//                     Engine
// 

Engine::Engine()
: mpOuter(nullptr)
, mpThreads(nullptr)
, mpMainThread( nullptr )
, mFastMode( true )
, mpExtension( nullptr )
, mpCore( nullptr )
, mpShell( nullptr )
, mTraceOutRoutine(defaultTraceOutRoutine)
, mpTraceOutData( nullptr )
, mBlockFileManager( nullptr )
, mIsServer(false)
, mDefaultConsoleOutStream(nullptr)
, mErrorOutStream(nullptr)
{
    // scratch area for temporary definitions
    ASSERT( mpInstance == nullptr );
    mpInstance = this;
    mpEngineScratch = new int32_t[70];
    mpErrorString = new char[ ERROR_STRING_MAX + 1 ];

    // remember creation time for elapsed time method
#ifdef WIN32
#ifdef MSDEV
    _ftime32_s( &mStartTime );
#else
    _ftime( &mStartTime );
#endif
#elif defined(LINUX)
    clock_gettime(CLOCK_REALTIME, &mStartTime);
#else
    ftime( &mStartTime );
#endif

	mDictionary.pBase = nullptr;

    ResetExecutionProfile();

    // At this point, the main thread does not exist, it will be created later in Initialize, this
    // is fairly screwed up, it is becauses originally Engine was the center of the universe,
    // and it created the shell, but now the shell is created first, and the shell or the main app
    // can create the engine, and then the shell calls OuterInterpreter::Initialize to hook the two up.
    // The main thread needs to get the file interface from the shell, so it can't be created until
    // after the engine is connected to the shell.  Did I mention its screwed up?
}

Engine::~Engine()
{
    if ( mpExtension != nullptr)
    {
        mpExtension->Shutdown();
    }
	
    delete mpOuter;

    // delete all thread and fiber objects
    Thread* pThread = mpThreads;
    while (pThread != nullptr)
    {
        Thread* pNextThread = pThread->mpNext;
        pThread->FreeObjects();
        pThread = pNextThread;
    }

    if (mDictionary.pBase)
    {
#ifdef WIN32
		VirtualFree( mDictionary.pBase, 0, MEM_RELEASE );
#elif defined(MACOSX) || defined(LINUX)
        munmap(mDictionary.pBase, mDictionary.len * sizeof(forthop));
#else
        __FREE( mDictionary.pBase );
#endif
    }
    delete [] mpErrorString;

    Forgettable* pForgettable = Forgettable::GetForgettableChainHead();
    while ( pForgettable != nullptr )
    {
        Forgettable* pNextForgettable = pForgettable->GetNextForgettable();
        delete pForgettable;
        pForgettable = pNextForgettable;
    }

    if ( mpCore->optypeAction )
    {
		__FREE(mpCore->optypeAction);
    }

    if ( mpCore->ops )
    {
		__FREE(mpCore->ops);
    }

    // delete all threads;
	pThread = mpThreads;
	while (pThread != nullptr)
    {
		Thread *pNextThread = pThread->mpNext;
		delete pThread;
		pThread = pNextThread;
    }

    delete mpEngineScratch;

    delete mBlockFileManager;

    mpInstance = nullptr;
}

Engine*
Engine::GetInstance( void )
{
    ASSERT( mpInstance != nullptr);
    return mpInstance;
}

//############################################################################
//
//    system initialization
//
//############################################################################

void Engine::Initialize(
    Shell*        pShell,
    int                totalLongs,
    bool               bAddBaseOps,
    Extension*    pExtension )
{
    mpShell = pShell;

    mBlockFileManager = new BlockFileManager(mpShell->GetBlockfilePath());

    size_t dictionarySize = totalLongs * sizeof(forthop);
#ifdef WIN32
	void* dictionaryAddress = nullptr;
	// we need to allocate memory that is immune to Data Execution Prevention
	mDictionary.pBase = (forthop *) VirtualAlloc( dictionaryAddress, dictionarySize, (MEM_COMMIT | MEM_RESERVE), PAGE_EXECUTE_READWRITE );
#elif defined(MACOSX) || defined(LINUX)
    mDictionary.pBase = (forthop *) mmap(nullptr, dictionarySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
#else
	 mDictionary.pBase = (forthop *) __MALLOC( dictionarySize );
#endif
    mDictionary.pCurrent = mDictionary.pBase;
    mDictionary.len = totalLongs;

    mpMainThread = CreateThread( 0, MAIN_THREAD_PSTACK_LONGS, MAIN_THREAD_RSTACK_LONGS );
    mpMainThread->SetName("MainThread");
    mpMainThread->GetFiber(0)->SetName("MainFiber");
	mpCore = mpMainThread->GetFiber(0)->GetCore();
	mpCore->optypeAction = (optypeActionRoutine *) __MALLOC(sizeof(optypeActionRoutine) * 256);
    mpCore->numBuiltinOps = 0;
    mpCore->numOps = 0;
    mpCore->maxOps = MAX_BUILTIN_OPS;
	mpCore->ops = (forthop **) __MALLOC(sizeof(forthop *) * mpCore->maxOps);

    mpOuter = new OuterInterpreter(this);
    mpOuter->Initialize();

    //
    // build dispatch table for different opcode types
    //
    InitDispatchTables( mpCore );
#ifdef ASM_INNER_INTERPRETER
    InitAsmTables( mpCore );
#endif

    if ( bAddBaseOps )
    {
		AddForthOps( this );
        mpOuter->AddBuiltinClasses();
    }

	// the primary thread objects can't be inited until builtin classes are initialized
	OThread::FixupThread(mpMainThread);

    GetForthConsoleOutStream(mpCore, mDefaultConsoleOutStream);
    GetForthErrorOutStream(mpCore, mErrorOutStream);
    ResetConsoleOut( *mpCore );

    if (pExtension != nullptr)
    {
        mpExtension = pExtension;
        mpExtension->Initialize(this);
    }

    Reset();
}


void
Engine::ToggleFastMode( void )
{
    SetFastMode(!mFastMode );
}

bool
Engine::GetFastMode( void )
{
    return mFastMode;
}

void
Engine::SetFastMode( bool goFast )
{
    mFastMode = goFast;
}

void Engine::Reset( void )
{
    mpOuter->Reset();

    if (mpExtension != nullptr)
    {
        mpExtension->Reset();
    }
}

void Engine::ErrorReset( void )
{
    Reset();
	mpMainThread->Reset();
}

cell* Engine::GetCompileStatePtr(void)
{
    return mpOuter->GetCompileStatePtr();
}

void Engine::SetCompileState(cell v)
{
    mpOuter->SetCompileState(v);
}

cell Engine::IsCompiling(void)
{
    return mpOuter->IsCompiling();
}


void Engine::ShowMemoryInfo()
{
    std::vector<MemoryStats *> memoryStats;
    int numStorageBlocks;
    int totalStorage;
    int unusedStorage;
    s_memoryManager->getStats(memoryStats, numStorageBlocks, totalStorage, unusedStorage);
    char buffer[256];
    for (MemoryStats* stats : memoryStats)
    {
        if (stats->getNumAllocations() > 0)
        {
            snprintf(buffer, sizeof(buffer), "%s allocs:%lld frees:%lld maxInUse:%lld currentUsed:%lld maxUsed:%lld\n",
                stats->getName().c_str(), stats->getNumAllocations(), stats->getNumDeallocations(),
                stats->getMaxActiveAllocations(), stats->getBytesInUse(), stats->getMaxBytesInUse());
            ForthConsoleStringOut(mpCore, buffer);
        }
    }
    snprintf(buffer, sizeof(buffer), "%d storage blocks, total %d bytes, %d bytes unused\n",
        numStorageBlocks, totalStorage, unusedStorage);
    ForthConsoleStringOut(mpCore, buffer);
}

Thread * Engine::CreateThread(forthop fiberOp, int paramStackSize, int returnStackSize )
{
	Thread *pThread = new Thread(this, paramStackSize, returnStackSize);
	Fiber *pNewThread = pThread->GetFiber(0);
	pNewThread->SetOp(fiberOp);

    InitCoreState(pNewThread->mCore);

	pThread->mpNext = mpThreads;
	mpThreads = pThread;

	return pThread;
}


void Engine::InitCoreState(CoreState& core)
{
   core.pEngine = this;
   core.pDictionary = &mDictionary;
   core.pFileFuncs = mpShell->GetFileInterface();

    if (mpCore != nullptr)
    {
        // fill in optype & opcode action tables from engine thread
       core.optypeAction = mpCore->optypeAction;
       core.numBuiltinOps = mpCore->numBuiltinOps;
       core.numOps = mpCore->numOps;
       core.maxOps = mpCore->maxOps;
       core.ops = mpCore->ops;
       core.innerLoop = mpCore->innerLoop;
       core.innerExecute = mpCore->innerExecute;
       core.innerExecute = mpCore->innerExecute;
    }
}


void Engine::DestroyThread(Thread *pThread)
{
	Thread *pNext, *pCurrent;

    if ( mpThreads == pThread )
    {

        // special case - thread is head of list
		mpThreads = (Thread *)(mpThreads->mpNext);
        delete pThread;

    }
    else
    {

        // TODO: this is untested
        pCurrent = mpThreads;
        while ( pCurrent != nullptr )
        {
			pNext = (Thread *)(pCurrent->mpNext);
            if ( pThread == pNext )
            {
                pCurrent->mpNext = pNext->mpNext;
                delete pThread;
                return;
            }
            pCurrent = pNext;
        }

        SPEW_ENGINE( "OuterInterpreter::DestroyThread tried to destroy unknown thread 0x%x!\n", pThread );
        // TODO: raise the alarm
    }
}


// interpret named file, interpret from standard in if pFileName is nullptr
// return 0 for normal exit
bool
Engine::PushInputFile( const char *pInFileName )
{
    return mpShell->PushInputFile( pInFileName );
}

void
Engine::PushInputBuffer( const char *pDataBuffer, int dataBufferLen )
{
    mpShell->PushInputBuffer( pDataBuffer, dataBufferLen );
}

void
Engine::PushInputBlocks(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock)
{
    mpShell->PushInputBlocks(pManager, firstBlock, lastBlock);
}


void
Engine::PopInputStream( void )
{
    mpShell->PopInputStream();
}

// TODO: tracing of built-in ops won't work for user-added builtins...
const char *
Engine::GetOpTypeName( int32_t opType )
{
    return (opType <= kOpLastBaseDefined) ? opTypeNames[opType] : "unknown";
}

static bool lookupUserTraces = true;


void Engine::TraceOut(const char* pFormat, ...)
{
	if (mTraceOutRoutine != nullptr)
	{
		va_list argList;
		va_start(argList, pFormat);

		mTraceOutRoutine(mpTraceOutData, pFormat, argList);

		va_end(argList);
	}
}


void Engine::SetTraceOutRoutine( traceOutRoutine traceRoutine, void* pTraceData )
{
	mTraceOutRoutine = traceRoutine;
	mpTraceOutData = pTraceData;
}

void Engine::GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const
{
	traceRoutine = mTraceOutRoutine;
	pTraceData = mpTraceOutData;
}

void Engine::TraceOp(forthop* pOp, forthop op)
{
#ifdef TRACE_INNER_INTERPRETER
    char buff[ 512 ];
    int numFollowing;
#if 0
    int rDepth = pCore->RT - pCore->RP;
    char* sixteenSpaces = "                ";     // 16 spaces
	//if ( *pOp != gCompiledOps[OP_DONE] )
	{
		DescribeOp(pOp, buff, sizeof(buff), lookupUserTraces, numFollowing);
		mpEngine->TraceOut("# 0x%08x #", pOp);
		while (rDepth > 16)
		{
			mpEngine->TraceOut(sixteenSpaces);
			rDepth -= 16;
		}
		char* pIndent = sixteenSpaces + (16 - rDepth);
		mpEngine->TraceOut("%s%s # ", pIndent, buff);
	}
#else
    forthop opIn;
    if (pOp == nullptr)
    {
        // this is used for indirect op execution, where the op to be executed
        //  isn't pointed to by the IP, things like 'execute' or methods
        opIn = op;
        pOp = &opIn;
    }
    DescribeOp(pOp, buff, sizeof(buff), lookupUserTraces, numFollowing);
    TraceOut("# 0x%16p # %s # ", pOp, buff);
#endif
#endif
}

void Engine::TraceStack( CoreState* pCore )
{
	ucell i;

    cell *pSP = GET_SP;
    ucell nItems = GET_SDEPTH;
    TraceOut("  stack[%d]:", nItems);
#define MAX_TRACE_STACK_ITEMS 8
#if defined(WIN32)
	ucell numToDisplay = min((ucell)MAX_TRACE_STACK_ITEMS, nItems);
#else
	ucell numToDisplay = std::min((ucell)MAX_TRACE_STACK_ITEMS, nItems);
#endif
	for (i = 0; i < numToDisplay; i++)
	{
#if defined(FORTH64)
        TraceOut(" %llx", *pSP++);
#else
        TraceOut( " %x", *pSP++ );
#endif
	}

	if (nItems > numToDisplay)
	{
		TraceOut(" <%d more>", nItems - numToDisplay);
	}

    cell *pRP = GET_RP;
    nItems = pCore->RT - pRP;
    TraceOut("  rstack[%d]", nItems);
#define MAX_TRACE_RSTACK_ITEMS 8
#if defined(WIN32)
    numToDisplay = min(MAX_TRACE_RSTACK_ITEMS, nItems);
#else
    numToDisplay = std::min((ucell)MAX_TRACE_RSTACK_ITEMS, nItems);
#endif
    for (i = 0; i < numToDisplay; i++)
    {
#if defined(FORTH64)
        TraceOut(" %llx", *pRP++);
#else
        TraceOut(" %x", *pRP++);
#endif
    }

    if (nItems > numToDisplay)
    {
        TraceOut(" <%d more>", nItems - numToDisplay);
    }
}

void Engine::DescribeOp(forthop *pOp, char *pBuffer, int buffSize, bool lookupUserDefs, int& numFollowing)
{
    forthop op = *pOp;
    forthOpType opType = FORTH_OP_TYPE( op );
    uint32_t opVal = FORTH_OP_VALUE( op );
    Vocabulary *pFoundVocab = nullptr;
    forthop *pEntry = nullptr;

    // the most common case - this op isn't followed by any immediate data
    numFollowing = 0;

	const char* preamble = "%02x:%06x    ";
	int preambleSize = (int)strlen( preamble );
	if ( buffSize <= (preambleSize + 1) )
	{
		return;
	}

    SNPRINTF( pBuffer, buffSize, preamble, opType, opVal );
    preambleSize = strlen(pBuffer);
    pBuffer += preambleSize;
	buffSize -= preambleSize;
    if ( opType >= (sizeof(opTypeNames) / sizeof(char *)) )
    {
        SNPRINTF( pBuffer, buffSize, "BadOpType" );
    }
    else
    {
        const char *opTypeName = opTypeNames[opType];

        bool searchVocabsForOp = false;

        switch( opType )
        {
            case kOpNative:
            case kOpNativeImmediate:
            case kOpCCode:
            case kOpCCodeImmediate:
                if ( (opVal < NUM_TRACEABLE_OPS) && (gOpNames[opVal] != nullptr) )
                {
                    forthop oppy = COMPILED_OP(opType, opVal);
                    // traceable built-in op
					if ( oppy == gCompiledOps[OP_INT_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s 0x%x", gOpNames[opVal], pOp[1] );
                        numFollowing = 1;
					}
                    else if (oppy == gCompiledOps[OP_UINT_VAL])
                    {
                        // TODO - differentiate between signed and unsigned
                        SNPRINTF(pBuffer, buffSize, "%s U0x%x", gOpNames[opVal], pOp[1]);
                        numFollowing = 1;
                    }
                    else if (oppy == gCompiledOps[OP_FLOAT_VAL])
                    {
                        SNPRINTF(pBuffer, buffSize, "%s %f", gOpNames[opVal], *((float*)(&(pOp[1]))));
                        numFollowing = 1;
                    }
                    else if ( oppy == gCompiledOps[OP_DOUBLE_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s %g", gOpNames[opVal], *((double *)(&(pOp[1]))) );
                        numFollowing = 2;
                    }
					else if ( oppy == gCompiledOps[OP_LONG_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s 0x%llx", gOpNames[opVal], *((int64_t *)(&(pOp[1]))) );
                        numFollowing = 2;
                    }
					else
					{
						SNPRINTF( pBuffer, buffSize, "%s", gOpNames[opVal] );
					}
                }
                else
                {
                    searchVocabsForOp = true;
                }
                break;
            
            case kOpNativeU32:
            case kOpCCodeU32:
            case kOpNativeS32:
            case kOpCCodeS32:
            case kOpNativeF32:
            case kOpCCodeF32:
            {
                SNPRINTF(pBuffer, buffSize, "0x%x ", pOp[1]);
                size_t immediateValSize = strlen(pBuffer);
                pBuffer += immediateValSize;
                buffSize -= immediateValSize;
                if ((opVal < NUM_TRACEABLE_OPS) && (gOpNames[opVal] != nullptr))
                {
                    SNPRINTF(pBuffer, buffSize, "%s", gOpNames[opVal]);
                }
                else
                {
                    searchVocabsForOp = true;
                }
                numFollowing = 1;
                break;
            }

            case kOpNativeS64:
            case kOpCCodeS64:
            case kOpNativeF64:
            case kOpCCodeF64:
            {
                SNPRINTF(pBuffer, buffSize, "0x%llx ", *((int64_t*)(&(pOp[1]))));
                size_t immediateValSize = strlen(pBuffer);
                pBuffer += immediateValSize;
                buffSize -= immediateValSize;
                if ((opVal < NUM_TRACEABLE_OPS) && (gOpNames[opVal] != nullptr))
                {
                    SNPRINTF(pBuffer, buffSize, "%s", gOpNames[opVal]);
                }
                else
                {
                    searchVocabsForOp = true;
                }
                numFollowing = 2;
                break;
            }

            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpDLLEntryPoint:
                if (lookupUserDefs)
                {
                    searchVocabsForOp = true;
                }
                break;

            case kOpUserDefU32:
            case kOpUserDefS32:
            case kOpUserDefF32:
            {
                SNPRINTF(pBuffer, buffSize, "0x%x ", pOp[1]);
                size_t immediateValSize = strlen(pBuffer);
                pBuffer += immediateValSize;
                buffSize -= immediateValSize;
                if (lookupUserDefs)
                {
                    searchVocabsForOp = true;
                }
                numFollowing = 1;
                break;
            }

            case kOpUserDefS64:
            case kOpUserDefF64:
            {
                SNPRINTF(pBuffer, buffSize, "0x%llx ", *((int64_t*)(&(pOp[1]))));
                size_t immediateValSize = strlen(pBuffer);
                pBuffer += immediateValSize;
                buffSize -= immediateValSize;
                if (lookupUserDefs)
                {
                    searchVocabsForOp = true;
                }
                numFollowing = 2;
                break;
            }

            case kOpLocalByte:          case kOpLocalByteArray:
            case kOpLocalShort:         case kOpLocalShortArray:
            case kOpLocalInt:           case kOpLocalIntArray:
            case kOpLocalFloat:         case kOpLocalFloatArray:
            case kOpLocalDouble:        case kOpLocalDoubleArray:
            case kOpLocalString:        case kOpLocalStringArray:
            case kOpLocalOp:            case kOpLocalOpArray:
            case kOpLocalLong:          case kOpLocalLongArray:
            case kOpLocalObject:        case kOpLocalObjectArray:
            case kOpLocalUByte:         case kOpLocalUByteArray:
            case kOpLocalUShort:        case kOpLocalUShortArray:
            case kOpLocalUInt:          case kOpLocalUIntArray:
            case kOpMemberByte:         case kOpMemberByteArray:
            case kOpMemberShort:        case kOpMemberShortArray:
            case kOpMemberInt:          case kOpMemberIntArray:
            case kOpMemberFloat:        case kOpMemberFloatArray:
            case kOpMemberDouble:       case kOpMemberDoubleArray:
            case kOpMemberString:       case kOpMemberStringArray:
            case kOpMemberOp:           case kOpMemberOpArray:
            case kOpMemberLong:         case kOpMemberLongArray:
            case kOpMemberObject:       case kOpMemberObjectArray:
            case kOpMemberUByte:        case kOpMemberUByteArray:
            case kOpMemberUShort:       case kOpMemberUShortArray:
            case kOpMemberUInt:         case kOpMemberUIntArray:
            case kOpFieldByte:          case kOpFieldByteArray:
            case kOpFieldShort:         case kOpFieldShortArray:
            case kOpFieldInt:           case kOpFieldIntArray:
            case kOpFieldFloat:         case kOpFieldFloatArray:
            case kOpFieldDouble:        case kOpFieldDoubleArray:
            case kOpFieldString:        case kOpFieldStringArray:
            case kOpFieldOp:            case kOpFieldOpArray:
            case kOpFieldLong:          case kOpFieldLongArray:
            case kOpFieldObject:        case kOpFieldObjectArray:
            case kOpFieldUByte:         case kOpFieldUByteArray:
            case kOpFieldUShort:        case kOpFieldUShortArray:
            case kOpFieldUInt:          case kOpFieldUIntArray:
            {
                if ((opVal & 0xF00000) != 0)
                {
                    VarOperation varOp = (VarOperation)(opVal >> 20);
                    SNPRINTF(pBuffer, buffSize, "%s_%x%s", opTypeName, (opVal & 0xFFFFF), ParseInfo::GetVaropSuffix(varOp));
                }
                else
                {
                    SNPRINTF(pBuffer, buffSize, "%s_%x", opTypeName, opVal);
                }
                break;
            }

            case kOpConstantString:
                SNPRINTF( pBuffer, buffSize, "\"%s\"", (char *)(pOp + 1) );
                numFollowing = opVal;
                break;

            case kOpPushBranch:
                numFollowing = opVal + 1;
                SNPRINTF(pBuffer, buffSize, "%s    %d following longs", opTypeName, numFollowing);
                break;

            case kOpRelativeDefBranch:
                // don't set numFollowing, otherwise following ops will be displayed as data not code
                SNPRINTF(pBuffer, buffSize, "%s    0x%16p", opTypeName, opVal + 1 + pOp);
                break;

            case kOpConstant:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                }
                SNPRINTF( pBuffer, buffSize, "%s    %d", opTypeName, opVal );
                break;

            case kOpOffset:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                    SNPRINTF( pBuffer, buffSize, "%s    %d", opTypeName, opVal );
                }
                else
                {
                    SNPRINTF( pBuffer, buffSize, "%s    +%d", opTypeName, opVal );
                }
                break;

            case kOpCaseBranchT:  case kOpCaseBranchF:
            case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                }
                SNPRINTF( pBuffer, buffSize, "%s    0x%16p", opTypeName, opVal + 1 + pOp );
                break;

            case kOpOZBCombo:  case kOpONZBCombo:
            {
                const char* pBranchType = (opType == kOpOZBCombo) ? "BranchFalse" : "BranchTrue";
                int32_t embeddedOp = opVal & 0xFFF;
                int32_t branchOffset = opVal >> 12;
                if (opVal & 0x800000)
                {
                    branchOffset |= 0xFFFFF000;
                }
                SNPRINTF(pBuffer, buffSize, "%s   %s   %s 0x%16p", opTypeName,
                    gOpNames[embeddedOp], pBranchType, branchOffset + 1 + pOp);
                break;
            }

            case kOpLocalRefOpCombo:  case kOpMemberRefOpCombo:
            {
                const char* pVarType = (opType == kOpLocalRefOpCombo) ? "Local" : "Member";
                int32_t varOffset = opVal & 0xFFF;
                int32_t embeddedOp = opVal >> 12;
                SNPRINTF(pBuffer, buffSize, "%s   &%s_%x   %s", opTypeName,
                    pVarType, varOffset, gOpNames[embeddedOp]);
                break;
            }
            
            case kOpLocalStringInit:    // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
            case kOpMemberStringInit:   // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
                SNPRINTF( pBuffer, buffSize, "%s    maxBytes %d offset %d", opTypeName, opVal & 0xFFF, opVal >> 12 );
                break;
            
            case kOpLocalStructArray:   // bits 0..11 are padded struct size in bytes, bits 12..23 are frame offset in longs
                SNPRINTF( pBuffer, buffSize, "%s    elementSize %d offset %d", opTypeName, opVal & 0xFFF, opVal >> 12 );
                break;
            
            case kOpAllocLocals:
                SNPRINTF( pBuffer, buffSize, "%s    cells %d", opTypeName, opVal );
                break;
            
            case kOpArrayOffset:
                SNPRINTF( pBuffer, buffSize, "%s    elementSize %d", opTypeName, opVal );
                break;
            
            case kOpMethodWithThis:
            case kOpMethodWithTOS:
            case kOpMethodWithSuper:
                SNPRINTF( pBuffer, buffSize, "%s    %d", opTypeName, opVal );
                break;

            case kOpSquishedFloat:
				SNPRINTF( pBuffer, buffSize, "%s %f", opTypeName, mpOuter->UnsquishFloat( opVal ) );
				break;

            case kOpSquishedDouble:
				SNPRINTF( pBuffer, buffSize, "%s %g", opTypeName, mpOuter->UnsquishDouble( opVal ) );
				break;

            case kOpSquishedLong:
				SNPRINTF( pBuffer, buffSize, "%s %lld", opTypeName, mpOuter->UnsquishLong( opVal ) );
				break;

            default:
                if ( opType >= (uint32_t)(sizeof(opTypeNames) / sizeof(char *)) )
                {
                    SNPRINTF( pBuffer, buffSize, "BAD OPTYPE!" );
                }
                else
                {
                    SNPRINTF( pBuffer, buffSize, "%s", opTypeName );
                }
                break;
        }

        if (searchVocabsForOp)
        {
            pEntry = mpOuter->GetVocabularyStack()->FindSymbolByValue(op, &pFoundVocab);
            if (pEntry == nullptr)
            {
                Vocabulary* pVocab = Vocabulary::GetVocabularyChainHead();
                while (pVocab != nullptr)
                {
                    pEntry = pVocab->FindSymbolByValue(op);
                    if (pEntry != nullptr)
                    {
                        pFoundVocab = pVocab;
                        break;
                    }
                    pVocab = pVocab->GetNextChainVocabulary();
                }
            }

            if (pEntry)
            {
                // the symbol name in the vocabulary doesn't always have a terminating null
                int len = pFoundVocab->GetEntryNameLength(pEntry);
                const char* pVocabName = pFoundVocab->GetName();
                while (*pVocabName != '\0')
                {
                    *pBuffer++ = *pVocabName++;
                }
                *pBuffer++ = ':';
                const char* pName = pFoundVocab->GetEntryName(pEntry);
                for (int i = 0; i < len; i++)
                {
                    *pBuffer++ = *pName++;
                }
                *pBuffer = '\0';
            }
            else
            {
                SNPRINTF(pBuffer, buffSize, "%s(%d)", opTypeName, opVal);
            }
        }
    }
}

void Engine::AddOpExecutionToProfile(forthop op)
{
    forthOpType opType = FORTH_OP_TYPE(op);
    uint32_t opVal = FORTH_OP_VALUE(op);

    switch (opType)
    {

    case kOpNative:
    case kOpNativeImmediate:
    case kOpCCode:
    case kOpCCodeImmediate:
    case kOpUserDef:
    case kOpUserDefImmediate:
    case kOpDLLEntryPoint:
    {
        size_t oldSize = mProfileOpcodeCounts.size();
        if (oldSize <= opVal)
        {
            size_t newSize = opVal + 64;
            mProfileOpcodeCounts.resize(newSize);
            for (size_t i = oldSize; i < newSize; ++i)
            {
                mProfileOpcodeCounts[i].count = 0;
                mProfileOpcodeCounts[i].op = 0;
                mProfileOpcodeCounts[i].pEntry = nullptr;
                mProfileOpcodeCounts[i].pVocabulary = nullptr;
            }
        }
        mProfileOpcodeCounts[opVal].count += 1;
        mProfileOpcodeCounts[opVal].op = op;
    }
    break;

    default:
        break;
    }
    mProfileOpcodeTypeCounts[opType] += 1;
}

void Engine::DumpExecutionProfile()
{
    char buffer[256];
    char opBuffer[128];

    Vocabulary* pVocab = Vocabulary::GetVocabularyChainHead();
    while (pVocab != nullptr)
    {
        forthop *pEntry = pVocab->GetNewestEntry();
        if (pVocab->IsClass())
        {

            forthop* pMethods = ((ClassVocabulary *)pVocab)->GetMethods();
            while (pEntry != nullptr)
            {
                forthop op = *pEntry;
                int32_t typeCode = pEntry[1];
                if (CODE_IS_METHOD(typeCode))
                {
                    uint32_t opVal = FORTH_OP_VALUE(op);
                    int32_t methodOp = pMethods[opVal];
                    forthOpType opType = FORTH_OP_TYPE(methodOp);
                    opVal = FORTH_OP_VALUE(methodOp);
                    switch (opType)
                    {

                    case kOpNative:
                    case kOpNativeImmediate:
                    case kOpCCode:
                    case kOpCCodeImmediate:
                    case kOpUserDef:
                    case kOpUserDefImmediate:
                    case kOpDLLEntryPoint:
                    {
                        if (opVal < mProfileOpcodeCounts.size())
                        {
                            opcodeProfileInfo& opInfo = mProfileOpcodeCounts[opVal];
                            if ((opInfo.op == methodOp) || (opInfo.op == 0))
                            {
                                opInfo.pEntry = pEntry;
                                opInfo.pVocabulary = pVocab;
                            }
                        }
                    }
                    break;

                    default:
                        break;
                    }
                }


                pEntry = pVocab->NextEntrySafe(pEntry);
            }
        }
        else
        {
            while (pEntry != nullptr)
            {
                forthop op = *pEntry;
                forthOpType opType = FORTH_OP_TYPE(op);
                uint32_t opVal = FORTH_OP_VALUE(op);

                switch (opType)
                {

                case kOpNative:
                case kOpNativeImmediate:
                case kOpCCode:
                case kOpCCodeImmediate:
                case kOpUserDef:
                case kOpUserDefImmediate:
                case kOpDLLEntryPoint:
                {
                    if (opVal < mProfileOpcodeCounts.size())
                    {
                        opcodeProfileInfo& opInfo = mProfileOpcodeCounts[opVal];
                        if ((opInfo.op == op) || (opInfo.op == 0))
                        {
                            opInfo.pEntry = pEntry;
                            opInfo.pVocabulary = pVocab;
                        }
                    }
                }
                break;

                default:
                    break;
                }

                pEntry = pVocab->NextEntrySafe(pEntry);
            }
        }
        pVocab = pVocab->GetNextChainVocabulary();
    }

    for (int i = 0; i < mProfileOpcodeCounts.size(); ++i)
    {
        opcodeProfileInfo& opInfo = mProfileOpcodeCounts[i];
        forthop op = opInfo.op;
        if (op == 0)
        {
            op = i;
        }

        Vocabulary *pVocab = opInfo.pVocabulary;
        const char* pVocabName = (pVocab != nullptr) ? pVocab->GetName() : "UNKNOWN_VOCABULARY";

        const char* pOpName = "UNKNOWN_OP";
        forthop* pEntry = opInfo.pEntry;
        if ((pEntry != nullptr) && (pVocab != nullptr))
        {
            // the symbol name in the vocabulary doesn't always have a terminating null
            int len = pVocab->GetEntryNameLength(pEntry);
            if (len > (sizeof(opBuffer) - 1))
            {
                len = sizeof(opBuffer) - 1;
            }
            char *pBuffer = &(opBuffer[0]);
            const char* pName = pVocab->GetEntryName(pEntry);
            for (int i = 0; i < len; i++)
            {
                *pBuffer++ = *pName++;
            }
            *pBuffer = '\0';
            pOpName = &(opBuffer[0]);
        }
        SNPRINTF(buffer, sizeof(buffer), "%s:%s %d\n", pVocabName, pOpName, opInfo.count);
        ForthConsoleStringOut(mpCore, buffer);
    }
    
    for (int i = 0; i < 256; ++i)
    {
        if ((i <= kOpLocalUserDefined) && (opTypeNames[i] != nullptr))
        {
            SNPRINTF(buffer, sizeof(buffer), "opType:%s %d\n", opTypeNames[i], mProfileOpcodeTypeCounts[i]);
            ForthConsoleStringOut(mpCore, buffer);
        }
        else
        {
            if (mProfileOpcodeTypeCounts[i] != 0)
            {
                SNPRINTF(buffer, sizeof(buffer), "opType:0x%x %d\n", i, mProfileOpcodeTypeCounts[i]);
                ForthConsoleStringOut(mpCore, buffer);
            }
        }
    }
}

void Engine::ResetExecutionProfile()
{
    for (int i = 0; i < mProfileOpcodeCounts.size(); ++i)
    {
        mProfileOpcodeCounts[i].count = 0;
        mProfileOpcodeCounts[i].op = 0;
        mProfileOpcodeCounts[i].pEntry = nullptr;
        mProfileOpcodeCounts[i].pVocabulary = nullptr;
    }

    for (int i = 0; i < 256; ++i)
    {
        mProfileOpcodeTypeCounts[i] = 0;
    }
}


//
// FullyExecuteOp is used by the Outer Interpreter (OuterInterpreter::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
OpResult Engine::FullyExecuteOp(CoreState* pCore, forthop opCode)
{
    forthop opScratch[2];

	opScratch[0] = opCode;
	opScratch[1] = gCompiledOps[OP_DONE];
	OpResult exitStatus = ExecuteOps(pCore, &(opScratch[0]));
	if (exitStatus == OpResult::kYield)
	{
		SetError(ForthError::kIllegalOperation, " yield not allowed in FullyExecuteOp");
	}

	return exitStatus;
}

//
// ExecuteOp executes a single op.  If the op is a user-defined op or method, only the
//   very first op is executed before returning.
//
OpResult Engine::ExecuteOp(CoreState* pCore, forthop opCode)
{
#ifdef ASM_INNER_INTERPRETER
    OpResult exitStatus = InterpretOneOpFast(pCore, opCode);
#else
    OpResult exitStatus = InterpretOneOp(pCore, opCode);
#endif
	return exitStatus;
}

//
// ExecuteOps executes a sequence of forth ops.
// code at pOps must be terminated with OP_DONE
//
OpResult Engine::ExecuteOps(CoreState* pCore, forthop *pOps)
{
    forthop *savedIP;

    savedIP = pCore->IP;
    pCore->IP = pOps;
    Fiber* pFiber = (Fiber*)(pCore->pFiber);
    pFiber->GetParent()->InnerLoop();

    OpResult exitStatus = (OpResult)pCore->state;

	pCore->IP = savedIP;
    if ( exitStatus == OpResult::kDone )
    {
		pCore->state = OpResult::kOk;
        exitStatus = OpResult::kOk;
    }
    return exitStatus;
}

OpResult Engine::FullyExecuteMethod(CoreState* pCore, ForthObject& obj, int methodNum)
{
    forthop opScratch[2];
    forthop opCode = obj->pMethods[methodNum];

	RPUSH(((cell)GET_TP));
	SET_THIS(obj);

	opScratch[0] = opCode;
	opScratch[1] = gCompiledOps[OP_DONE];
	OpResult exitStatus = ExecuteOps(pCore, &(opScratch[0]));

	if (exitStatus == OpResult::kYield)
	{
		SetError(ForthError::kIllegalOperation, " yield not allowed in FullyExecuteMethod");
	}
	return exitStatus;
}

// TODO: find a better way to do this
extern forthop gObjectDeleteOpcode;

//#define DEBUG_DELETE_OBJECT 1
#ifdef DEBUG_DELETE_OBJECT
static int deleteDepth = 0;
#endif
OpResult Engine::DeleteObject(CoreState* pCore, ForthObject& obj)
{
    //
    // iterate up the class inheritance chain, calling the delete method at each step
    // don't call delete method if it is Object::delete, since that just deallocates the
    //  memory for the whole object, and that is done directly at the end of this method
    //
    // NOTE: as we move up the inheritance chain, we will overwrite the methods pointer
    // of the object being deleted at each step.
    //
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
    OpResult exitStatus = OpResult::kOk;
    forthop opScratch[2];
    opScratch[1] = gCompiledOps[OP_DONE];

#ifdef DEBUG_DELETE_OBJECT
    printf("*** DELETING %s @%p size %d  depth:%d  rdepth:%d\n", pClassObject->pVocab->GetName(), obj, objSize,
        deleteDepth, GET_RDEPTH/8);
    deleteDepth++;
#endif
    forthop* savedMethods = obj->pMethods;
    while (exitStatus == OpResult::kOk)
    {
        forthop opCode = obj->pMethods[kMethodDelete];

        if (opCode != gObjectDeleteOpcode)
        {
#ifdef DEBUG_DELETE_OBJECT
            printf("executing %s.delete op 0x%x\n", pClassObject->pVocab->GetName(), opCode);
#endif

            void* oldRP = GET_RP;
            RPUSH(((cell)GET_TP));
            SET_THIS(obj);

            opScratch[0] = opCode;
            OpResult exitStatus = ExecuteOps(pCore, &(opScratch[0]));

            if (exitStatus == OpResult::kYield)
            {
                SetError(ForthError::kIllegalOperation, " yield not allowed in delete!");
            }
        }
        else
        {

#ifdef DEBUG_DELETE_OBJECT
            printf("skipping %s op 0x%x\n", pClassObject->pVocab->GetName(), opCode);
#endif
        }

        ClassVocabulary* parentVocabulary = pClassObject->pVocab->ParentClass();
        if (parentVocabulary == nullptr)
        {
            // we've reached the top of the inheritance chain, exit loop
            break;
        }
        pClassObject = parentVocabulary ? parentVocabulary->GetClassObject() : nullptr;
        obj->pMethods = parentVocabulary->GetMethods();
    }
    // is there any reason to restore the methods pointer here, since it is about to be deleted?
    //obj->pMethods = savedMethods;

    // now free the storage for the object instance
    int objSize = pClassObject->pVocab->GetSize();
    DEALLOCATE_BYTES(obj, objSize);

    //deleteDepth--;
    return exitStatus;
}

void Engine::ReleaseObject(CoreState* pCore, ForthObject& inObject)
{
    // TODO: why isn't this just a ForthObject?
    oOutStreamStruct* pObjData = reinterpret_cast<oOutStreamStruct*>(inObject);
#if defined(ATOMIC_REFCOUNTS)
    if (pObjData->refCount.fetch_sub(1) == 1)
    {
        DeleteObject(pCore, inObject);
        inObject = nullptr;
    }
#else
    if (pObjData->refCount > 1)
    {
        --pObjData->refCount;
    }
    else
    {
        DeleteObject(pCore, inObject);
        inObject = nullptr;
    }
#endif
}


void Engine::AddErrorText( const char *pString )
{
    strcat( mpErrorString, pString );
}

void Engine::SetError( ForthError e, const char *pString )
{
    mpCore->error = e;
    if ( pString )
    {
	    strcat( mpErrorString, pString );
    }

    if ( e == ForthError::kNone )
    {
        // previous error state is being cleared
        mpErrorString[0] = '\0';
    }
    else
    {
        mpCore->state = OpResult::kError;
    }
}

void Engine::SetFatalError( ForthError e, const char *pString )
{
    mpCore->state = OpResult::kFatalError;
    mpCore->error = e;
    if ( pString )
    {
        strcpy( mpErrorString, pString );
    }
}

void Engine::GetErrorString( char *pBuffer, int bufferSize )
{
    int errorNum = (int) mpCore->error;
    if ( errorNum < (sizeof(pErrorStrings) / sizeof(char *)) )
    {
        if ( mpErrorString[0] != '\0' )
        {
            sprintf( pBuffer, "%s: %s", pErrorStrings[errorNum], mpErrorString );
        }
        else
        {
            strcpy( pBuffer, pErrorStrings[errorNum] );
        }
    }
    else
    {
        sprintf( pBuffer, "Unknown Error %d", errorNum );
    }
}


OpResult Engine::CheckStacks( void )
{
    cell depth;
    OpResult result = OpResult::kOk;

    // check parameter stack for over/underflow
    depth = mpCore->ST - mpCore->SP;
    if ( depth < 0 )
    {
        SetError( ForthError::kParamStackUnderflow );
        result = OpResult::kError;
    }
    else if ( depth >= (int32_t) mpCore->SLen )
    {
        SetError( ForthError::kParamStackOverflow );
        result = OpResult::kError;
    }
    
    // check return stack for over/underflow
    depth = mpCore->RT - mpCore->RP;
    if ( depth < 0 )
    {
        SetError( ForthError::kReturnStackUnderflow );
        result = OpResult::kError;
    }
    else if ( depth >= (int32_t) mpCore->RLen )
    {
        SetError( ForthError::kReturnStackOverflow );
        result = OpResult::kError;
    }

    return result;
}


void Engine::SetDefaultConsoleOut( ForthObject& newOutStream )
{
	SPEW_SHELL("SetDefaultConsoleOut pCore=%p  pMethods=%p  pData=%p\n", mpCore, newOutStream->pMethods, newOutStream);
    OBJECT_ASSIGN(mpCore, mDefaultConsoleOutStream, newOutStream);
}

void Engine::SetConsoleOut(CoreState* pCore, ForthObject& newOutStream)
{
    SPEW_SHELL("SetConsoleOut pCore=%p  pMethods=%p  pData=%p\n", pCore, newOutStream->pMethods, newOutStream);
    OBJECT_ASSIGN(pCore, pCore->consoleOutStream, newOutStream);
}

void Engine::SetErrorOut(CoreState* pCore, ForthObject& newOutStream)
{
    SPEW_SHELL("SetErrorOut pCore=%p  pMethods=%p  pData=%p\n", pCore, newOutStream->pMethods, newOutStream);
    OBJECT_ASSIGN(pCore, mErrorOutStream, newOutStream);
}

void* Engine::GetErrorOut(CoreState* pCore)
{
    return mErrorOutStream;
}

void Engine::PushConsoleOut( CoreState* pCore )
{
	PUSH_OBJECT( pCore->consoleOutStream );
}

void Engine::PushDefaultConsoleOut( CoreState* pCore )
{
	PUSH_OBJECT( mDefaultConsoleOutStream );
}

void Engine::PushErrorOut(CoreState* pCore)
{
    PUSH_OBJECT(mErrorOutStream);
}

void Engine::ResetConsoleOut( CoreState& core )
{
	// TODO: there is a dilemma here - either we just replace the current output stream
	//  without doing a release, and possibly leak a stream object, or we do a release
	//  and risk a crash, since ResetConsoleOut is called when an error is detected,
	//  so the object we are releasing may already be deleted or otherwise corrupted.
    CLEAR_OBJECT(core.consoleOutStream);
    OBJECT_ASSIGN(&core, core.consoleOutStream, mDefaultConsoleOutStream);
}

void Engine::ResetConsoleOut()
{
    ResetConsoleOut(*mpCore);
}


void Engine::ConsoleOut( const char* pBuff )
{
    ForthConsoleStringOut( mpCore, pBuff );
}


int32_t Engine::GetTraceFlags( void )
{
	return mpCore->traceFlags;
}

void Engine::SetTraceFlags( int32_t flags )
{
	mpCore->traceFlags = flags;
}

// return milliseconds since engine was created
uint32_t Engine::GetElapsedTime( void )
{
	uint32_t millisecondsElapsed = 0;
#if defined(WIN32)
#if defined(MSDEV)
	struct __timeb32 now;

	_ftime32_s( &now );
	__time32_t seconds = now.time - mStartTime.time;
    __time32_t milliseconds = now.millitm - mStartTime.millitm;
	millisecondsElapsed =  (uint32_t) ((seconds * 1000) + milliseconds);
#else
	struct _timeb now;
    _ftime( &now );

    int32_t seconds = now.time - mStartTime.time;
    int32_t milliseconds = now.millitm - mStartTime.millitm;
	millisecondsElapsed = (uint32_t) ((seconds * 1000) + milliseconds);
#endif
#elif defined(LINUX)
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
    int32_t seconds = now.tv_sec - mStartTime.tv_sec;
    int32_t milliseconds = (now.tv_nsec - mStartTime.tv_nsec) / 1000000;
	millisecondsElapsed = (uint32_t) ((seconds * 1000) + milliseconds);
#else
	struct timeb now;
    ftime( &now );

    int32_t seconds = now.time - mStartTime.time;
    int32_t milliseconds = now.millitm - mStartTime.millitm;
	millisecondsElapsed = (uint32_t) ((seconds * 1000) + milliseconds);
#endif
	return millisecondsElapsed;
}


void Engine::DumpCrashState()
{
	char buff[256];

    cell* pSP = mpCore->SP;
	if ( (pSP < mpCore->ST) && (pSP > mpCore->SB) )
	{
		cell numToShow = 64;
		cell depth = mpCore->ST - pSP;
		if ( depth < numToShow )
		{
			numToShow = depth;
		}
		for ( int i = 0; i < numToShow; i++ )
		{
#ifdef FORTH64
			SNPRINTF( buff, sizeof(buff), "S[%2d] 0x%16llx  %lld\n", depth - (i + 1), pSP[i], pSP[i] );
#else
            SNPRINTF(buff, sizeof(buff), "S[%2d] 0x%08x  %d\n", depth - (i + 1), pSP[i], pSP[i]);
#endif
			ConsoleOut( buff );
		}
	}

	SNPRINTF( buff, sizeof(buff), "\n   IP 0x%08x   ", mpCore->IP );
	ConsoleOut( buff );
	DisplayUserDefCrash( mpCore->IP, buff, sizeof(buff) );
	ConsoleOut( "\n" );

	cell* pRP = mpCore->RP;
	if ( (pRP < mpCore->RT) && (pRP > mpCore->RB) )
	{
        cell numToShow = 64;
		cell depth = mpCore->RT - pRP;
		cell* pFP = mpCore->FP;
		if ( depth < numToShow )
		{
			numToShow = depth;
		}
		for ( cell i = 0; i < numToShow; i++ )
		{
            cell rVal = pRP[i];
			forthop* pRVal = (forthop *) rVal;
#if defined(FORTH64)
            SNPRINTF(buff, sizeof(buff), "R[%2d] 0x%016llx   ", depth - (i + 1), rVal);
#else
            SNPRINTF(buff, sizeof(buff), "R[%2d] 0x%08x   ", depth - (i + 1), rVal);
#endif
			ConsoleOut( buff );
			if ( (pRP + i) == pFP )
			{
				ConsoleOut( "<FP>" );
				pFP = (cell *)pRVal;
			}
			else
			{
				DisplayUserDefCrash( pRVal, buff, sizeof(buff) );
			}
			ConsoleOut( "\n" );
		}
	}

}

void Engine::DisplayUserDefCrash( forthop *pRVal, char* buff, int buffSize )
{
	if ( (pRVal >= mDictionary.pBase) && (pRVal < mDictionary.pCurrent) )
	{
        forthop* pDefBase = nullptr;
		Vocabulary* pVocab = Vocabulary::GetVocabularyChainHead();
        forthop* pClosestIP = nullptr;
        forthop* pFoundClosest = nullptr;
		Vocabulary* pFoundVocab = nullptr;
		while ( pVocab != nullptr)
		{
            forthop* pClosest = mpOuter->FindUserDefinition( pVocab, pClosestIP, pRVal, pDefBase );
			if ( pClosest != nullptr)
			{
				pFoundClosest = pClosest;
				pFoundVocab = pVocab;
			}
			pVocab = pVocab->GetNextChainVocabulary();
		}

		if ( pFoundClosest != nullptr)
		{
			SNPRINTF( buff, buffSize, "%s:", pFoundVocab->GetName() );
			ConsoleOut( buff );
			const char* pName = pFoundVocab->GetEntryName( pFoundClosest );
			int len = (int) pName[-1];
			for ( int i = 0; i < len; i++ )
			{
				buff[i] = pName[i];
			}
			buff[len] = '\0';
			ConsoleOut( buff );
			SNPRINTF(buff, buffSize, " + 0x%04x", (int)(pRVal - pDefBase));
		}
		else
		{
			strcpy( buff, "*user def* " );
		}
	}
	else
	{
		SNPRINTF( buff, buffSize, "%p", pRVal );
	}
	ConsoleOut( buff );
}

// this was an inline, but sometimes that returned the wrong value for unknown reasons
CoreState*	Engine::GetCoreState( void )
{
	return mpCore;
}


BlockFileManager* Engine::GetBlockFileManager()
{
    return mBlockFileManager;
}


bool Engine::IsServer() const
{
	return mIsServer;
}

void Engine::SetIsServer(bool isServer)
{
	mIsServer = isServer;
}

ClassVocabulary* Engine::AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry* pEntries)
{
    return mpOuter->AddBuiltinClass(pClassName, classIndex, parentClassIndex, pEntries);
}

void Engine::RaiseException(CoreState *pCore, cell newExceptionNum)
{
    char errorMsg[64];
    ForthExceptionFrame *pExceptionFrame = pCore->pExceptionFrame;
    forthop* pHandlerOffsets = pExceptionFrame->pHandlerOffsets;
    if (pExceptionFrame != nullptr)
    {
        // exception frame:
        //  0   old exception frame ptr
        //  1   saved pstack ptr
        //  2   ptr to catchIPOffset,finallyIPOffset
        //  3   saved frame ptr
        //  4   exception number
        //  5   exception state
        if (newExceptionNum)
        {
            cell oldExceptionNum = pExceptionFrame->exceptionNumber;
            pExceptionFrame->exceptionNumber = newExceptionNum;
            if (oldExceptionNum)
            {
                if (pExceptionFrame->exceptionState == ExceptionState::kFinally)
                {
                    // exception inside a finally section, avoid infinite loop
                    // ? should this be a reraise to surrounding handler instead
                    snprintf(errorMsg, sizeof(errorMsg), "Reraised exception of type %d in finally section", (int)newExceptionNum);
                    SetError(ForthError::kException, errorMsg);
                }
                else
                {
                    // re-raise inside an exception handler - transfer control to finally body
                    SET_SP(pExceptionFrame->pSavedSP);
                    SET_IP(pHandlerOffsets + pHandlerOffsets[1]);
                }
            }
            else
            {
                // raise in try body
                SET_SP(pExceptionFrame->pSavedSP);
                SPUSH(newExceptionNum);
                SET_IP(pHandlerOffsets + pHandlerOffsets[0]);
                pExceptionFrame->exceptionState = ExceptionState::kExcept;
            }
        }
        pExceptionFrame->exceptionNumber = newExceptionNum;
    }
    else
    {
        if (newExceptionNum)
        {
            snprintf(errorMsg, sizeof(errorMsg), "Unhandled exception of type %d", (int)newExceptionNum);
            SetError(ForthError::kException, errorMsg);
        }
    }
}


void Engine::AddOpNameForTracing(const char* pName)
{
    if ((mpCore->numOps - 1) < NUM_TRACEABLE_OPS)
    {
        gOpNames[mpCore->numOps - 1] = pName;
    }
}

void Engine::Interrupt()
{
    // user pressed Ctrl-C
    FiberState ff;
    Fiber* mainFiber = mpMainThread->GetFiber(0);
    mainFiber->GetCore()->state = OpResult::kInterrupted;
    mainFiber->SetRunState(FiberState::kStopped);
}

//############################################################################
