//////////////////////////////////////////////////////////////////////
//
// ForthEngine.cpp: implementation of the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(LINUX) || defined(MACOSX)
#include <ctype.h>
#include <stdarg.h>
#include <sys/mman.h>
#endif

#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthVocabulary.h"
#include "ForthInner.h"
#include "ForthExtension.h"
#include "ForthStructs.h"
#include "ForthOpcodeCompiler.h"
#include "ForthPortability.h"
#include "ForthBuiltinClasses.h"
#include "ForthBlockFileManager.h"
#include "ForthParseInfo.h"

extern "C"
{

    extern baseDictionaryEntry baseDictionary[];
	extern void AddForthOps( ForthEngine* pEngine );
#ifdef ASM_INNER_INTERPRETER
    extern void InitAsmTables(  ForthCoreState *pCore );
#endif
    extern OpResult InnerInterp( ForthCoreState *pCore );
    extern void consoleOutToFile( ForthCoreState   *pCore,  const char       *pMessage );
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

	ForthEngine* pEngine = ForthEngine::GetInstance();
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
	void traceOp(ForthCoreState* pCore, forthop* pIP, forthop op)
	{
		if (pCore->traceFlags != 0)
		{
		    ForthEngine* pEngine = (ForthEngine *)(pCore->pEngine);
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

ForthEngine* ForthEngine::mpInstance = nullptr;

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
	"LocalRefOpCombo", "MemberRefOpCombo", "MethodWithSuper", "LocalUserDefined"
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
//                     TokenStack
// 

TokenStack::TokenStack()
	: mpCurrent(nullptr)
	, mpBase(nullptr)
	, mpLimit(nullptr)
	, mNumBytes(0)
{
}

TokenStack::~TokenStack()
{
	if (mpCurrent != nullptr)
	{
		__FREE(mpBase);
	}
}

void TokenStack::Initialize(uint32_t numBytes)
{
	mpBase = (char *)__MALLOC(numBytes);
	mpLimit = mpBase + numBytes;
	mpCurrent = mpLimit;
	mNumBytes = numBytes;
}

void TokenStack::Push(const char* pToken)
{
	size_t newTokenBytes = strlen(pToken) + 1;
	char* newCurrent = mpCurrent - newTokenBytes;
	if (newCurrent >= mpBase)
	{
		mpCurrent = newCurrent;
	}
	else
	{
		// resize stack to fit pushed token plus a little bit
		size_t currentBytes = mpLimit - mpCurrent;
		mNumBytes = newTokenBytes + 64 + (mNumBytes - currentBytes);
		mpBase = (char *)__REALLOC(mpBase, mNumBytes);
		mpLimit = mpBase + mNumBytes;
		mpCurrent = mpLimit - (currentBytes + newTokenBytes);
	}
	memcpy(mpCurrent, pToken, newTokenBytes);
}

char* TokenStack::Pop()
{
	char* pResult = nullptr;
	if (mpCurrent != mpLimit)
	{
		pResult = mpCurrent;
		size_t bytesToRemove = strlen(mpCurrent) + 1;
		mpCurrent += bytesToRemove;
	}

	return pResult;
}

char* TokenStack::Peek()
{
	char* pResult = nullptr;
	if (mpCurrent != mpLimit)
	{
		pResult = mpCurrent;
	}

	return pResult;
}

void TokenStack::Clear()
{
	mpCurrent = mpLimit;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     OuterInterpreter
// 

OuterInterpreter::OuterInterpreter(ForthEngine* pEngine)
    : mpEngine(pEngine)
    , mpForthVocab(nullptr)
    , mpLiteralsVocab(nullptr)
    , mpLocalVocab(nullptr)
    , mpDefinitionVocab(nullptr)
    , mpStringBufferA(nullptr)
    , mpStringBufferANext(nullptr)
    , mStringBufferASize(0)
    , mpTempBuffer(nullptr)
    , mpTempBufferLock(nullptr)
    , mpInterpreterExtension(nullptr)
    , mNumElements(0)
    , mpTypesManager(nullptr)
    , mpVocabStack(nullptr)
    , mpOpcodeCompiler(nullptr)
    , mFeatures(kFFCCharacterLiterals | kFFMultiCharacterLiterals | kFFCStringLiterals
        | kFFCHexLiterals | kFFDoubleSlashComment | kFFCFloatLiterals | kFFParenIsExpression
        | kFFAllowVaropSuffix)
    , mContinuationIx(0)
    , mContinueDestination(nullptr)
    , mContinueCount(0)
    , mpNewestEnum(nullptr)
    , mLocalFrameSize(0)
    , mNextEnum(0)
    , mpLastToken(nullptr)
    , mpLocalAllocOp(nullptr)
    , mCompileState(0)
    , mCompileFlags(0)
{
    mpShell = mpEngine->GetShell();
    mpCore = mpEngine->GetMainFiber()->GetCore();
    mpDictionary = mpEngine->GetDictionaryMemorySection();

    mTokenStack.Initialize(4);

    mpOpcodeCompiler = new ForthOpcodeCompiler(mpEngine->GetDictionaryMemorySection());

    if (mpTypesManager == nullptr)
    {
        mpTypesManager = new ForthTypesManager();
    }

    mpForthVocab = new ForthVocabulary("forth", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpLiteralsVocab = new ForthVocabulary("literals", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpLocalVocab = new ForthLocalVocabulary("locals", NUM_LOCALS_VOCAB_VALUE_LONGS);
    mStringBufferASize = 3 * MAX_STRING_SIZE;
    mpStringBufferA = new char[mStringBufferASize];
    mpTempBuffer = new char[MAX_STRING_SIZE];

    mpDefinitionVocab = mpForthVocab;
}

void OuterInterpreter::Initialize()
{
    mpVocabStack = new ForthVocabularyStack;
    mpVocabStack->Initialize();
    mpVocabStack->Clear();

}

OuterInterpreter::~OuterInterpreter()
{
    delete mpForthVocab;
    delete mpLiteralsVocab;
    delete mpLocalVocab;
    delete mpOpcodeCompiler;
    delete[] mpStringBufferA;
    delete[] mpTempBuffer;

    if (mpTempBufferLock != nullptr)
    {
#ifdef WIN32
        DeleteCriticalSection(mpTempBufferLock);
#else
        pthread_mutex_destroy(mpTempBufferLock);
#endif
        delete mpTempBufferLock;
    }

    delete mpVocabStack;

    if (mpTypesManager != nullptr)
    {
        mpTypesManager->ShutdownBuiltinClasses(mpEngine);
        delete mpTypesManager;
    }

    CleanupGlobalObjectVariables(nullptr);
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthEngine
// 

ForthEngine::ForthEngine()
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
    // is fairly screwed up, it is becauses originally ForthEngine was the center of the universe,
    // and it created the shell, but now the shell is created first, and the shell or the main app
    // can create the engine, and then the shell calls OuterInterpreter::Initialize to hook the two up.
    // The main thread needs to get the file interface from the shell, so it can't be created until
    // after the engine is connected to the shell.  Did I mention its screwed up?
}

ForthEngine::~ForthEngine()
{
    if ( mpExtension != nullptr)
    {
        mpExtension->Shutdown();
    }
	
    delete mpOuter;

    // delete all thread and fiber objects
    ForthThread* pThread = mpThreads;
    while (pThread != nullptr)
    {
        ForthThread* pNextThread = pThread->mpNext;
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

    ForthForgettable* pForgettable = ForthForgettable::GetForgettableChainHead();
    while ( pForgettable != nullptr )
    {
        ForthForgettable* pNextForgettable = pForgettable->GetNextForgettable();
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
		ForthThread *pNextThread = pThread->mpNext;
		delete pThread;
		pThread = pNextThread;
    }

    delete mpEngineScratch;

    delete mBlockFileManager;

    mpInstance = nullptr;
}

ForthEngine*
ForthEngine::GetInstance( void )
{
    ASSERT( mpInstance != nullptr);
    return mpInstance;
}

//############################################################################
//
//    system initialization
//
//############################################################################

void ForthEngine::Initialize(
    ForthShell*        pShell,
    int                totalLongs,
    bool               bAddBaseOps,
    ForthExtension*    pExtension )
{
    mpShell = pShell;

    mBlockFileManager = new ForthBlockFileManager(mpShell->GetBlockfilePath());

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
ForthEngine::ToggleFastMode( void )
{
    SetFastMode(!mFastMode );
}

bool
ForthEngine::GetFastMode( void )
{
    return mFastMode;
}

void
ForthEngine::SetFastMode( bool goFast )
{
    mFastMode = goFast;
}

void OuterInterpreter::Reset(void)
{
    mpVocabStack->Clear();

    mpStringBufferANext = mpStringBufferA;

    mpOpcodeCompiler->Reset();
    mCompileState = 0;
    mCompileFlags = 0;
    ResetContinuations();
    mpNewestEnum = nullptr;

    mTokenStack.Clear();

#ifdef WIN32
    if (mpTempBufferLock != nullptr)
    {
        DeleteCriticalSection(mpTempBufferLock);
        delete mpTempBufferLock;
    }
    mpTempBufferLock = new CRITICAL_SECTION();
    InitializeCriticalSection(mpTempBufferLock);
#else
    if (mpTempBufferLock != nullptr)
    {
        pthread_mutex_destroy(mpTempBufferLock);
        delete mpTempBufferLock;
    }
    mpTempBufferLock = new pthread_mutex_t;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(mpTempBufferLock, &mutexAttr);

    pthread_mutexattr_destroy(&mutexAttr);
#endif
}

void ForthEngine::Reset( void )
{
    mpOuter->Reset();

    if (mpExtension != nullptr)
    {
        mpExtension->Reset();
    }
}

void ForthEngine::ErrorReset( void )
{
    Reset();
	mpMainThread->Reset();
}

cell* ForthEngine::GetCompileStatePtr(void)
{
    return mpOuter->GetCompileStatePtr();
}

void ForthEngine::SetCompileState(cell v)
{
    mpOuter->SetCompileState(v);
}

cell ForthEngine::IsCompiling(void)
{
    return mpOuter->IsCompiling();
}


// add an op to engine dispatch table
forthop OuterInterpreter::AddOp( const void *pOp )
{
    forthop newOp = (forthop)(mpCore->numOps);

    if ( mpCore->numOps == mpCore->maxOps )
    {
        mpCore->maxOps += 128;
        void* pNewBlock = realloc( mpCore->ops, sizeof(forthop *) * mpCore->maxOps );
        if (pNewBlock != nullptr)
        {
            // avoid a compiler warning
            mpCore->ops = (forthop**)pNewBlock;
        }
    }
    mpCore->ops[ mpCore->numOps++ ] = (forthop *) pOp;

	return newOp;
}


// add an op to dictionary and corresponding symbol to current vocabulary
forthop OuterInterpreter::AddUserOp( const char *pSymbol, forthop** pEntryOut, bool smudgeIt )
{
    mpEngine->AlignDP();
    forthop newestOp = AddOp(mpDictionary->pCurrent);
    newestOp = COMPILED_OP(kOpUserDef, newestOp);
    forthop* pEntry = mpDefinitionVocab->AddSymbol(pSymbol, newestOp);
	if (pEntryOut != nullptr)
	{
		*pEntryOut = pEntry;
	}

    if (smudgeIt)
    {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return newestOp;
}

void OuterInterpreter::AddBuiltinClasses()
{
    mpTypesManager->AddBuiltinClasses(this);
}

forthop* OuterInterpreter::AddBuiltinOp(const char* name, uint32_t flags, void* value)
{
    forthop newestOp = AddOp(value);
    newestOp = COMPILED_OP(flags, newestOp);
    // AddSymbol will call OuterInterpreter::AddOp to add the operators to op table
    forthop *pEntry = mpDefinitionVocab->AddSymbol(name, newestOp);

//#ifdef TRACE_INNER_INTERPRETER
    // add built-in op names to table for TraceOp
	ucell index = mpCore->numOps - 1;
    if (index < NUM_TRACEABLE_OPS)
    {
        gOpNames[index] = name;
    }
	mpCore->numBuiltinOps = mpCore->numOps;
//#endif
	return pEntry;
}


void
OuterInterpreter::AddBuiltinOps( baseDictionaryEntry *pEntries )
{
    // I think this assert is a holdover from when userOps and builtinOps were in a single dispatch table
    // assert if this is called after any user ops have been defined
    //ASSERT( mpCore->numUserOps == 0 );

    while ( pEntries->value != nullptr )
    {

        AddBuiltinOp( pEntries->name, (uint32_t)(pEntries->flags), pEntries->value);
        pEntries++;
		//mpCore->numOps++;
    }
}


ForthClassVocabulary*
OuterInterpreter::StartClassDefinition(const char* pClassName, eBuiltinClassIndex classIndex)
{
    SetFlag( kEngineFlagInStructDefinition );
    SetFlag( kEngineFlagInClassDefinition );
	
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	ForthClassVocabulary* pVocab = pManager->StartClassDefinition(pClassName, classIndex);

	// add new class vocab to top of search order
	mpVocabStack->DupTop();
	mpVocabStack->SetTop( pVocab );

    CompileBuiltinOpcode( OP_DO_CLASS_TYPE );
    CompileCell((cell) pVocab);

    return pVocab;
}

void
OuterInterpreter::EndClassDefinition()
{
	ClearFlag( kEngineFlagInStructDefinition );
    ClearFlag( kEngineFlagInClassDefinition );

    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	pManager->EndClassDefinition();
	mpVocabStack->DropTop();
}

ForthClassVocabulary*
OuterInterpreter::AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry *pEntries)
{
    // do "class:" - define class subroutine
	ForthClassVocabulary* pVocab = StartClassDefinition(pClassName, classIndex);
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	ForthClassVocabulary* pParentClass = pManager->GetClassVocabulary(parentClassIndex);

    if ( pParentClass )
    {
        // do "extends" - tie into parent class
        pManager->GetNewestClass()->Extends( pParentClass );
    }

    // loop through pEntries, adding ops to builtinOps table and adding methods to class
    while ( pEntries->name != nullptr )
    {
        const char* pMemberName = pEntries->name;
        uint32_t entryType = (uint32_t)(pEntries->returnType);
        if (CODE_IS_METHOD(entryType))
        {
            if ( !strcmp( pMemberName, "__newOp" ) )
            {
                // this isn't a new method, it is the class constructor op
                forthop* pEntry = AddBuiltinOp( pMemberName, kOpCCode, pEntries->value );
				pEntry[1] = (uint32_t)BaseType::kUserDefinition;
                pVocab->GetClassObject()->newOp = *pEntry;
            }
            else
            {
                // this entry is a member method
                // add method routine to builtinOps table
                int32_t methodOp = gCompiledOps[OP_BAD_OP];
                // do "method:"
                int32_t methodIndex = pVocab->FindMethod( pMemberName );
                StartOpDefinition( pMemberName, false );
                forthop* pEntry = pVocab->GetNewestEntry();
                methodOp = FORTH_OP_VALUE(*pEntry);
                methodOp = COMPILED_OP(kOpCCode, methodOp);
                if (pEntries->value != nullptr)
                {
                    if ((mpCore->numOps - 1) < NUM_TRACEABLE_OPS)
                    {
                        gOpNames[mpCore->numOps - 1] = pMemberName;
                    }
                }
                // pEntry[0] is initially the opcode for the method, now we replace it with the method index,
                //  and put the opcode in the method table
				methodIndex = pVocab->AddMethod( pMemberName, methodIndex, methodOp );
                pEntry[0] = methodIndex;
                pEntry[1] = (uint32_t)(pEntries->returnType);
                mpCore->ops[mpCore->numOps - 1] = (forthop *)(pEntries->value);

                SPEW_ENGINE( "Method %s op is 0x%x\n", pMemberName, methodOp );

                // do ";method"
                EndOpDefinition( false );
            }
        }
        else
        {
            BaseType baseType = CODE_TO_BASE_TYPE(entryType);
            if (baseType == BaseType::kUserDefinition)
            {
                // forth op defined within class
                if (CODE_IS_FUNKY(entryType))
                {
                    // class op with precedence
                    forthop* pEntry = AddBuiltinOp(pMemberName, kOpCCodeImmediate, pEntries->value);
                    pEntry[1] = (uint32_t)BaseType::kUserDefinition;
                }
                else
                {
                    // class op
                    forthop* pEntry = AddBuiltinOp(pMemberName, kOpCCode, pEntries->value);
                    pEntry[1] = (uint32_t)BaseType::kUserDefinition;
                }
            }
            else
            {
                // this entry is a member variable
                pManager->GetNewestStruct()->AddField(pMemberName, (uint32_t)(pEntries->returnType),
                		static_cast<int>(reinterpret_cast<intptr_t>(pEntries->value)));
            }
        }

#ifdef TRACE_INNER_INTERPRETER
        // add built-in op names to table for TraceOp
        if ( (mpCore->numOps - 1) < NUM_TRACEABLE_OPS )
        {
            gOpNames[mpCore->numOps - 1] = pEntries->name;
        }
#endif
        pEntries++;
    }

    // do ";class"
    ClearFlag( kEngineFlagInStructDefinition );
    pManager->EndClassDefinition();

    return pVocab;
}


// forget the specified op and all higher numbered ops, and free the memory where those ops were stored
void
OuterInterpreter::ForgetOp(forthop opNumber, bool quietMode )
{
    if ( opNumber < mpCore->numOps )
    {
        forthop* pNewDP = mpCore->ops[opNumber];
        CleanupGlobalObjectVariables(pNewDP);
        mpDictionary->pCurrent = pNewDP;
        mpCore->numOps = opNumber;
    }
    else
    {
        if ( !quietMode )
        {
            SPEW_ENGINE( "OuterInterpreter::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, (int)mpCore->numOps );
            printf( "OuterInterpreter::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, (int)mpCore->numOps );
        }
    }

    ForthExtension* pExtension = mpEngine->GetExtension();
    if (pExtension != nullptr)
    {
        pExtension->ForgetOp( opNumber );
    }
}

// return true if symbol was found
bool OuterInterpreter::ForgetSymbol( const char *pSym, bool quietMode )
{
    forthop *pEntry = nullptr;
    forthop op;
    forthOpType opType;
    bool forgotIt = false;
    char buff[256];
    buff[0] = '\0';

    ForthVocabulary* pFoundVocab = nullptr;
    pEntry = GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pFoundVocab != nullptr )
    {
        op = ForthVocabulary::GetEntryValue( pEntry );
        opType = ForthVocabulary::GetEntryType( pEntry );
		uint32_t opIndex = ((uint32_t)FORTH_OP_VALUE( op ));
        switch ( opType )
        {
            case kOpNative:
            case kOpNativeImmediate:
            case kOpCCode:
            case kOpCCodeImmediate:
            case kOpUserDef:
            case kOpUserDefImmediate:
				if ( opIndex > mpCore->numBuiltinOps )
				{
					ForgetOp( op, quietMode );
					ForthForgettable::ForgetPropagate( mpDictionary->pCurrent, op );
					forgotIt = true;
				}
				else
				{
					// sym is built-in op - no way
					SNPRINTF( buff, sizeof(buff), "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
				}
                break;

            default:
                const char* pStr = mpEngine->GetOpTypeName( opType );
                SNPRINTF( buff, sizeof(buff), "Error - attempt to forget op %s of type %s from %s\n", pSym, pStr, pFoundVocab->GetName() );
                break;

        }
    }
    else
    {
        SNPRINTF( buff, sizeof(buff), "Error - attempt to forget unknown op %s from %s\n", pSym, GetSearchVocabulary()->GetName() );
    }
    if ( buff[0] != '\0' )
    {
        if ( !quietMode )
        {
            printf( "%s", buff );
        }
        SPEW_ENGINE( "%s", buff );
    }
    return forgotIt;
}

void
OuterInterpreter::ShowSearchInfo()
{
	ForthVocabularyStack* pVocabStack = GetVocabularyStack();
	int depth = 0;
	ForthConsoleStringOut(mpCore, "vocab stack:");
	while (true)
	{
		ForthVocabulary* pVocab = pVocabStack->GetElement(depth);
		if (pVocab == nullptr)
		{
			break;
		}
		ForthConsoleCharOut(mpCore, ' ');
		ForthConsoleStringOut(mpCore, pVocab->GetName());
		depth++;
	}
	ForthConsoleCharOut(mpCore, '\n');
	ForthConsoleStringOut(mpCore, "definitions vocab: ");
	ForthConsoleStringOut(mpCore, GetDefinitionVocabulary()->GetName());
	ForthConsoleCharOut(mpCore, '\n');
}

void ForthEngine::ShowMemoryInfo()
{
    std::vector<ForthMemoryStats *> memoryStats;
    int numStorageBlocks;
    int totalStorage;
    int unusedStorage;
    s_memoryManager->getStats(memoryStats, numStorageBlocks, totalStorage, unusedStorage);
    char buffer[256];
    for (ForthMemoryStats* stats : memoryStats)
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

ForthThread * ForthEngine::CreateThread(forthop fiberOp, int paramStackSize, int returnStackSize )
{
	ForthThread *pThread = new ForthThread(this, paramStackSize, returnStackSize);
	ForthFiber *pNewThread = pThread->GetFiber(0);
	pNewThread->SetOp(fiberOp);

    InitCoreState(pNewThread->mCore);

	pThread->mpNext = mpThreads;
	mpThreads = pThread;

	return pThread;
}


void ForthEngine::InitCoreState(ForthCoreState& core)
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


void ForthEngine::DestroyThread(ForthThread *pThread)
{
	ForthThread *pNext, *pCurrent;

    if ( mpThreads == pThread )
    {

        // special case - thread is head of list
		mpThreads = (ForthThread *)(mpThreads->mpNext);
        delete pThread;

    }
    else
    {

        // TODO: this is untested
        pCurrent = mpThreads;
        while ( pCurrent != nullptr )
        {
			pNext = (ForthThread *)(pCurrent->mpNext);
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


char * OuterInterpreter::GetNextSimpleToken( void )
{
	return mTokenStack.IsEmpty() ? mpShell->GetNextSimpleToken() : mTokenStack.Pop();
}


// interpret named file, interpret from standard in if pFileName is nullptr
// return 0 for normal exit
bool
ForthEngine::PushInputFile( const char *pInFileName )
{
    return mpShell->PushInputFile( pInFileName );
}

void
ForthEngine::PushInputBuffer( const char *pDataBuffer, int dataBufferLen )
{
    mpShell->PushInputBuffer( pDataBuffer, dataBufferLen );
}

void
ForthEngine::PushInputBlocks(ForthBlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock)
{
    mpShell->PushInputBlocks(pManager, firstBlock, lastBlock);
}


void
ForthEngine::PopInputStream( void )
{
    mpShell->PopInputStream();
}



forthop *
OuterInterpreter::StartOpDefinition(const char *pName, bool smudgeIt, forthOpType opType, ForthVocabulary* pDefinitionVocab)
{
    mpLocalVocab->Empty();
    mpLocalVocab->ClearFrame();
    //mLocalFrameSize = 0;
    //mpLocalAllocOp = nullptr;
    mpOpcodeCompiler->ClearPeephole();
    mpEngine->AlignDP();

    if (pDefinitionVocab == nullptr)
    {
        pDefinitionVocab = mpDefinitionVocab;
    }

    if ( pName == nullptr )
    {
        pName = GetNextSimpleToken();
    }

    forthop newestOp = AddOp(mpDictionary->pCurrent);
    newestOp = COMPILED_OP(opType, newestOp);
    forthop* pEntry = pDefinitionVocab->AddSymbol(pName, newestOp);
    if ( smudgeIt )
    {
        pDefinitionVocab->SmudgeNewestSymbol();
    }
	mLabels.clear();

    return pEntry;
}


void
OuterInterpreter::EndOpDefinition( bool unsmudgeIt )
{
    forthop* pLocalAllocOp = mpLocalVocab->GetFrameAllocOpPointer();
    if ( pLocalAllocOp != nullptr )
    {
        int nCells = mpLocalVocab->GetFrameCells();
        forthop op = COMPILED_OP( kOpAllocLocals, nCells);
        *pLocalAllocOp = op;
		SPEW_COMPILATION("Backpatching allocLocals 0x%08x @ 0x%08x\n", op, pLocalAllocOp);
		mpLocalVocab->ClearFrame();
    }
    mpOpcodeCompiler->ClearPeephole();
    if ( unsmudgeIt )
    {
        mpDefinitionVocab->UnSmudgeNewestSymbol();
    }
}


forthop*
OuterInterpreter::FindSymbol( const char *pSymName )
{
    ForthVocabulary* pFoundVocab = nullptr;
    return GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
}

void
OuterInterpreter::DescribeOp( const char* pSymName, forthop op, int32_t auxData )
{
    char buff[256];
    char buff2[128];
    char c;
    int line = 1;
    bool notDone = true;

    int32_t opType = FORTH_OP_TYPE( op );
    int32_t opValue = FORTH_OP_VALUE( op );
    bool isUserOp = (opType == kOpUserDef) || (opType == kOpUserDefImmediate);
    const char* pStr = mpEngine->GetOpTypeName( opType );
    if ( isUserOp )
    {
        ForthStructVocabulary::TypecodeToString( auxData, buff2, sizeof(buff2) );
        SNPRINTF( buff, sizeof(buff), "%s: type %s:%x value 0x%x 0x%x (%s) \n", pSymName, pStr, opValue, op, auxData, buff2 );
    }
    else
    {
        SNPRINTF( buff, sizeof(buff), "%s: type %s:%x value 0x%x 0x%x \n", pSymName, pStr, opValue, op, auxData );
    }

    mpEngine->ConsoleOut( buff );
    if ( isUserOp )
    {
        // disassemble the op until IP reaches next newer op
        forthop* curIP = mpCore->ops[ opValue ];
        forthop* baseIP = curIP;
        forthop* endIP = (opValue == (mpCore->numOps - 1)) ? mpEngine->GetDP() : mpCore->ops[ opValue + 1 ];
        if (*curIP == gCompiledOps[OP_DO_ENUM])
        {
            ForthEnumInfo* pEnumInfo = (ForthEnumInfo *)(curIP + 1);
            ForthVocabulary* pVocab = pEnumInfo->pVocab;
            int32_t numEnums = pEnumInfo->numEnums;
            forthop* pEntry = pVocab->GetEntriesEnd() - pEnumInfo->vocabOffset;
            SNPRINTF(buff, sizeof(buff), "Enum size %d entries %d\n", pEnumInfo->size, numEnums);
            mpEngine->ConsoleOut(buff);
            for (int i = 0; i < numEnums; ++i)
            {
                char* pEnumName = AddTempString(pVocab->GetEntryName(pEntry), pVocab->GetEntryNameLength(pEntry));
                SNPRINTF(buff, sizeof(buff), "%d %s\n", *pEntry & 0xFFFFFF, pEnumName);
                mpEngine->ConsoleOut(buff);
                pEntry = pVocab->NextEntry(pEntry);
            }
        }
        else
        {
            while ((curIP < endIP) && notDone)
            {
#if defined(FORTH64)
                SNPRINTF(buff, sizeof(buff), "  +%04x  %p  ", (int)(curIP - baseIP), curIP);
#else
                SNPRINTF(buff, sizeof(buff), "  +%04x  %08x  ", (curIP - baseIP), curIP);
#endif
                mpEngine->ConsoleOut(buff);
                mpEngine->DescribeOp(curIP, buff, sizeof(buff), true);
                mpEngine->ConsoleOut(buff);
                SNPRINTF(buff, sizeof(buff), "\n");
                mpEngine->ConsoleOut(buff);
                if (((line & 31) == 0) && (mpShell != nullptr) && mpShell->GetInput()->InputStream()->IsInteractive())
                {
                    mpEngine->ConsoleOut("Hit ENTER to continue, 'q' & ENTER to quit\n");
                    c = mpShell->GetChar();

                    if ((c == 'q') || (c == 'Q'))
                    {
                        c = mpShell->GetChar();
                        notDone = false;
                    }
                }
                curIP = NextOp(curIP);
                line++;
            }
        }
    }
}

void
OuterInterpreter::DescribeSymbol( const char *pSymName )
{
    forthop *pEntry = nullptr;
    char buff[256];

    ForthVocabulary* pFoundVocab = nullptr;
    pEntry = GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
    if ( pEntry )
    {
		DescribeOp( pSymName, pEntry[0], pEntry[1] );
    }
    else
    {
        SNPRINTF( buff, sizeof(buff), "Symbol %s not found\n", pSymName );
        SPEW_ENGINE( buff );
        mpEngine->ConsoleOut( buff );
    }
}

forthop*
OuterInterpreter::NextOp(forthop *pOp )
{
    forthop op = *pOp++;
    int32_t opType = FORTH_OP_TYPE( op );
    int32_t opVal = FORTH_OP_VALUE( op );

    switch ( opType )
    {
        case kOpNative:
			if ( (opVal == gCompiledOps[OP_INT_VAL]) || (opVal == gCompiledOps[OP_FLOAT_VAL])
				|| (opVal == gCompiledOps[OP_DO_DO]) ||  (opVal == gCompiledOps[OP_DO_STRUCT_ARRAY]) )
			{
				pOp++;
			}
			else if ( (opVal == gCompiledOps[OP_LONG_VAL]) || (opVal == gCompiledOps[OP_DOUBLE_VAL]) )
			{
				pOp += 2;
			}
            break;

        case kOpConstantString:
            pOp += opVal;
            break;

        default:
            break;
    }
    return pOp;
}

void
OuterInterpreter::StartStructDefinition( void )
{
    mCompileFlags |= kEngineFlagInStructDefinition;
}

void
OuterInterpreter::EndStructDefinition( void )
{
    mCompileFlags &= (~kEngineFlagInStructDefinition);
}

int32_t
OuterInterpreter::AddLocalVar( const char        *pVarName,
                          int32_t              typeCode,
                          int32_t              varSize )
{
    forthop *pEntry;
	int frameCells = mpLocalVocab->GetFrameCells();
    varSize = BYTES_TO_CELLS(varSize);
    BaseType baseType = CODE_TO_BASE_TYPE( typeCode );
    int32_t fieldType = kOpLocalCell;
    if ( !CODE_IS_PTR( typeCode ) )
    {
        if ( baseType <= BaseType::kObject )
        {
            fieldType = kOpLocalByte + (int32_t)baseType;
        }
        else
        {
            fieldType = kOpLocalRef;
        }
    }
    pEntry = mpLocalVocab->AddVariable( pVarName, fieldType, frameCells + varSize, varSize );
    pEntry[1] = typeCode;
    if (frameCells == 0)
    {
        if (mpShell->GetShellStack()->PeekTag() != kShellTagDefine)
        {
            mpEngine->SetError(ForthError::kBadSyntax, "First local variable definition inside control structure");
        }
        else
        {
            // this is first local var definition, leave space for local alloc op
            CompileInt(0);
            ClearPeephole();
        }
    }

    return mpLocalVocab->GetFrameCells();
}

int32_t
OuterInterpreter::AddLocalArray( const char          *pArrayName,
                            int32_t                typeCode,
                            int32_t                elementSize )
{
    forthop *pEntry;
	int frameCells = mpLocalVocab->GetFrameCells();

    BaseType elementType = CODE_TO_BASE_TYPE( typeCode );
    if ( elementType != BaseType::kStruct )
    {
        // array of non-struct
        int32_t arraySize = elementSize * mNumElements;
        arraySize = BYTES_TO_CELLS(arraySize);
        int32_t opcode = CODE_IS_PTR(typeCode) ? kOpLocalCellArray : (kOpLocalByteArray + (int32_t)CODE_TO_BASE_TYPE(typeCode));
        pEntry = mpLocalVocab->AddVariable( pArrayName, opcode, frameCells + arraySize, arraySize );
    }
    else
    {
        // array of struct
        int32_t fieldBytes, alignment, padding, alignMask;
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        pManager->GetFieldInfo( typeCode, fieldBytes, alignment );
        alignMask = alignment - 1;
        padding = fieldBytes & alignMask;
        int32_t paddedSize = (padding) ? (fieldBytes + (alignment - padding)) : fieldBytes;
        int32_t arraySize = paddedSize * mNumElements;
        if ( CODE_IS_PTR(typeCode) )
        {
	        pEntry = mpLocalVocab->AddVariable( pArrayName, kOpLocalCellArray, frameCells + arraySize, arraySize );
        }
        else
        {
	        pEntry = mpLocalVocab->AddVariable( pArrayName, kOpLocalStructArray, ((frameCells + arraySize) << 12) + paddedSize, arraySize );
        }
    }

    if ( frameCells == 0 )
    {
        // this is first local var definition, leave space for local alloc op
        CompileInt( 0 );
    }

	pEntry[1] = typeCode;

    mNumElements = 0;
    return mpLocalVocab->GetFrameCells();
}

bool
OuterInterpreter::HasLocalVariables()
{
	return mpLocalVocab->GetFrameCells() != 0;
}

// TODO: tracing of built-in ops won't work for user-added builtins...
const char *
ForthEngine::GetOpTypeName( int32_t opType )
{
    return (opType < kOpLocalUserDefined) ? opTypeNames[opType] : "unknown";
}

static bool lookupUserTraces = true;


void ForthEngine::TraceOut(const char* pFormat, ...)
{
	if (mTraceOutRoutine != nullptr)
	{
		va_list argList;
		va_start(argList, pFormat);

		mTraceOutRoutine(mpTraceOutData, pFormat, argList);

		va_end(argList);
	}
}


void ForthEngine::SetTraceOutRoutine( traceOutRoutine traceRoutine, void* pTraceData )
{
	mTraceOutRoutine = traceRoutine;
	mpTraceOutData = pTraceData;
}

void ForthEngine::GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const
{
	traceRoutine = mTraceOutRoutine;
	pTraceData = mpTraceOutData;
}

void ForthEngine::TraceOp(forthop* pOp, forthop op)
{
#ifdef TRACE_INNER_INTERPRETER
    char buff[ 512 ];
#if 0
    int rDepth = pCore->RT - pCore->RP;
    char* sixteenSpaces = "                ";     // 16 spaces
	//if ( *pOp != gCompiledOps[OP_DONE] )
	{
		DescribeOp(pOp, buff, sizeof(buff), lookupUserTraces);
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
    DescribeOp(pOp, buff, sizeof(buff), lookupUserTraces);
    TraceOut("# 0x%16p # %s # ", pOp, buff);
#endif
#endif
}

void ForthEngine::TraceStack( ForthCoreState* pCore )
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
        mpEngine->TraceOut(" %llx", *pSP++);
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
        mpEngine->TraceOut(" %llx", *pRP++);
#else
        TraceOut(" %x", *pRP++);
#endif
    }

    if (nItems > numToDisplay)
    {
        TraceOut(" <%d more>", nItems - numToDisplay);
    }
}

void ForthEngine::DescribeOp(forthop *pOp, char *pBuffer, int buffSize, bool lookupUserDefs )
{
    forthop op = *pOp;
    forthOpType opType = FORTH_OP_TYPE( op );
    uint32_t opVal = FORTH_OP_VALUE( op );
    ForthVocabulary *pFoundVocab = nullptr;
    forthop *pEntry = nullptr;

	const char* preamble = "%02x:%06x    ";
	int preambleSize = (int)strlen( preamble );
	if ( buffSize <= (preambleSize + 1) )
	{
		return;
	}

    SNPRINTF( pBuffer, buffSize, preamble, opType, opVal );
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
                    // traceable built-in op
					if ( opVal == gCompiledOps[OP_INT_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s 0x%x", gOpNames[opVal], pOp[1] );
					}
					else if ( opVal == gCompiledOps[OP_FLOAT_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s %f", gOpNames[opVal], *((float *)(&(pOp[1]))) );
					}
					else if ( opVal == gCompiledOps[OP_DOUBLE_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s %g", gOpNames[opVal], *((double *)(&(pOp[1]))) );
					}
					else if ( opVal == gCompiledOps[OP_LONG_VAL] )
					{
						SNPRINTF( pBuffer, buffSize, "%s 0x%llx", gOpNames[opVal], *((int64_t *)(&(pOp[1]))) );
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
            
            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpDLLEntryPoint:
                if (lookupUserDefs)
                {
                    searchVocabsForOp = true;
                }
                break;

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
                    SNPRINTF(pBuffer, buffSize, "%s_%x%s", opTypeName, (opVal & 0xFFFFF), ForthParseInfo::GetVaropSuffix(varOp));
                }
                else
                {
                    SNPRINTF(pBuffer, buffSize, "%s_%x", opTypeName, opVal);
                }
                break;
            }

            case kOpConstantString:
                SNPRINTF( pBuffer, buffSize, "\"%s\"", (char *)(pOp + 1) );
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
                ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
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

void ForthEngine::AddOpExecutionToProfile(forthop op)
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

void ForthEngine::DumpExecutionProfile()
{
    char buffer[256];
    char opBuffer[128];

    ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
    while (pVocab != nullptr)
    {
        forthop *pEntry = pVocab->GetNewestEntry();
        if (pVocab->IsClass())
        {

            forthop* pMethods = ((ForthClassVocabulary *)pVocab)->GetMethods();
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

        ForthVocabulary *pVocab = opInfo.pVocabulary;
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

void ForthEngine::ResetExecutionProfile()
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


bool
OuterInterpreter::AddOpType( forthOpType opType, optypeActionRoutine opAction )
{

    if ( (opType >= kOpLocalUserDefined) && (opType <= kOpMaxLocalUserDefined) )
    {
        mpCore->optypeAction[ opType ] = opAction;
    }
    else
    {
        // opType out of range
        return false;
    }
    return true;
}


char *
OuterInterpreter::GetLastInputToken( void )
{
    return mpLastToken;
}


// return true IFF token is an integer literal
// sets isOffset if token ended with a + or -
// NOTE: temporarily modifies string @pToken
bool
OuterInterpreter::ScanIntegerToken( char         *pToken,
                               int64_t      &value,
                               int          base,
                               bool         &isOffset,
                               bool&        isSingle )
{
    int32_t digit;
    bool isNegative;
    char c;
    int digitsFound = 0;
    ucell len = (ucell)strlen( pToken );
    char *pLastChar = pToken + (len - 1);
    char lastChar = *pLastChar;
    bool isValid = false;

    // if CFloatLiterals is off, '.' indicates a double-precision number, not a float
    bool periodMeansDoubleInt = !CheckFeature( kFFCFloatLiterals );

    isOffset = false;
    isSingle = true;

    // handle leading plus or minus sign
    isNegative = (pToken[0] == '-');
    if ( isNegative || (pToken[0] == '+') )
    {
        // strip leading plus/minus sign
        pToken++;
    }

    // handle trailing plus or minus sign
    if ( lastChar == '+' )
    {
        isOffset = true;
        *pLastChar = 0;
    }
    else if ( lastChar == '-' )
    {
        isOffset = true;
        *pLastChar = 0;
        isNegative = !isNegative;
    }
    else if ( (lastChar == 'L') || (lastChar == 'l') )
    {
        isSingle = false;
        *pLastChar = 0;
    }
    else
    {
        pLastChar = nullptr;
    }

    // see if this is ANSI style double precision int
    if (periodMeansDoubleInt)
    {
        isSingle = (strchr(pToken, '.') == nullptr);
    }

    if ( (pToken[0] == '$') && CheckFeature(kFFDollarHexLiterals) )
    {
        if ( sscanf( pToken + 1, "%llx", &value) == 1 )
        {
            if ( isNegative )
            {
                value = 0 - value;
            }
            isValid = true;
        }
    }
	
    if ( !isValid && ((pToken[0] == '0') && (pToken[1] == 'x')) && CheckFeature(kFFCHexLiterals) )
    {
#if defined(WIN32)
        if (sscanf(pToken + 2, "%I64x", &value) == 1)
#else
        if (sscanf(pToken + 2, "%llx", &value) == 1)
#endif
        {
            if (isNegative)
            {
                value = 0 - value;
            }
            isValid = true;
        }
    }

    if ( !isValid )
    {

        isValid = true;
        value = 0;
        while ( (c = *pToken++) != 0 )
        {

            if ( (c >= '0') && (c <= '9') )
            {
                digit = c - '0';
                digitsFound++;
            }
            else if ( (c >= 'A') && (c <= 'Z') )
            {
                digit = 10 + (c - 'A');
                digitsFound++;
            }
            else if ( (c >= 'a') && (c <= 'z') )
            {
                digit = 10 + (c - 'a');
                digitsFound++;
            }
            else
            {
                // char can't be a digit
                if ( (c == '.') && periodMeansDoubleInt )
                {
                    // ignore . in double precision int
                    continue;
                }
                isValid = false;
                break;
            }

            if ( digit >= base )
            {
                // invalid digit for current base
                isValid = false;
                break;
            }
            value = (value * base) + digit;
        }

        if ( digitsFound == 0 )
        {
            isValid = false;
        }

        // all chars were valid digits
        if ( isNegative )
        {
            value = 0 - value;
        }
    }

    // restore original last char
    if ( pLastChar != nullptr )
    {
        *pLastChar = lastChar;
    }

    return isValid;
}


// return true IFF token is a real literal
// sets isSingle to tell if result is a float or double
// NOTE: temporarily modifies string @pToken
bool OuterInterpreter::ScanFloatToken( char *pToken, float& fvalue, double& dvalue, bool& isSingle, bool& isApproximate )
{
   bool retVal = false;
   double dtemp;

   isApproximate = false;
   // a leading tilde means that value may be approximated with lowest precision 
   if ( *pToken == '~' )
   {
	   isApproximate = true;
	   pToken++;
   }

   ucell len = (ucell)strlen( pToken );
   if ( CheckFeature( kFFCFloatLiterals ) )
   {
       if ( strchr( pToken, '.' ) == nullptr )
       {
          return false;
       }
   }
   else
   {
       if ( (strchr( pToken, 'e' ) == nullptr) && (strchr( pToken, 'E' ) == nullptr) )
       {
          return false;
       }
   }
   char *pLastChar = pToken + (len - 1);
   char lastChar = tolower(*pLastChar);
   switch ( lastChar )
   {
   case 'd':
   case 'l':
      *pLastChar = 0;
      if ( sscanf( pToken, "%lf", &dvalue ) == 1)
      {
         retVal = true;
         isSingle = false;
      }
      *pLastChar = lastChar;
      break;
   case 'f':
	  *pLastChar = 0;

      if ( sscanf( pToken, "%lf", &dtemp) == 1)
      {
          fvalue = (float)dtemp;
          retVal = true;
          isSingle = true;
      }
      *pLastChar = lastChar;
      break;
   default:
       if ( sscanf( pToken, "%lf", &dtemp) == 1)
        {
            fvalue = (float)dtemp;
            retVal = true;
            isSingle = true;
        }
        break;
   }

   return retVal;
}


// squish float down to 24-bits, returns true IFF number can be represented exactly
//   OR approximateOkay==true and number is within range of squished float
bool
OuterInterpreter::SquishFloat( float fvalue, bool approximateOkay, uint32_t& squishedFloat )
{
	// single precision format is 1 sign, 8 exponent, 23 mantissa
	uint32_t inVal = *(reinterpret_cast<uint32_t *>( &fvalue ));

	// if bottom 5 bits of inVal aren't 0, number can't be exactly represented in squished format
	if ( !approximateOkay && ((inVal & 0x1f) != 0) )
	{
		// number can't be represented exactly
		return false;
	}
    uint32_t sign = (inVal & 0x80000000) >> 8;
	int32_t exponent = (((inVal >> 23) & 0xff) - (127 - 15));
	// if exponent is less than 0 or greater than 31, number can't be represented in squished format at all
	if ( (exponent < 0) || (exponent > 31) )
	{
		return false;
	}
	uint32_t mantissa = (inVal >> 5) & 0x3ffff;
	squishedFloat = sign | (exponent << 18) | mantissa;

	return true;
}

// squish double down to 24-bits, returns true IFF number can be represented exactly
//   OR approximateOkay==true and number is within range of squished float
bool
OuterInterpreter::SquishDouble( double dvalue, bool approximateOkay, uint32_t& squishedDouble )
{
	// double precision format is 1 sign, 11 exponent, 52 mantissa
	uint32_t* pInVal = reinterpret_cast<uint32_t *>( &dvalue );
	uint32_t inVal = pInVal[1];

	// if bottom 34 bits of inVal aren't 0, number can't be exactly represented in squished format
	if ( !approximateOkay && ( (pInVal[0] != 0) || ((inVal & 0x3) != 0) ) )
	{
		// number can't be represented exactly
		return false;
	}
    uint32_t sign = (inVal & 0x80000000) >> 8;
	int32_t exponent = (((inVal >> 20) & 0x7ff) - (1023 - 15));
	// if exponent is less than 0 or greater than 31, number can't be represented in squished format at all
	if ( (exponent < 0) || (exponent > 31) )
	{
		return false;
	}
	uint32_t mantissa = (inVal >> 2) & 0x3ffff;
	squishedDouble = sign | (exponent << 18) | mantissa;

	return true;
}

float
OuterInterpreter::UnsquishFloat( uint32_t squishedFloat )
{
	uint32_t unsquishedFloat;

	uint32_t sign = (squishedFloat & 0x800000) << 8;
	uint32_t exponent = (((squishedFloat >> 18) & 0x1f) + (127 - 15)) << 23;
	uint32_t mantissa = (squishedFloat & 0x3ffff) << 5;
	unsquishedFloat = sign | exponent | mantissa;

	return *(reinterpret_cast<float *>( &unsquishedFloat ));
}

double
OuterInterpreter::UnsquishDouble( uint32_t squishedDouble )
{
	uint32_t unsquishedDouble[2];

	unsquishedDouble[0] = 0;
	uint32_t sign = (squishedDouble & 0x800000) << 8;
	uint32_t exponent = (((squishedDouble >> 18) & 0x1f) + (1023 - 15)) << 20;
	uint32_t mantissa = (squishedDouble & 0x3ffff) << 2;
	unsquishedDouble[1] = sign | exponent | mantissa;

	return *(reinterpret_cast<double *>( &unsquishedDouble[0] ));
}

bool
OuterInterpreter::SquishLong( int64_t lvalue, uint32_t& squishedLong )
{
	bool isValid = false;
	int32_t* pLValue = reinterpret_cast<int32_t*>( &lvalue );
	int32_t hiPart = pLValue[1];
	uint32_t lowPart = static_cast<uint32_t>( pLValue[0] & 0x00FFFFFF );
	uint32_t midPart = static_cast<uint32_t>( pLValue[0] & 0xFF000000 );

	if ( (lowPart & 0x800000) != 0 )
	{
		// negative number
		if ( (hiPart == -1) && (midPart == 0xFF000000) )
		{
			isValid = true;
			squishedLong = lowPart;
		}
	}
	else
	{
		// positive number
		if ( (hiPart == 0) && (midPart == 0) )
		{
			isValid = true;
			squishedLong = lowPart;
		}
	}

	return isValid;
}

int64_t
OuterInterpreter::UnsquishLong( uint32_t squishedLong )
{
	int32_t unsquishedLong[2];

	unsquishedLong[0] = 0;
	if ( (squishedLong & 0x800000) != 0 )
	{
		unsquishedLong[0] = squishedLong | 0xFF000000;
		unsquishedLong[1] = -1;
	}
	else
	{
		unsquishedLong[0] = squishedLong;
		unsquishedLong[1] = 0;
	}
	return *(reinterpret_cast<int64_t *>( &unsquishedLong[0] ));
}

// compile an opcode
// remember the last opcode compiled so someday we can do optimizations
//   like combining "->" followed by a local var name into one opcode
void
OuterInterpreter::CompileOpcode(forthOpType opType, forthop opVal)
{
    SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x\n", COMPILED_OP(opType, opVal), mpDictionary->pCurrent);
    mpOpcodeCompiler->CompileOpcode(opType, opVal);
}

#if defined(DEBUG)
void OuterInterpreter::CompileInt(int32_t v)
{
    SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x\n", v, mpDictionary->pCurrent);
    *mpDictionary->pCurrent++ = v;
}

void OuterInterpreter::CompileDouble(double v)
{
    SPEW_COMPILATION("Compiling double %g @ 0x%08x\n", v, mpDictionary->pCurrent);
    *((double *)mpDictionary->pCurrent) = v; mpDictionary->pCurrent += 2;
}

void OuterInterpreter::CompileCell(cell v)
{
    SPEW_COMPILATION("Compiling cell 0x%p @ 0x%p\n", v, mpDictionary->pCurrent);
    *((cell*)mpDictionary->pCurrent) = v; mpDictionary->pCurrent += CELL_LONGS;
}
#endif

// patch an opcode - fill in the branch destination offset
void OuterInterpreter::PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode)
{
    SPEW_COMPILATION("Patching 0x%08x @ 0x%08x\n", COMPILED_OP(opType, opVal), pOpcode);
    mpOpcodeCompiler->PatchOpcode(opType, opVal, pOpcode);
}

void OuterInterpreter::ClearPeephole()
{
    mpOpcodeCompiler->ClearPeephole();
}

void
OuterInterpreter::CompileOpcode(forthop op )
{
	CompileOpcode( FORTH_OP_TYPE( op ), FORTH_OP_VALUE( op ) );
}

void
OuterInterpreter::CompileBuiltinOpcode(forthop op )
{
	if ( op < NUM_COMPILED_OPS )
	{
		CompileOpcode( gCompiledOps[op] );
	}

    if (op == OP_ABORT)
    {
        ClearPeephole();
    }
}

void OuterInterpreter::UncompileLastOpcode( void )
{
    forthop *pLastCompiledOpcode = mpOpcodeCompiler->GetLastCompiledOpcodePtr();
    if ( pLastCompiledOpcode != nullptr )
    {
		SPEW_COMPILATION("Uncompiling from 0x%08x to 0x%08x\n", mpDictionary->pCurrent, pLastCompiledOpcode);
		mpOpcodeCompiler->UncompileLastOpcode();
    }
    else
    {
        SPEW_ENGINE( "OuterInterpreter::UncompileLastOpcode called with no previous opcode\n" );
        mpEngine->SetError( ForthError::kMissingSize, "UncompileLastOpcode called with no previous opcode" );
    }
}

forthop* OuterInterpreter::GetLastCompiledOpcodePtr( void )
{
	return mpOpcodeCompiler->GetLastCompiledOpcodePtr();
}

forthop*
OuterInterpreter::GetLastCompiledIntoPtr( void )
{
	return mpOpcodeCompiler->GetLastCompiledIntoPtr();
}

// interpret/compile a constant value/offset
void
OuterInterpreter::ProcessConstant(int64_t value, bool isOffset, bool isSingle)
{
    if ( mCompileState )
    {
        // compile the literal value
        if (isSingle)
        {
            int32_t lvalue = (int32_t)value;
            if ((lvalue < (1 << 23)) && (lvalue >= -(1 << 23)))
            {
                // value fits in opcode immediate field
                CompileOpcode((isOffset ? kOpOffset : kOpConstant), lvalue & 0xFFFFFF);
            }
            else
            {
                // value too big, must go in next longword
                if (isOffset)
                {
                    CompileBuiltinOpcode(OP_INT_VAL);
                    *mpDictionary->pCurrent++ = lvalue;
                    CompileBuiltinOpcode(OP_PLUS);
                }
                else
                {
                    CompileBuiltinOpcode(OP_INT_VAL);
                    *mpDictionary->pCurrent++ = lvalue;
                }
            }
        }
        else
        {
            // compile the literal value
            // TODO: support 64-bit offsets?
            uint32_t squishedLong;
            if (SquishLong(value, squishedLong))
            {
                CompileOpcode(kOpSquishedLong, squishedLong);
            }
            else
            {
                CompileBuiltinOpcode(OP_DOUBLE_VAL);
                forthop* pDP = mpDictionary->pCurrent;
#if defined(FORTH64)
                *(int64_t*)pDP = value;
                pDP += 2;
#else
                stackInt64 val;
                val.s64 = value;
                *pDP++ = val.s32[1];
                *pDP++ = val.s32[0];
#endif
                mpDictionary->pCurrent = pDP;
            }
        }
    }
    else
    {
        if ( isOffset )
        {
            // add value to top of param stack
            *mpCore->SP += value;
        }
        else
        {
            // leave value on param stack
#if defined(FORTH64)
            *--mpCore->SP = value;
#else
            if (isSingle)
            {
                *--mpCore->SP = (int32_t) value;
            }
            else
            {
                stackInt64 val;
                val.s64 = value;
                *--mpCore->SP = val.s32[0];
                *--mpCore->SP = val.s32[1];
            }
#endif
        }
    }
}

// return true IFF the last compiled opcode was an integer literal
bool
OuterInterpreter::GetLastConstant( int32_t& constantValue )
{
    forthop *pLastCompiledOpcode = mpOpcodeCompiler->GetLastCompiledOpcodePtr();
    if ( pLastCompiledOpcode != nullptr )
	{
        forthop op = *pLastCompiledOpcode;
        if ( ((pLastCompiledOpcode + 1) == mpDictionary->pCurrent)
            && (FORTH_OP_TYPE( op ) == kOpConstant) )
        {
            constantValue = FORTH_OP_VALUE( op );
            return true;
        }
    }
    return false;
}

//
// FullyExecuteOp is used by the Outer Interpreter (OuterInterpreter::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
OpResult ForthEngine::FullyExecuteOp(ForthCoreState* pCore, forthop opCode)
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
OpResult ForthEngine::ExecuteOp(ForthCoreState* pCore, forthop opCode)
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
OpResult ForthEngine::ExecuteOps(ForthCoreState* pCore, forthop *pOps)
{
    forthop *savedIP;

    savedIP = pCore->IP;
    pCore->IP = pOps;
    ForthFiber* pFiber = (ForthFiber*)(pCore->pFiber);
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

OpResult ForthEngine::FullyExecuteMethod(ForthCoreState* pCore, ForthObject& obj, int methodNum)
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
OpResult ForthEngine::DeleteObject(ForthCoreState* pCore, ForthObject& obj)
{
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
    int objSize = pClassObject->pVocab->GetSize();
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

        ForthClassVocabulary* parentVocabulary = pClassObject->pVocab->ParentClass();
        pClassObject = parentVocabulary ? parentVocabulary->GetClassObject() : nullptr;
        if (pClassObject != nullptr)
        {
            obj->pMethods = parentVocabulary->GetMethods();
        }
        else
        {
            break;
        }
    }
    obj->pMethods = savedMethods;

    // now free the storage for the object instance
    DEALLOCATE_BYTES(obj, objSize);

    //deleteDepth--;
    return exitStatus;
}

void ForthEngine::ReleaseObject(ForthCoreState* pCore, ForthObject& inObject)
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


void ForthEngine::AddErrorText( const char *pString )
{
    strcat( mpErrorString, pString );
}

void ForthEngine::SetError( ForthError e, const char *pString )
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

void ForthEngine::SetFatalError( ForthError e, const char *pString )
{
    mpCore->state = OpResult::kFatalError;
    mpCore->error = e;
    if ( pString )
    {
        strcpy( mpErrorString, pString );
    }
}

void ForthEngine::GetErrorString( char *pBuffer, int bufferSize )
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


OpResult ForthEngine::CheckStacks( void )
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


void ForthEngine::SetDefaultConsoleOut( ForthObject& newOutStream )
{
	SPEW_SHELL("SetDefaultConsoleOut pCore=%p  pMethods=%p  pData=%p\n", mpCore, newOutStream->pMethods, newOutStream);
    OBJECT_ASSIGN(mpCore, mDefaultConsoleOutStream, newOutStream);
}

void ForthEngine::SetConsoleOut(ForthCoreState* pCore, ForthObject& newOutStream)
{
    SPEW_SHELL("SetConsoleOut pCore=%p  pMethods=%p  pData=%p\n", pCore, newOutStream->pMethods, newOutStream);
    OBJECT_ASSIGN(pCore, pCore->consoleOutStream, newOutStream);
}

void ForthEngine::SetErrorOut(ForthCoreState* pCore, ForthObject& newOutStream)
{
    SPEW_SHELL("SetErrorOut pCore=%p  pMethods=%p  pData=%p\n", pCore, newOutStream->pMethods, newOutStream);
    OBJECT_ASSIGN(pCore, mErrorOutStream, newOutStream);
}

void* ForthEngine::GetErrorOut(ForthCoreState* pCore)
{
    return mErrorOutStream;
}

void ForthEngine::PushConsoleOut( ForthCoreState* pCore )
{
	PUSH_OBJECT( pCore->consoleOutStream );
}

void ForthEngine::PushDefaultConsoleOut( ForthCoreState* pCore )
{
	PUSH_OBJECT( mDefaultConsoleOutStream );
}

void ForthEngine::PushErrorOut(ForthCoreState* pCore)
{
    PUSH_OBJECT(mErrorOutStream);
}

void ForthEngine::ResetConsoleOut( ForthCoreState& core )
{
	// TODO: there is a dilemma here - either we just replace the current output stream
	//  without doing a release, and possibly leak a stream object, or we do a release
	//  and risk a crash, since ResetConsoleOut is called when an error is detected,
	//  so the object we are releasing may already be deleted or otherwise corrupted.
    CLEAR_OBJECT(core.consoleOutStream);
    OBJECT_ASSIGN(&core, core.consoleOutStream, mDefaultConsoleOutStream);
}

void ForthEngine::ResetConsoleOut()
{
    ResetConsoleOut(*mpCore);
}


void ForthEngine::ConsoleOut( const char* pBuff )
{
    ForthConsoleStringOut( mpCore, pBuff );
}


int32_t ForthEngine::GetTraceFlags( void )
{
	return mpCore->traceFlags;
}

void ForthEngine::SetTraceFlags( int32_t flags )
{
	mpCore->traceFlags = flags;
}

////////////////////////////
//
// enumerated type support
//
void
OuterInterpreter::StartEnumDefinition( void )
{
    SetFlag( kEngineFlagInEnumDefinition );
    mNextEnum = 0;
    // remember the stack level at start of definition
    // when another enum is to be defined, if the current stack is above this level,
    // the top element on the stack will be popped and set the current value of the enum
    mpEnumStackBase = mpCore->SP;
}

void
OuterInterpreter::EndEnumDefinition( void )
{
    ClearFlag( kEngineFlagInEnumDefinition );
}

// return milliseconds since engine was created
uint32_t ForthEngine::GetElapsedTime( void )
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


void ForthEngine::DumpCrashState()
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

void ForthEngine::DisplayUserDefCrash( forthop *pRVal, char* buff, int buffSize )
{
	if ( (pRVal >= mDictionary.pBase) && (pRVal < mDictionary.pCurrent) )
	{
        forthop* pDefBase = nullptr;
		ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
        forthop* pClosestIP = nullptr;
        forthop* pFoundClosest = nullptr;
		ForthVocabulary* pFoundVocab = nullptr;
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

forthop * OuterInterpreter::FindUserDefinition( ForthVocabulary* pVocab, forthop*& pClosestIP, forthop* pIP, forthop*& pBase  )
{
    forthop* pClosest = nullptr;
	forthop* pEntry = pVocab->GetNewestEntry();

	for ( int i = 0; i < pVocab->GetNumEntries(); i++ )
	{
		int32_t typeCode = pEntry[1];
		uint32_t opcode = 0;
		if ( CODE_TO_BASE_TYPE(pEntry[1]) == BaseType::kUserDefinition )
		{
			switch ( FORTH_OP_TYPE( *pEntry ) )
			{
				case kOpUserDef:
				case kOpUserDefImmediate:
					{
						opcode = ((uint32_t)FORTH_OP_VALUE( *pEntry ));
					}
					break;
				default:
					break;
			}
		}
		else if ( CODE_IS_METHOD( typeCode ) )
		{
			// get opcode from method
			ForthClassVocabulary* pClassVocab = (ForthClassVocabulary*) pVocab;
			ForthInterface* pInterface = pClassVocab->GetInterface(0);
			// TODO: deal with secondary interfaces
			opcode = pInterface->GetMethod( *pEntry );
			opcode = ((uint32_t)FORTH_OP_VALUE( opcode ));
		}
		if ( (opcode > 0) && (opcode < mpCore->numOps) )
		{
            forthop* pDef = mpCore->ops[opcode];
			if ( (pDef > pClosestIP) && (pDef <= pIP) )
			{
				pClosestIP = pDef;
				pClosest = pEntry;
				pBase = pDef;
			}
		}
		pEntry = pVocab->NextEntry( pEntry );
	}
	return pClosest;
}


// this was an inline, but sometimes that returned the wrong value for unknown reasons
ForthCoreState*	ForthEngine::GetCoreState( void )
{
	return mpCore;
}


ForthBlockFileManager* ForthEngine::GetBlockFileManager()
{
    return mBlockFileManager;
}


bool ForthEngine::IsServer() const
{
	return mIsServer;
}

void ForthEngine::SetIsServer(bool isServer)
{
	mIsServer = isServer;
}

ForthClassVocabulary* ForthEngine::AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry* pEntries)
{
    return mpOuter->AddBuiltinClass(pClassName, classIndex, parentClassIndex, pEntries);
}

void OuterInterpreter::DefineLabel(const char* inLabelName, forthop* inLabelIP)
{
	for (Label& label : mLabels)
	{
		if (label.name == inLabelName)
		{
			label.DefineLabelIP(inLabelIP);
			return;
		}
	}
	mLabels.emplace_back(Label(inLabelName, inLabelIP));
}

void OuterInterpreter::AddGoto(const char* inLabelName, int inBranchType, forthop* inBranchIP)
{
	for (Label& label : mLabels)
	{
		if (label.name == inLabelName)
		{
			label.AddReference(inBranchIP, inBranchType);
			return;
		}
	}
	Label newLabel(inLabelName);
	newLabel.AddReference(inBranchIP, inBranchType);
	mLabels.push_back(newLabel);
}

// if inText is null, string is not copied, an uninitialized space of size inNumChars+1 is allocated
// if inNumChars is -1 and inText is not null, length of input string is used for temp string size
// if both inText is null and inNumChars is -1, an uninitialized space of 255 chars is allocated
char* OuterInterpreter::AddTempString(const char* inText, cell inNumChars)
{
	// this hooha turns mpStringBufferA into multiple string buffers
	//   so that you can use multiple interpretive string buffers
	// it is used both for quoted strings in interpretive mode and blword/$word
	// we leave space for a preceeding length byte and a trailing null terminator
	if (inNumChars < 0)
	{
		inNumChars = (inText == nullptr) ? 255 : strlen(inText);
	}
	if (UnusedTempStringSpace() <= (inNumChars + 2))
	{
		mpStringBufferANext = mpStringBufferA;
	}
	char* result = mpStringBufferANext + 1;
	if (inText != nullptr)
	{
		memcpy(result, inText, inNumChars);
	}

	// the preceeding length byte will be wrong for strings longer than 255 characters
	*mpStringBufferANext = (char)inNumChars;
	result[inNumChars] = '\0';

	mpStringBufferANext += (inNumChars + 2);

	return result;
}

//############################################################################
//
//          Continue statement support
//
//############################################################################
void OuterInterpreter::PushContinuationType(cell val)
{
    PushContinuationAddress((forthop*)val);
}

void OuterInterpreter::PushContinuationAddress(forthop* pOp)
{
    if ((size_t)mContinuationIx >= mContinuations.size())
    {
        mContinuations.resize(mContinuationIx + 32);
    }
    mContinuations[mContinuationIx++] = pOp;
}

forthop* OuterInterpreter::PopContinuationAddress()
{
    forthop* result = nullptr;
    if (mContinuationIx > 0)
    {
        mContinuationIx--;
        result = mContinuations[mContinuationIx];
    }
    else
    {
        mpEngine->SetError(ForthError::kBadSyntax, "not enough continuations");
    }
    return result;
}

cell OuterInterpreter::PopContinuationType()
{
    return (cell) PopContinuationAddress();
}

void OuterInterpreter::ResetContinuations()
{
    mContinuations.clear();
    mContinuationIx = 0;
    mContinueDestination = nullptr;
    mContinueCount = 0;
}

forthop* OuterInterpreter::GetContinuationDestination()
{
    return mContinueDestination;
}

void OuterInterpreter::SetContinuationDestination(forthop* pDest)
{
    mContinueDestination = pDest;
}

void OuterInterpreter::AddContinuationBranch(forthop* pAddr, cell opType)
{
    PushContinuationAddress(pAddr);
    PushContinuationType(opType);
    ++mContinueCount;
}

void OuterInterpreter::AddBreakBranch(forthop* pAddr, cell opType)
{
    // set low bit of address to flag that this is a break branch
    PushContinuationAddress((forthop *)(((cell)pAddr) + 1));
    PushContinuationType(opType);
    ++mContinueCount;
}

void OuterInterpreter::StartLoopContinuations()
{
    PushContinuationAddress(mContinueDestination);
    PushContinuationType(mContinueCount);
    mContinueDestination = nullptr;
    mContinueCount = 0;
}

void OuterInterpreter::EndLoopContinuations(int controlFlowType)  // actually takes a eShellTag
{
    // fixup pending continue branches for current loop
    if (mContinueCount > 0)
    {
        forthop *pDP = mpEngine->GetDP();

        for (int i = 0; i < mContinueCount; ++i)
        {
            if (mContinuationIx >= 2)
            {
                cell opType = PopContinuationType();
                forthop* target = PopContinuationAddress();
                if (((cell)target & 1) != 0)
                {
                    // this is actually a break
                    if (controlFlowType != kShellTagDo)
                    {
                        forthop *pBreak = (forthop *)((cell)target & ~1);
                        *pBreak = (forthop)COMPILED_OP(opType, (int32_t)(pDP - pBreak) - 1);
                    }
                    else
                    {
                        mpEngine->SetError(ForthError::kBadSyntax, "break not allowed in do loop, use leave");
                        break;
                    }
                }
                else
                {
                    if (controlFlowType != kShellTagCase)
                    {
                        if (mContinueDestination != nullptr)
                        {
                            forthop *pContinue = target;
                            *pContinue = (forthop)COMPILED_OP(opType, (int32_t)(mContinueDestination - pContinue) - 1);
                        }
                        else
                        {
                            mpEngine->SetError(ForthError::kBadSyntax, "end loop with unresolved continues");
                            break;
                        }
                    }
                    else
                    {
                        mpEngine->SetError(ForthError::kBadSyntax, "continue not allowed in case statement");
                        break;
                    }
                }
            }
            else
            {
                // report error - end loop with continuation stack empty
                mpEngine->SetError(ForthError::kBadSyntax, "end loop with continuation stack empty");
                break;
            }
        }
    }
    mContinueCount = PopContinuationType();
    mContinueDestination = PopContinuationAddress();
}

bool OuterInterpreter::HasPendingContinuations()
{
    return mContinueCount != 0;
}


void OuterInterpreter::AddGlobalObjectVariable(ForthObject* pObject, ForthVocabulary* pVocab, const char* pName)
{
    mpEngine->TraceOut("Adding %s global object [%d] @%p of class %s\n", pName, mGlobalObjectVariables.size(), pObject, pVocab->GetName());
    mGlobalObjectVariables.push_back(pObject);
}

void OuterInterpreter::CleanupGlobalObjectVariables(forthop* pNewDP)
{
    cell objectIndex = mGlobalObjectVariables.size() - 1;
    while (objectIndex >= 0)
    {
        if (mGlobalObjectVariables[objectIndex] < (ForthObject *)pNewDP)
        {
            // we are done releasing objects, all remaining objects are below new DP
            break;
        }
	ForthObject* pVariable = mGlobalObjectVariables[objectIndex];
        ForthObject& obj = *pVariable;
        if (obj != nullptr)
        {
            ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
            ucell refCount = obj->refCount;
            mpEngine->TraceOut("Releasing global object [%d] @%p of class %s, refcount %d\n", objectIndex, pVariable, pClassObject->pVocab->GetName(), refCount);
            SAFE_RELEASE(mpCore, obj);
        }
        objectIndex--;
    }
    mGlobalObjectVariables.resize(objectIndex + 1);
}

char* OuterInterpreter::GrabTempBuffer()
{
#ifdef WIN32
    EnterCriticalSection(mpTempBufferLock);
#else
    pthread_mutex_lock(mpTempBufferLock);
#endif

    return mpTempBuffer;
}

void OuterInterpreter::UngrabTempBuffer()
{
#ifdef WIN32
    EnterCriticalSection(mpTempBufferLock);
#else
    pthread_mutex_lock(mpTempBufferLock);
#endif
}

void ForthEngine::RaiseException(ForthCoreState *pCore, cell newExceptionNum)
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

// return true IFF compilation occured
bool OuterInterpreter::CompileLocalVariableOpcode(forthop* pEntry, VarOperation varop)
{
    if (varop < VarOperation::kNumBasicVarops)
    {
        CompileOpcode(*pEntry | (((int)varop) << 20));
        return true;
    }

    // TODO: handle pointer varops
    return false;
}

//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################

// return true to exit forth shell
OpResult OuterInterpreter::ProcessToken( ForthParseInfo   *pInfo )
{
    forthop* pEntry;
    int64_t lvalue;
    OpResult exitStatus = OpResult::kOk;
    float fvalue;
    double dvalue;
    char *pToken = pInfo->GetToken();
    int len = pInfo->GetTokenLength();
    bool isAString = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_STRING) != 0;
	bool isAQuotedCharacter = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_CHARACTER) != 0;
    bool isSingle, isOffset, isApproximate;
    double* pDPD;
    ForthVocabulary* pFoundVocab = nullptr;

    mpLastToken = pToken;
    if ( (pToken == nullptr)   ||   ((len == 0) && !(isAString || isAQuotedCharacter)) )
    {
        // ignore empty tokens, except for the empty quoted string and null character
        return OpResult::kOk;
    }
    
    if (mCompileState)
    {
        SPEW_OUTER_INTERPRETER("Compile {%s} flags[%x] @0x%08x\t", pToken, pInfo->GetFlags(), mpDictionary->pCurrent);
    }
    else
    {
        SPEW_OUTER_INTERPRETER("Interpret {%s} flags[%x]\t", pToken, pInfo->GetFlags());
    }

    if ( isAString )
    {
        ////////////////////////////////////
        //
        // symbol is a quoted string - the quotes have already been stripped
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "String{%s} flags[%x] len %d\n", pToken, pInfo->GetFlags(), len );
        if ( mCompileState )
        {
            int lenLongs = ((len + 4) & ~3) >> 2;
            CompileOpcode( kOpConstantString, lenLongs );
            strcpy( (char *) mpDictionary->pCurrent, pToken );
            mpDictionary->pCurrent += lenLongs;
        }
        else
        {
            // in interpret mode, stick the string in string buffer A
            //   and leave the address on param stack
            *--mpCore->SP = (cell) AddTempString(pToken, len);
        }
        return OpResult::kOk;
        
    }
    else if ( isAQuotedCharacter )
    {
        ////////////////////////////////////
        //
        // symbol is a quoted character - the quotes have already been stripped
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Character{%s} flags[%x]\n", pToken, pInfo->GetFlags() );
		bool isALongQuotedCharacter = (pInfo->GetFlags() & PARSE_FLAG_FORCE_LONG) != 0;
		int tokenLen = (int)strlen(pToken);
        lvalue = 0;
        char* cval = (char *)&lvalue;
        for (int i = 0; i < len; i++)
        {
            cval[i] = pToken[i];
        }
#if defined(FORTH64)
        isSingle = (tokenLen > 3);
#else
        isSingle = !(isALongQuotedCharacter || (tokenLen > 4));
#endif
        ProcessConstant(lvalue, false, isSingle);
		return OpResult::kOk;
    }
    
    if ( mpInterpreterExtension != nullptr )
    {
        if ( (*mpInterpreterExtension)( pToken ) )
        {
            ////////////////////////////////////
            //
            // symbol was processed by user-defined interpreter extension
            //
            ////////////////////////////////////
            return OpResult::kOk;
        }
    }

    // check for local variables
    if ( mCompileState)
    {
        pEntry = mpLocalVocab->FindSymbol( pInfo );
        if ( pEntry )
        {
            ////////////////////////////////////
            //
            // symbol is a local variable
            //
            ////////////////////////////////////
            SPEW_OUTER_INTERPRETER( "Local variable {%s}\n", pToken );
            CompileOpcode( *pEntry );
            return OpResult::kOk;
        }
    }

    // check for member variables and methods
    if ( mCompileState && CheckFlag( kEngineFlagInClassDefinition ) )
    {
        if ( mpTypesManager->ProcessMemberSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a member variable or method
            //
            ////////////////////////////////////
            return exitStatus;
        }
    }

    pEntry = nullptr;
    if ( pInfo->GetFlags() & PARSE_FLAG_HAS_COLON )
    {
        if ( (len > 2) && (*pToken != ':') && (pToken[len - 1] != ':') )
        {
            ////////////////////////////////////
            //
            // symbol may be of form VOCABULARY:SYMBOL or LITERALTYPE:LITERALSTRING
            //
            ////////////////////////////////////
            char* pColon = strchr( pToken, ':' );
            *pColon = '\0';
            ForthVocabulary* pVocab = ForthVocabulary::FindVocabulary( pToken );
            if ( pVocab != nullptr )
            {
                pEntry = pVocab->FindSymbol( pColon + 1 );
                if ( pEntry != nullptr )
                {
                    pFoundVocab = pVocab;
                }
            }

            if (pEntry == nullptr)
            {
                pEntry = mpLiteralsVocab->FindSymbol(pToken);
                if (pEntry != nullptr)
                {
                    // push ptr to string after colon and invoke literal processing op
                    *--mpCore->SP = (cell)(pColon + 1);
                    exitStatus = mpEngine->FullyExecuteOp(mpCore, *pEntry);
                    return exitStatus;
                }
            }
            *pColon = ':';
        }
    }

    if ( pEntry == nullptr )
    {
#ifdef MAP_LOOKUP
        pEntry = mpVocabStack->FindSymbol( pToken, &pFoundVocab );
#else
        pEntry = mpVocabStack->FindSymbol( pInfo, &pFoundVocab );
#endif
    }

    if ( pEntry != nullptr )
    {
        ////////////////////////////////////
        //
        // symbol is a forth op
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Forth op {%s} in vocabulary %s\n", pToken, pFoundVocab->GetName() );
        return pFoundVocab->ProcessEntry( pEntry );
    }

    // see if this is a structure/object access (like structA.fieldB.fieldC)
    if ( pInfo->GetFlags() & PARSE_FLAG_HAS_PERIOD ) 
    {
        if ( mpTypesManager->ProcessSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a structure/object access
            //
            ////////////////////////////////////
            return exitStatus;
        }
    }

	// see if this is an array indexing op like structType[] or number[]
    if ( (len > 2) && (strcmp( "[]", &(pToken[len - 2]) ) == 0) )
    {
		// symbol ends with [], see if preceeding token is either a number or a structure type
		pToken[len - 2] = '\0';
		int elementSize = 0;
        ForthStructVocabulary* pStructVocab = mpTypesManager->GetStructVocabulary( pToken );
        if ( pStructVocab != nullptr )
        {
			elementSize = pStructVocab->GetSize();
        }
		else
		{
			ForthNativeType *pNative = mpTypesManager->GetNativeTypeFromName( pToken );
			if ( pNative != nullptr )
			{
				// string[] is not supported
				if ( pNative->GetBaseType() != BaseType::kString )
				{
					elementSize = pNative->GetSize();
				}
			}
			else if ( ScanIntegerToken( pToken, lvalue, mpCore->base, isOffset, isSingle ) && isSingle )
			{
				elementSize = (int)lvalue;
			}
		}
		pToken[len - 2] = '[';
		if ( elementSize > 0 )
		{
			// compile or execute 
			if ( mCompileState )
			{
	            CompileOpcode( kOpArrayOffset, elementSize );
			}
			else
			{
				// in interpret mode, stick the result of array indexing on the stack
				cell baseAddress = *mpCore->SP++;		// get base address
				*mpCore->SP = baseAddress + (elementSize * (*mpCore->SP));
			}
			return OpResult::kOk;
		}
    }

    // try to convert to a number
    // if there is a period in string
    //    try to covert to a floating point number
    // else
    //    try to convert to an integer
    if ( ((pInfo->GetFlags() & PARSE_FLAG_HAS_PERIOD) || !CheckFeature(kFFCFloatLiterals))
          && ScanFloatToken( pToken, fvalue, dvalue, isSingle, isApproximate ) )
    {
       if ( isSingle )
       {
          ////////////////////////////////////
          //
          // symbol is a single precision fp literal
          //
          ////////////////////////////////////
          SPEW_OUTER_INTERPRETER( "Floating point literal %f\n", fvalue );
          if ( mCompileState )
          {
              // compile the literal value
			  uint32_t squishedFloat;
			  if ( SquishFloat( fvalue, isApproximate, squishedFloat ) )
			  {
	              CompileOpcode( kOpSquishedFloat, squishedFloat );
			  }
			  else
			  {
				  CompileBuiltinOpcode( OP_FLOAT_VAL );
				  *(float *) mpDictionary->pCurrent++ = fvalue;
			  }
          }
          else
          {
              --mpCore->SP;
#if defined(FORTH64)
              *mpCore->SP = 0;
#endif
              *(float *) mpCore->SP = fvalue;
          }
       }
       else
       {
          ////////////////////////////////////
          //
          // symbol is a double precision fp literal
          //
          ////////////////////////////////////
          SPEW_OUTER_INTERPRETER( "Floating point double literal %g\n", dvalue );
          if ( mCompileState )
          {
              // compile the literal value
			  uint32_t squishedDouble;
			  if (  SquishDouble( dvalue, isApproximate, squishedDouble ) )
			  {
	              CompileOpcode( kOpSquishedDouble, squishedDouble );
			  }
			  else
			  {
				  CompileBuiltinOpcode( OP_DOUBLE_VAL );
				  pDPD = (double *) mpDictionary->pCurrent;
				  *pDPD++ = dvalue;
				  mpDictionary->pCurrent = (forthop *) pDPD;
			  }
          }
          else
          {
#if defined(FORTH64)
              mpCore->SP -= 1;
#else
              mpCore->SP -= 2;
#endif
              *(double *) mpCore->SP = dvalue;
          }
       }
        
    }
    else if ( ScanIntegerToken( pToken, lvalue, mpCore->base, isOffset, isSingle ) )
    {

        ////////////////////////////////////
        //
        // symbol is an integer literal
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Integer literal %lld\n", lvalue );
        ProcessConstant(lvalue, isOffset, isSingle);
    }
    else if ( CheckFlag( kEngineFlagInEnumDefinition ) )
    {
        // add symbol as an enumerated value
        if ( mpCore->SP < mpEnumStackBase )
        {
            // pop enum value off stack
            mNextEnum = *mpCore->SP++;
        }

        if ( (mNextEnum < (1 << 23)) && (mNextEnum >= -(1 << 23)) )
        {
            // number is in range supported by kOpConstant, just add it to vocabulary
            forthop enumOp = COMPILED_OP(kOpConstant, mNextEnum & 0x00FFFFFF);
            mpDefinitionVocab->AddSymbol(pToken, enumOp);
			forthop* pNewEnum = mpDefinitionVocab->GetNewestEntry();
		    pNewEnum[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
        }
        else
        {
            // number is out of range of kOpConstant, need to define a user op
            StartOpDefinition( pToken );
            CompileBuiltinOpcode( OP_DO_CONSTANT );
            CompileCell( mNextEnum );
        }
        mNextEnum++;
    }
    else if (CheckFeature(kFFAllowVaropSuffix))
    {
        ////////////////////////////////////
        //
        // last chance - is it something with a varop suffix?
        //
        ////////////////////////////////////
        VarOperation varop = pInfo->CheckVaropSuffix();
        if (varop != VarOperation::kVarDefaultOp)
        {
            pInfo->ChopVaropSuffix();

            if (mCompileState)
            {
                pEntry = mpLocalVocab->FindSymbol(pToken);
                if (pEntry)
                {
                    ////////////////////////////////////
                    //
                    // symbol is a local variable with a varop suffix
                    //
                    ////////////////////////////////////
                    SPEW_OUTER_INTERPRETER("Local variable with varop {%s%s}\n", pToken, ForthParseInfo::GetVaropSuffix(varop));
                    if (CompileLocalVariableOpcode(pEntry, varop))
                    {
                        return OpResult::kOk;
                    }
                }
                else
                {
                    if (CheckFlag(kEngineFlagInClassDefinition))
                    {
                        if (mpTypesManager->ProcessMemberSymbol(pInfo, exitStatus, varop))
                        {
                            ////////////////////////////////////
                            //
                            // symbol is a member variable with a varop suffix
                            //
                            ////////////////////////////////////
                            return exitStatus;
                        }
                    }
                }
            }

            // TODO: handle compile-mode global var with varop suffix
#ifdef MAP_LOOKUP
            pEntry = mpVocabStack->FindSymbol(pToken, &pFoundVocab);
#else
            pEntry = mpVocabStack->FindSymbol(pInfo, &pFoundVocab);
#endif
            if (pEntry != nullptr && pEntry[1] <= (uint32_t) BaseType::kObject)
            {
                ////////////////////////////////////
                //
                // symbol is a global variable with varop suffix
                //
                ////////////////////////////////////
                SPEW_OUTER_INTERPRETER("Global variable with varop {%s%s}\n", pToken, ForthParseInfo::GetVaropSuffix(varop));

                if (mCompileState)
                {
                    // compile NumOpCombo, op is setVarop, num is varop
                    uint32_t opVal = (OP_SETVAROP << 13) | ((uint32_t)varop);
                    CompileOpcode(kOpNOCombo, opVal);
                }
                else
                {
                    mpCore->varMode = varop;
                }

                return pFoundVocab->ProcessEntry(pEntry);
            }

            pInfo->UnchopVaropSuffix();
        }

        SPEW_ENGINE( "Unknown symbol %s\n", pToken );
		mpCore->error = ForthError::kUnknownSymbol;
		exitStatus = OpResult::kError;
    }

    // TODO: return exit-shell flag
    return exitStatus;
}



//############################################################################
