//////////////////////////////////////////////////////////////////////
//
// ForthEngine.cpp: implementation of the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

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
    extern eForthResult InnerInterp( ForthCoreState *pCore );
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
	if ((pEngine->GetTraceFlags() & kLogToConsole) != 0)
	{
#if defined(LINUX) || defined(MACOSX)
		vsnprintf(buffer, sizeof(buffer), pFormat, argList);
#else
		wvnsprintf(buffer, sizeof(buffer), pFormat, argList);
#endif

		ForthEngine::GetInstance()->ConsoleOut(buffer);
	}
	else
	{
#if defined(LINUX) || defined(MACOSX)
		vsnprintf(buffer, sizeof(buffer), pFormat, argList);
#else
		wvnsprintf(buffer, sizeof(buffer), pFormat, argList);
#endif
        OutputToLogger(buffer);
	}
}

extern "C"
{
	void traceOp(ForthCoreState* pCore, long* pIP, long op)
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
                    long* pMethods = thisObject->pMethods;
                    long opVal = FORTH_OP_VALUE(op);
                    SpewMethodName(pMethods, opVal);
                }
                else if (opType == kOpMethodWithTOS)
                {
                    ForthObject thisObject = (ForthObject)*(pCore->SP);
                    long* pMethods = thisObject->pMethods;
                    long opVal = FORTH_OP_VALUE(op);
                    SpewMethodName(pMethods, opVal);
                }
                else if (opType == kOpMethodWithSuper)
                {
                    ForthObject thisObject = GET_TP;
                    ForthClassObject* pClassObject = (ForthClassObject*)(thisObject->pMethods[-1]);
                    long* pSuperMethods = pClassObject->pVocab->ParentClass()->GetMethods();
                    long opVal = FORTH_OP_VALUE(op);
                    SpewMethodName(pSuperMethods, opVal);
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

ForthEngine* ForthEngine::mpInstance = NULL;

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
// pErrorStrings must be kept in sync with eForthError enum in forth.h
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
//                     ForthEngineTokenStack
// 

ForthEngineTokenStack::ForthEngineTokenStack()
	: mpCurrent(nullptr)
	, mpBase(nullptr)
	, mpLimit(nullptr)
	, mNumBytes(0)
{
}

ForthEngineTokenStack::~ForthEngineTokenStack()
{
	if (mpCurrent != nullptr)
	{
		__FREE(mpBase);
	}
}

void ForthEngineTokenStack::Initialize(ulong numBytes)
{
	mpBase = (char *)__MALLOC(numBytes);
	mpLimit = mpBase + numBytes;
	mpCurrent = mpLimit;
	mNumBytes = numBytes;
}

void ForthEngineTokenStack::Push(const char* pToken)
{
	ulong newTokenBytes = strlen(pToken) + 1;
	char* newCurrent = mpCurrent - newTokenBytes;
	if (newCurrent >= mpBase)
	{
		mpCurrent = newCurrent;
	}
	else
	{
		// resize stack to fit pushed token plus a little bit
		ulong currentBytes = mpLimit - mpCurrent;
		mNumBytes = newTokenBytes + 64 + (mNumBytes - currentBytes);
		mpBase = (char *)__REALLOC(mpBase, mNumBytes);
		mpLimit = mpBase + mNumBytes;
		mpCurrent = mpLimit - (currentBytes + newTokenBytes);
	}
	memcpy(mpCurrent, pToken, newTokenBytes);
}

char* ForthEngineTokenStack::Pop()
{
	char* pResult = nullptr;
	if (mpCurrent != mpLimit)
	{
		pResult = mpCurrent;
		ulong bytesToRemove = strlen(mpCurrent) + 1;
		mpCurrent += bytesToRemove;
	}

	return pResult;
}

char* ForthEngineTokenStack::Peek()
{
	char* pResult = nullptr;
	if (mpCurrent != mpLimit)
	{
		pResult = mpCurrent;
	}

	return pResult;
}

void ForthEngineTokenStack::Clear()
{
	mpCurrent = mpLimit;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthEngine
// 

ForthEngine::ForthEngine()
: mpForthVocab( NULL )
, mpLiteralsVocab(nullptr)
, mpLocalVocab( NULL )
, mpDefinitionVocab( NULL )
, mpStringBufferA( NULL )
, mpStringBufferANext( NULL )
, mStringBufferASize( 0 )
, mpTempBuffer(NULL)
, mpTempBufferLock(NULL)
, mpThreads(NULL)
, mpInterpreterExtension( NULL )
, mpMainThread( NULL )
, mFastMode( true )
, mNumElements( 0 )
, mpTypesManager( NULL )
, mpVocabStack( NULL )
, mpExtension( NULL )
, mpCore( NULL )
, mpShell( NULL )
, mTraceOutRoutine(defaultTraceOutRoutine)
, mpTraceOutData( NULL )
, mpOpcodeCompiler( NULL )
, mFeatures( kFFCCharacterLiterals | kFFMultiCharacterLiterals | kFFCStringLiterals
            | kFFCHexLiterals | kFFDoubleSlashComment | kFFCFloatLiterals | kFFParenIsExpression)
, mBlockFileManager( NULL )
, mIsServer(false)
, mContinuationIx(0)
, mContinueDestination(nullptr)
, mContinueCount(0)
, mpNewestEnum(nullptr)
{
    // scratch area for temporary definitions
    ASSERT( mpInstance == NULL );
    mpInstance = this;
    mpEngineScratch = new long[70];
    mpErrorString = new char[ ERROR_STRING_MAX + 1 ];

    initMemoryAllocation();

    // remember creation time for elapsed time method
#ifdef WIN32
#ifdef MSDEV
    _ftime32_s( &mStartTime );
#else
    _ftime( &mStartTime );
#endif
#else
    ftime( &mStartTime );
#endif

	mDefaultConsoleOutStream = nullptr;

	mDictionary.pBase = nullptr;

    // At this point, the main thread does not exist, it will be created later in Initialize, this
    // is fairly screwed up, it is becauses originally ForthEngine was the center of the universe,
    // and it created the shell, but now the shell is created first, and the shell or the main app
    // can create the engine, and then the shell calls ForthEngine::Initialize to hook the two up.
    // The main thread needs to get the file interface from the shell, so it can't be created until
    // after the engine is connected to the shell.  Did I mention its screwed up?
}

ForthEngine::~ForthEngine()
{
    CleanupGlobalObjectVariables(nullptr);

    if ( mpExtension != nullptr)
    {
        mpExtension->Shutdown();
    }
	
    if (mpTypesManager != nullptr)
    {
        mpTypesManager->ShutdownBuiltinClasses(this);
        delete mpTypesManager;
    }

    if (mDictionary.pBase)
    {
#ifdef WIN32
		VirtualFree( mDictionary.pBase, 0, MEM_RELEASE );
#elif MACOSX
        munmap(mDictionary.pBase, mDictionary.len * sizeof(long));
#else
        __FREE( mDictionary.pBase );
#endif
		delete mpForthVocab;
        delete mpLiteralsVocab;
        delete mpLocalVocab;
		delete mpOpcodeCompiler;
        delete [] mpStringBufferA;
        delete [] mpTempBuffer;
    }
    delete [] mpErrorString;

    if (mpTempBufferLock != nullptr)
    {
#ifdef WIN32
        DeleteCriticalSection(mpTempBufferLock);
#else
        pthread_mutex_destroy(mpTempBufferLock);
#endif
        delete mpTempBufferLock;
    }

    ForthForgettable* pForgettable = ForthForgettable::GetForgettableChainHead();
    while ( pForgettable != NULL )
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
	ForthAsyncThread *pThread = mpThreads;
	while (pThread != NULL)
    {
		ForthAsyncThread *pNextThread = pThread->mpNext;
		delete pThread;
		pThread = pNextThread;
    }

    delete mpEngineScratch;

    delete mpVocabStack;

    delete mBlockFileManager;

	mpInstance = NULL;
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

void
ForthEngine::Initialize( ForthShell*        pShell,
                         int                totalLongs,
                         bool               bAddBaseOps,
                         ForthExtension*    pExtension )
{
    mpShell = pShell;

    mBlockFileManager = new ForthBlockFileManager(mpShell->GetBlockfilePath());

    size_t dictionarySize = totalLongs * sizeof(long);
#ifdef WIN32
	void* dictionaryAddress = NULL;
	// we need to allocate memory that is immune to Data Execution Prevention
	mDictionary.pBase = (long *) VirtualAlloc( dictionaryAddress, dictionarySize, (MEM_COMMIT | MEM_RESERVE), PAGE_EXECUTE_READWRITE );
#elif MACOSX
    mDictionary.pBase = (long *) mmap(NULL, dictionarySize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
#else
	 mDictionary.pBase = (long *) __MALLOC( dictionarySize );
#endif
    mDictionary.pCurrent = mDictionary.pBase;
    mDictionary.len = totalLongs;

	mTokenStack.Initialize(4);
	
	mpOpcodeCompiler = new ForthOpcodeCompiler( &mDictionary );

	if (mpTypesManager == nullptr)
	{
		mpTypesManager = new ForthTypesManager();
	}

    mpForthVocab = new ForthVocabulary("forth", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpLiteralsVocab = new ForthVocabulary("literals", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpLocalVocab = new ForthLocalVocabulary( "locals", NUM_LOCALS_VOCAB_VALUE_LONGS );
	mStringBufferASize = 3 *  MAX_STRING_SIZE;
    mpStringBufferA = new char[mStringBufferASize];
    mpTempBuffer = new char[MAX_STRING_SIZE];

    mpMainThread = CreateAsyncThread( 0, MAIN_THREAD_PSTACK_LONGS, MAIN_THREAD_RSTACK_LONGS );
	mpCore = mpMainThread->GetThread(0)->GetCore();
	mpCore->optypeAction = (optypeActionRoutine *) __MALLOC(sizeof(optypeActionRoutine) * 256);
    mpCore->numBuiltinOps = 0;
    mpCore->numOps = 0;
    mpCore->maxOps = MAX_BUILTIN_OPS;
	mpCore->ops = (long **) __MALLOC(sizeof(long *) * mpCore->maxOps);

    mpVocabStack = new ForthVocabularyStack;
    mpVocabStack->Initialize();
    mpVocabStack->Clear();
    mpDefinitionVocab = mpForthVocab;


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
        mpTypesManager->AddBuiltinClasses( this );
    }

	// the primary thread objects can't be inited until builtin classes are initialized
	OThread::FixupAsyncThread(mpMainThread);

    if ( pExtension != NULL )
    {
        mpExtension = pExtension;
        mpExtension->Initialize( this );
    }

	GetForthConsoleOutStream( mpCore, mDefaultConsoleOutStream );
    ResetConsoleOut( mpCore );

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

void
ForthEngine::Reset( void )
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

    if (mpExtension != NULL)
    {
        mpExtension->Reset();
    }
}

void
ForthEngine::ErrorReset( void )
{
    Reset();
	mpMainThread->Reset();
}


// add an op to engine dispatch table
long
ForthEngine::AddOp( const long *pOp )
{
    long newOp;

    if ( mpCore->numOps == mpCore->maxOps )
    {
        mpCore->maxOps += 128;
        mpCore->ops = (long **) realloc( mpCore->ops, sizeof(long *) * mpCore->maxOps );
    }
    mpCore->ops[ mpCore->numOps++ ] = (long *) pOp;

    newOp = mpCore->numOps - 1;

	return newOp;
}


// add an op to dictionary and corresponding symbol to current vocabulary
long
ForthEngine::AddUserOp( const char *pSymbol, long** pEntryOut, bool smudgeIt )
{
    AlignDP();
    long* pEntry = mpDefinitionVocab->AddSymbol( pSymbol, kOpUserDef, (long) mDictionary.pCurrent, true );
	if ( pEntryOut != NULL )
	{
		*pEntryOut = pEntry;
	}
    if ( smudgeIt )
    {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return mpCore->numOps - 1;
}


long*
ForthEngine::AddBuiltinOp( const char* name, ulong flags, ulong value )
{
    // AddSymbol will call ForthEngine::AddOp to add the operators to op table
    long *pEntry = mpDefinitionVocab->AddSymbol( name, flags, value, true );

//#ifdef TRACE_INNER_INTERPRETER
    // add built-in op names to table for TraceOp
	int index = mpCore->numOps - 1;
    if ( index < NUM_TRACEABLE_OPS )
    {
        gOpNames[index] = name;
    }
	mpCore->numBuiltinOps = mpCore->numOps;
//#endif
	return pEntry;
}


void
ForthEngine::AddBuiltinOps( baseDictionaryEntry *pEntries )
{
    // I think this assert is a holdover from when userOps and builtinOps were in a single dispatch table
    // assert if this is called after any user ops have been defined
    //ASSERT( mpCore->numUserOps == 0 );

    while ( pEntries->value != NULL )
    {

        AddBuiltinOp( pEntries->name, pEntries->flags, pEntries->value);
        pEntries++;
		//mpCore->numOps++;
    }
}


ForthClassVocabulary*
ForthEngine::StartClassDefinition(const char* pClassName, eBuiltinClassIndex classIndex)
{
    SetFlag( kEngineFlagInStructDefinition );
    SetFlag( kEngineFlagInClassDefinition );
	
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	ForthClassVocabulary* pVocab = pManager->StartClassDefinition(pClassName, classIndex);

	// add new class vocab to top of search order
	mpVocabStack->DupTop();
	mpVocabStack->SetTop( pVocab );

    CompileBuiltinOpcode( OP_DO_CLASS_TYPE );
    CompileLong( (long) pVocab );

    return pVocab;
}

void
ForthEngine::EndClassDefinition()
{
	ClearFlag( kEngineFlagInStructDefinition );
    ClearFlag( kEngineFlagInClassDefinition );

    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	pManager->EndClassDefinition();
	mpVocabStack->DropTop();
}

ForthClassVocabulary*
ForthEngine::AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry *pEntries)
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
    while ( pEntries->name != NULL )
    {
        const char* pMemberName = pEntries->name;
        ulong entryType = pEntries->returnType;
        if (CODE_IS_METHOD(entryType))
        {
            if ( !strcmp( pMemberName, "__newOp" ) )
            {
                // this isn't a new method, it is the class constructor op
                long* pEntry = AddBuiltinOp( pMemberName, kOpCCode, pEntries->value );
				pEntry[1] = kBaseTypeUserDefinition;
                pVocab->GetClassObject()->newOp = *pEntry;
            }
            else
            {
                // this entry is a member method
                // add method routine to builtinOps table
                long methodOp = gCompiledOps[OP_BAD_OP];
                // do "method:"
                long methodIndex = pVocab->FindMethod( pMemberName );
                StartOpDefinition( pMemberName, false );
                long* pEntry = pVocab->GetNewestEntry();
                methodOp = FORTH_OP_VALUE(*pEntry);
                methodOp = COMPILED_OP(kOpCCode, methodOp);
                if (pEntries->value != NULL)
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
                pEntry[1] = pEntries->returnType;
                mpCore->ops[mpCore->numOps - 1] = (long *)(pEntries->value);

                SPEW_ENGINE( "Method %s op is 0x%x\n", pMemberName, methodOp );

                // do ";method"
                EndOpDefinition( false );
            }
        }
        else
        {
            ulong baseType = CODE_TO_BASE_TYPE(entryType);
            if (baseType == kBaseTypeUserDefinition)
            {
                // forth op defined within class
                if (CODE_IS_FUNKY(entryType))
                {
                    // class op with precedence
                    long* pEntry = AddBuiltinOp(pMemberName, kOpCCodeImmediate, pEntries->value);
                    pEntry[1] = kBaseTypeUserDefinition;
                }
                else
                {
                    // class op
                    long* pEntry = AddBuiltinOp(pMemberName, kOpCCode, pEntries->value);
                    pEntry[1] = kBaseTypeUserDefinition;
                }
            }
            else
            {
                // this entry is a member variable
                pManager->GetNewestStruct()->AddField(pMemberName, pEntries->returnType, (int)pEntries->value);
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
ForthEngine::ForgetOp( ulong opNumber, bool quietMode )
{
    if ( opNumber < mpCore->numOps )
    {
        long* pNewDP = mpCore->ops[opNumber];
        CleanupGlobalObjectVariables(pNewDP);
        mDictionary.pCurrent = pNewDP;
        mpCore->numOps = opNumber;
    }
    else
    {
        if ( !quietMode )
        {
            SPEW_ENGINE( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mpCore->numOps );
            printf( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mpCore->numOps );
        }
    }
    if ( mpExtension != NULL )
    {
        mpExtension->ForgetOp( opNumber );
    }
}

// return true if symbol was found
bool
ForthEngine::ForgetSymbol( const char *pSym, bool quietMode )
{
    long *pEntry = NULL;
    long op;
    forthOpType opType;
    bool forgotIt = false;
    char buff[256];
    buff[0] = '\0';

    ForthVocabulary* pFoundVocab = NULL;
    pEntry = GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pFoundVocab != NULL )
    {
        op = ForthVocabulary::GetEntryValue( pEntry );
        opType = ForthVocabulary::GetEntryType( pEntry );
		unsigned long opIndex = ((unsigned long)FORTH_OP_VALUE( op ));
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
					ForthForgettable::ForgetPropagate( mDictionary.pCurrent, op );
					forgotIt = true;
				}
				else
				{
					// sym is built-in op - no way
					SNPRINTF( buff, sizeof(buff), "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
				}
                break;

            default:
                const char* pStr = GetOpTypeName( opType );
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
ForthEngine::ShowSearchInfo()
{
	ForthVocabularyStack* pVocabStack = GetVocabularyStack();
	int depth = 0;
	ForthConsoleStringOut(mpCore, "vocab stack:");
	while (true)
	{
		ForthVocabulary* pVocab = pVocabStack->GetElement(depth);
		if (pVocab == NULL)
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

ForthAsyncThread *
ForthEngine::CreateAsyncThread( long threadOp, int paramStackSize, int returnStackSize )
{
	ForthAsyncThread *pAsyncThread = new ForthAsyncThread(this, paramStackSize, returnStackSize);
	ForthThread *pNewThread = pAsyncThread->GetThread(0);
	pNewThread->SetOp(threadOp);

    pNewThread->mCore.pEngine = this;
    pNewThread->mCore.pDictionary = &mDictionary;
    pNewThread->mCore.pFileFuncs = mpShell->GetFileInterface();

    if ( mpCore != NULL )
    {
        // fill in optype & opcode action tables from engine thread
        pNewThread->mCore.optypeAction = mpCore->optypeAction;
        pNewThread->mCore.numBuiltinOps = mpCore->numBuiltinOps;
        pNewThread->mCore.numOps = mpCore->numOps;
        pNewThread->mCore.maxOps = mpCore->maxOps;
        pNewThread->mCore.ops = mpCore->ops;
        pNewThread->mCore.innerLoop = mpCore->innerLoop;
        pNewThread->mCore.innerExecute = mpCore->innerExecute;
    }

	pAsyncThread->mpNext = mpThreads;
	mpThreads = pAsyncThread;

	return pAsyncThread;
}


void
ForthEngine::DestroyAsyncThread(ForthAsyncThread *pThread)
{
	ForthAsyncThread *pNext, *pCurrent;

    if ( mpThreads == pThread )
    {

        // special case - thread is head of list
		mpThreads = (ForthAsyncThread *)(mpThreads->mpNext);
        delete pThread;

    }
    else
    {

        // TODO: this is untested
        pCurrent = mpThreads;
        while ( pCurrent != NULL )
        {
			pNext = (ForthAsyncThread *)(pCurrent->mpNext);
            if ( pThread == pNext )
            {
                pCurrent->mpNext = pNext->mpNext;
                delete pThread;
                return;
            }
            pCurrent = pNext;
        }

        SPEW_ENGINE( "ForthEngine::DestroyThread tried to destroy unknown thread 0x%x!\n", pThread );
        // TODO: raise the alarm
    }
}


char *
ForthEngine::GetNextSimpleToken( void )
{
	return mTokenStack.IsEmpty() ? mpShell->GetNextSimpleToken() : mTokenStack.Pop();
}


// interpret named file, interpret from standard in if pFileName is NULL
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
ForthEngine::PushInputBlocks(ForthBlockFileManager* pManager, unsigned int firstBlock, unsigned int lastBlock)
{
    mpShell->PushInputBlocks(pManager, firstBlock, lastBlock);
}


void
ForthEngine::PopInputStream( void )
{
    mpShell->PopInputStream();
}



long *
ForthEngine::StartOpDefinition(const char *pName, bool smudgeIt, forthOpType opType, ForthVocabulary* pDefinitionVocab)
{
    mpLocalVocab->Empty();
    mpLocalVocab->ClearFrame();
    //mLocalFrameSize = 0;
    //mpLocalAllocOp = NULL;
    mpOpcodeCompiler->ClearPeephole();
    AlignDP();

    if (pDefinitionVocab == nullptr)
    {
        pDefinitionVocab = mpDefinitionVocab;
    }

    if ( pName == NULL )
    {
        pName = GetNextSimpleToken();
    }
    long* pEntry = pDefinitionVocab->AddSymbol(pName, opType, (long)mDictionary.pCurrent, true);
    if ( smudgeIt )
    {
        pDefinitionVocab->SmudgeNewestSymbol();
    }
	mLabels.clear();

    return pEntry;
}


void
ForthEngine::EndOpDefinition( bool unsmudgeIt )
{
	long* pLocalAllocOp = mpLocalVocab->GetFrameAllocOpPointer();
    if ( pLocalAllocOp != NULL )
    {
        int nLongs = mpLocalVocab->GetFrameLongs();
	    long op = COMPILED_OP( kOpAllocLocals, nLongs );
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


long *
ForthEngine::FindSymbol( const char *pSymName )
{
    ForthVocabulary* pFoundVocab = NULL;
    return GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
}

void
ForthEngine::DescribeOp( const char* pSymName, long op, long auxData )
{
    char buff[256];
    char buff2[128];
    char c;
    int line = 1;
    bool notDone = true;

    long opType = FORTH_OP_TYPE( op );
    long opValue = FORTH_OP_VALUE( op );
    bool isUserOp = (opType == kOpUserDef) || (opType == kOpUserDefImmediate);
    const char* pStr = GetOpTypeName( opType );
    if ( isUserOp )
    {
        ForthStructVocabulary::TypecodeToString( auxData, buff2, sizeof(buff2) );
        SNPRINTF( buff, sizeof(buff), "%s: type %s:%x value 0x%x 0x%x (%s) \n", pSymName, pStr, opValue, op, auxData, buff2 );
    }
    else
    {
        SNPRINTF( buff, sizeof(buff), "%s: type %s:%x value 0x%x 0x%x \n", pSymName, pStr, opValue, op, auxData );
    }
    ConsoleOut( buff );
    if ( isUserOp )
    {
        // disassemble the op until IP reaches next newer op
        long* curIP = mpCore->ops[ opValue ];
		long* baseIP = curIP;
        long* endIP = (opValue == (mpCore->numOps - 1)) ? GetDP() : mpCore->ops[ opValue + 1 ];
        if (*curIP == gCompiledOps[OP_DO_ENUM])
        {
            ForthEnumInfo* pEnumInfo = (ForthEnumInfo *)(curIP + 1);
            ForthVocabulary* pVocab = pEnumInfo->pVocab;
            long numEnums = pEnumInfo->numEnums;
            long* pEntry = pVocab->GetEntriesEnd() - pEnumInfo->vocabOffset;
            SNPRINTF(buff, sizeof(buff), "Enum size %d entries %d\n", pEnumInfo->size, numEnums);
            ConsoleOut(buff);
            for (int i = 0; i < numEnums; ++i)
            {
                char* pEnumName = AddTempString(pVocab->GetEntryName(pEntry), pVocab->GetEntryNameLength(pEntry));
                SNPRINTF(buff, sizeof(buff), "%d %s\n", *pEntry & 0xFFFFFF, pEnumName);
                ConsoleOut(buff);
                pEntry = pVocab->NextEntry(pEntry);
            }
        }
        else
        {
            while ((curIP < endIP) && notDone)
            {
                SNPRINTF(buff, sizeof(buff), "  +%04x  %08x  ", (curIP - baseIP), curIP);
                ConsoleOut(buff);
                DescribeOp(curIP, buff, sizeof(buff), true);
                ConsoleOut(buff);
                SNPRINTF(buff, sizeof(buff), "\n");
                ConsoleOut(buff);
                if (((line & 31) == 0) && (mpShell != NULL) && mpShell->GetInput()->InputStream()->IsInteractive())
                {
                    ConsoleOut("Hit ENTER to continue, 'q' & ENTER to quit\n");
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
ForthEngine::DescribeSymbol( const char *pSymName )
{
    long *pEntry = NULL;
    char buff[256];

    ForthVocabulary* pFoundVocab = NULL;
    pEntry = GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
    if ( pEntry )
    {
		DescribeOp( pSymName, pEntry[0], pEntry[1] );
    }
    else
    {
        SNPRINTF( buff, sizeof(buff), "Symbol %s not found\n", pSymName );
        SPEW_ENGINE( buff );
        ConsoleOut( buff );
    }
}

long *
ForthEngine::NextOp( long *pOp )
{
    long op = *pOp++;
    long opType = FORTH_OP_TYPE( op );
    long opVal = FORTH_OP_VALUE( op );

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
ForthEngine::StartStructDefinition( void )
{
    mCompileFlags |= kEngineFlagInStructDefinition;
}

void
ForthEngine::EndStructDefinition( void )
{
    mCompileFlags &= (~kEngineFlagInStructDefinition);
}

long
ForthEngine::AddLocalVar( const char        *pVarName,
                          long              typeCode,
                          long              varSize )
{
    long *pEntry;
	int frameLongs = mpLocalVocab->GetFrameLongs();
    varSize = ((varSize + 3) & ~3) >> 2;
    long baseType = CODE_TO_BASE_TYPE( typeCode );
    long fieldType = kOpLocalInt;
    if ( !CODE_IS_PTR( typeCode ) )
    {
        if ( baseType <= kBaseTypeObject )
        {
            fieldType = kOpLocalByte + baseType;
        }
        else
        {
            fieldType = kOpLocalRef;
        }
    }
    pEntry = mpLocalVocab->AddVariable( pVarName, fieldType, frameLongs + varSize, varSize );
    pEntry[1] = typeCode;
    if (frameLongs == 0)
    {
        if (mpShell->GetShellStack()->PeekTag() != kShellTagDefine)
        {
            SetError(kForthErrorBadSyntax, "First local variable definition inside control structure");
        }
        else
        {
            // this is first local var definition, leave space for local alloc op
            CompileLong(0);
            ClearPeephole();
        }
    }

    return mpLocalVocab->GetFrameLongs();
}

long
ForthEngine::AddLocalArray( const char          *pArrayName,
                            long                typeCode,
                            long                elementSize )
{
    long *pEntry;
	int frameLongs = mpLocalVocab->GetFrameLongs();

    long elementType = CODE_TO_BASE_TYPE( typeCode );
    if ( elementType != kBaseTypeStruct )
    {
        // array of non-struct
        long arraySize = elementSize * mNumElements;
        arraySize = ((arraySize + 3) & ~3) >> 2;
        long opcode = CODE_IS_PTR(typeCode) ? kOpLocalIntArray : (kOpLocalByteArray + CODE_TO_BASE_TYPE(typeCode));
        pEntry = mpLocalVocab->AddVariable( pArrayName, opcode, frameLongs + arraySize, arraySize );
    }
    else
    {
        // array of struct
        long fieldBytes, alignment, padding, alignMask;
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        pManager->GetFieldInfo( typeCode, fieldBytes, alignment );
        alignMask = alignment - 1;
        padding = fieldBytes & alignMask;
        long paddedSize = (padding) ? (fieldBytes + (alignment - padding)) : fieldBytes;
        long arraySize = paddedSize * mNumElements;
        if ( CODE_IS_PTR(typeCode) )
        {
	        pEntry = mpLocalVocab->AddVariable( pArrayName, kOpLocalIntArray, frameLongs + arraySize, arraySize );
        }
        else
        {
	        pEntry = mpLocalVocab->AddVariable( pArrayName, kOpLocalStructArray, ((frameLongs + arraySize) << 12) + paddedSize, arraySize );
        }
    }
    if ( frameLongs == 0 )
    {
        // this is first local var definition, leave space for local alloc op
        CompileLong( 0 );
    }

	pEntry[1] = typeCode;

    mNumElements = 0;
    return mpLocalVocab->GetFrameLongs();
}

bool
ForthEngine::HasLocalVariables()
{
	return mpLocalVocab->GetFrameLongs() != 0;
}

// TODO: tracing of built-in ops won't work for user-added builtins...
const char *
ForthEngine::GetOpTypeName( long opType )
{
    return (opType < kOpLocalUserDefined) ? opTypeNames[opType] : "unknown";
}

static bool lookupUserTraces = true;


void
ForthEngine::TraceOut(const char* pFormat, ...)
{
	if (mTraceOutRoutine != NULL)
	{
		va_list argList;
		va_start(argList, pFormat);

		mTraceOutRoutine(mpTraceOutData, pFormat, argList);

		va_end(argList);
	}
}


void
ForthEngine::SetTraceOutRoutine( traceOutRoutine traceRoutine, void* pTraceData )
{
	mTraceOutRoutine = traceRoutine;
	mpTraceOutData = pTraceData;
}

void
ForthEngine::GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const
{
	traceRoutine = mTraceOutRoutine;
	pTraceData = mpTraceOutData;
}

void
ForthEngine::TraceOp(long* pOp, long op)
{
#ifdef TRACE_INNER_INTERPRETER
    char buff[ 512 ];
#if 0
    int rDepth = pCore->RT - pCore->RP;
    char* sixteenSpaces = "                ";     // 16 spaces
	//if ( *pOp != gCompiledOps[OP_DONE] )
	{
		DescribeOp(pOp, buff, sizeof(buff), lookupUserTraces);
		TraceOut("# 0x%08x #", pOp);
		while (rDepth > 16)
		{
			TraceOut(sixteenSpaces);
			rDepth -= 16;
		}
		char* pIndent = sixteenSpaces + (16 - rDepth);
		TraceOut("%s%s # ", pIndent, buff);
	}
#else
    long opIn;
    if (pOp == nullptr)
    {
        // this is used for indirect op execution, where the op to be executed
        //  isn't pointed to by the IP, things like 'execute' or methods
        opIn = op;
        pOp = &opIn;
    }
    DescribeOp(pOp, buff, sizeof(buff), lookupUserTraces);
    TraceOut("# 0x%08x # %s # ", pOp, buff);
#endif
#endif
}

void
ForthEngine::TraceStack( ForthCoreState* pCore )
{
	int i;

    long *pSP = GET_SP;
    int nItems = GET_SDEPTH;
    TraceOut("  stack[%d]:", nItems);
#define MAX_TRACE_STACK_ITEMS 8
#if defined(WIN32)
	int numToDisplay = min(MAX_TRACE_STACK_ITEMS, nItems);
#else
	int numToDisplay = std::min(MAX_TRACE_STACK_ITEMS, nItems);
#endif
	for (i = 0; i < numToDisplay; i++)
	{
		TraceOut( " %x", *pSP++ );
	}
	if (nItems > numToDisplay)
	{
		TraceOut(" <%d more>", nItems - numToDisplay);
	}

    long *pRP = GET_RP;
    nItems = pCore->RT - pRP;
    TraceOut("  rstack[%d]", nItems);
#define MAX_TRACE_RSTACK_ITEMS 8
#if defined(WIN32)
    numToDisplay = min(MAX_TRACE_RSTACK_ITEMS, nItems);
#else
    numToDisplay = std::min(MAX_TRACE_RSTACK_ITEMS, nItems);
#endif
    for (i = 0; i < numToDisplay; i++)
    {
        TraceOut(" %x", *pRP++);
    }
    if (nItems > numToDisplay)
    {
        TraceOut(" <%d more>", nItems - numToDisplay);
    }
}

void
ForthEngine::DescribeOp( long *pOp, char *pBuffer, int buffSize, bool lookupUserDefs )
{
    long op = *pOp;
    forthOpType opType = FORTH_OP_TYPE( op );
    ulong opVal = FORTH_OP_VALUE( op );
    ForthVocabulary *pFoundVocab = NULL;
    long *pEntry = NULL;

	const char* preamble = "%02x:%06x    ";
	int preambleSize = strlen( preamble );
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
                if ( (opVal < NUM_TRACEABLE_OPS) && (gOpNames[opVal] != NULL) )
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
						SNPRINTF( pBuffer, buffSize, "%s 0x%llx", gOpNames[opVal], *((long long *)(&(pOp[1]))) );
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
            case kOpMemberByte:          case kOpMemberByteArray:
            case kOpMemberShort:         case kOpMemberShortArray:
            case kOpMemberInt:           case kOpMemberIntArray:
            case kOpMemberFloat:         case kOpMemberFloatArray:
            case kOpMemberDouble:        case kOpMemberDoubleArray:
            case kOpMemberString:        case kOpMemberStringArray:
            case kOpMemberOp:            case kOpMemberOpArray:
            case kOpMemberLong:          case kOpMemberLongArray:
            case kOpMemberObject:        case kOpMemberObjectArray:
            case kOpMemberUByte:         case kOpMemberUByteArray:
            case kOpMemberUShort:        case kOpMemberUShortArray:
            {
                if ((opVal & 0xE00000) != 0)
                {
                    int varOp = opVal >> 21;
                    SNPRINTF(pBuffer, buffSize, "%s %s_%x", gOpNames[(OP_FETCH - 1) + varOp], opTypeName, (opVal & 0x1FFFFF));
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
                SNPRINTF( pBuffer, buffSize, "%s    0x%08x", opTypeName, opVal + 1 + pOp );
                break;

            case kOpOZBCombo:  case kOpONZBCombo:
            {
                const char* pBranchType = (opType == kOpOZBCombo) ? "BranchFalse" : "BranchTrue";
                long embeddedOp = opVal & 0xFFF;
                long branchOffset = opVal >> 12;
                if (opVal & 0x800000)
                {
                    branchOffset |= 0xFFFFF000;
                }
                SNPRINTF(pBuffer, buffSize, "%s   %s   %s 0x%08x", opTypeName,
                    gOpNames[embeddedOp], pBranchType, branchOffset + 1 + pOp);
                break;
            }

            case kOpLocalRefOpCombo:  case kOpMemberRefOpCombo:
            {
                const char* pVarType = (opType == kOpLocalRefOpCombo) ? "Local" : "Member";
                long varOffset = opVal & 0xFFF;
                long embeddedOp = opVal >> 12;
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
                SNPRINTF( pBuffer, buffSize, "%s    longs %d", opTypeName, opVal );
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
				SNPRINTF( pBuffer, buffSize, "%s %f", opTypeName, UnsquishFloat( opVal ) );
				break;

            case kOpSquishedDouble:
				SNPRINTF( pBuffer, buffSize, "%s %g", opTypeName, UnsquishDouble( opVal ) );
				break;

            case kOpSquishedLong:
				SNPRINTF( pBuffer, buffSize, "%s %lld", opTypeName, UnsquishLong( opVal ) );
				break;

            default:
                if ( opType >= (unsigned int)(sizeof(opTypeNames) / sizeof(char *)) )
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
            pEntry = GetVocabularyStack()->FindSymbolByValue(op, &pFoundVocab);
            if (pEntry == NULL)
            {
                ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
                while (pVocab != NULL)
                {
                    pEntry = pVocab->FindSymbolByValue(op);
                    if (pEntry != NULL)
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

void ForthEngine::AddOpExecutionToProfile(long op)
{
    forthOpType opType = FORTH_OP_TYPE(op);
    ulong opVal = FORTH_OP_VALUE(op);

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
    while (pVocab != NULL)
    {
        long *pEntry = pVocab->GetNewestEntry();
        if (pVocab->IsClass())
        {

            long* pMethods = ((ForthClassVocabulary *)pVocab)->GetMethods();
            while (pEntry != nullptr)
            {
                long op = *pEntry;
                long typeCode = pEntry[1];
                if (CODE_IS_METHOD(typeCode))
                {
                    ulong opVal = FORTH_OP_VALUE(op);
                    long methodOp = pMethods[opVal];
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
                long op = *pEntry;
                forthOpType opType = FORTH_OP_TYPE(op);
                ulong opVal = FORTH_OP_VALUE(op);

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
        long op = opInfo.op;
        if (op == 0)
        {
            op = i;
        }

        ForthVocabulary *pVocab = opInfo.pVocabulary;
        const char* pVocabName = (pVocab != nullptr) ? pVocab->GetName() : "UNKNOWN_VOCABULARY";

        const char* pOpName = "UNKNOWN_OP";
        long *pEntry = opInfo.pEntry;
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
ForthEngine::AddOpType( forthOpType opType, optypeActionRoutine opAction )
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
ForthEngine::GetLastInputToken( void )
{
    return mpLastToken;
}


// return true IFF token is an integer literal
// sets isOffset if token ended with a + or -
// NOTE: temporarily modifies string @pToken
bool
ForthEngine::ScanIntegerToken( char         *pToken,
                               long         &value,
                               long long    &lvalue,
                               int          base,
                               bool         &isOffset,
                               bool&        isSingle )
{
    long digit;
    bool isNegative;
    char c;
    int digitsFound = 0;
    int len = strlen( pToken );
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
        pLastChar = NULL;
    }

    // see if this is ANSI style double precision int
    if ( periodMeansDoubleInt )
    {
        isSingle = (strchr( pToken, '.' ) == NULL);
    }

    if ( (pToken[0] == '$') && CheckFeature(kFFDollarHexLiterals) )
    {
        if ( sscanf( pToken + 1, "%x", &value ) == 1 )
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
        if ( isSingle )
        {
            if ( sscanf( pToken + 2, "%x", &value ) == 1 )
            {
                if ( isNegative )
                {
                    value = 0 - value;
                }
                isValid = true;
            }
        }
        else
        {
#if defined(WIN32)
            if ( sscanf( pToken + 2, "%I64x", &lvalue ) == 1 )
#else
            if ( sscanf( pToken + 2, "%llx", &lvalue ) == 1 )
#endif
            {
                if ( isNegative )
                {
                    lvalue = 0 - lvalue;
                }
                isValid = true;
            }
        }
    }

    if ( !isValid )
    {

        isValid = true;
        value = 0;
        lvalue = 0;
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
            if ( isSingle )
            {
                value = (value * base) + digit;
            }
            else
            {
                lvalue = (lvalue * base) + digit;
            }
        }

        if ( digitsFound == 0 )
        {
            isValid = false;
        }

        // all chars were valid digits
        if ( isNegative )
        {
            if ( isSingle )
            {
                value = 0 - value;
            }
            else
            {
                lvalue = 0 - lvalue;
            }
        }
    }

    // restore original last char
    if ( pLastChar != NULL )
    {
        *pLastChar = lastChar;
    }
    return isValid;
}


// return true IFF token is a real literal
// sets isSingle to tell if result is a float or double
// NOTE: temporarily modifies string @pToken
bool ForthEngine::ScanFloatToken( char *pToken, float& fvalue, double& dvalue, bool& isSingle, bool& isApproximate )
{
   bool retVal = false;

   isApproximate = false;
   // a leading tilde means that value may be approximated with lowest precision 
   if ( *pToken == '~' )
   {
	   isApproximate = true;
	   pToken++;
   }

   int len = strlen( pToken );
   if ( CheckFeature( kFFCFloatLiterals ) )
   {
       if ( strchr( pToken, '.' ) == NULL )
       {
          return false;
       }
   }
   else
   {
       if ( (strchr( pToken, 'e' ) == NULL) && (strchr( pToken, 'E' ) == NULL) )
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
      if ( sscanf( pToken, "%f", &fvalue ) == 1)
      {
         retVal = true;
         isSingle = true;
      }
      *pLastChar = lastChar;
      break;
   default:
       if ( sscanf( pToken, "%f", &fvalue ) == 1)
        {
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
ForthEngine::SquishFloat( float fvalue, bool approximateOkay, ulong& squishedFloat )
{
	// single precision format is 1 sign, 8 exponent, 23 mantissa
	ulong inVal = *(reinterpret_cast<ulong *>( &fvalue ));

	// if bottom 5 bits of inVal aren't 0, number can't be exactly represented in squished format
	if ( !approximateOkay && ((inVal & 0x1f) != 0) )
	{
		// number can't be represented exactly
		return false;
	}
    ulong sign = (inVal & 0x80000000) >> 8;
	long exponent = (((inVal >> 23) & 0xff) - (127 - 15));
	// if exponent is less than 0 or greater than 31, number can't be represented in squished format at all
	if ( (exponent < 0) || (exponent > 31) )
	{
		return false;
	}
	ulong mantissa = (inVal >> 5) & 0x3ffff;
	squishedFloat = sign | (exponent << 18) | mantissa;

	return true;
}

// squish double down to 24-bits, returns true IFF number can be represented exactly
//   OR approximateOkay==true and number is within range of squished float
bool
ForthEngine::SquishDouble( double dvalue, bool approximateOkay, ulong& squishedDouble )
{
	// double precision format is 1 sign, 11 exponent, 52 mantissa
	ulong* pInVal = reinterpret_cast<ulong *>( &dvalue );
	ulong inVal = pInVal[1];

	// if bottom 34 bits of inVal aren't 0, number can't be exactly represented in squished format
	if ( !approximateOkay && ( (pInVal[0] != 0) || ((inVal & 0x3) != 0) ) )
	{
		// number can't be represented exactly
		return false;
	}
    ulong sign = (inVal & 0x80000000) >> 8;
	long exponent = (((inVal >> 20) & 0x7ff) - (1023 - 15));
	// if exponent is less than 0 or greater than 31, number can't be represented in squished format at all
	if ( (exponent < 0) || (exponent > 31) )
	{
		return false;
	}
	ulong mantissa = (inVal >> 2) & 0x3ffff;
	squishedDouble = sign | (exponent << 18) | mantissa;

	return true;
}

float
ForthEngine::UnsquishFloat( ulong squishedFloat )
{
	ulong unsquishedFloat;

	ulong sign = (squishedFloat & 0x800000) << 8;
	ulong exponent = (((squishedFloat >> 18) & 0x1f) + (127 - 15)) << 23;
	ulong mantissa = (squishedFloat & 0x3ffff) << 5;
	unsquishedFloat = sign | exponent | mantissa;

	return *(reinterpret_cast<float *>( &unsquishedFloat ));
}

double
ForthEngine::UnsquishDouble( ulong squishedDouble )
{
	ulong unsquishedDouble[2];

	unsquishedDouble[0] = 0;
	ulong sign = (squishedDouble & 0x800000) << 8;
	ulong exponent = (((squishedDouble >> 18) & 0x1f) + (1023 - 15)) << 20;
	ulong mantissa = (squishedDouble & 0x3ffff) << 2;
	unsquishedDouble[1] = sign | exponent | mantissa;

	return *(reinterpret_cast<double *>( &unsquishedDouble[0] ));
}

bool
ForthEngine::SquishLong( long long lvalue, ulong& squishedLong )
{
	bool isValid = false;
	long* pLValue = reinterpret_cast<long*>( &lvalue );
	long hiPart = pLValue[1];
	ulong lowPart = static_cast<ulong>( pLValue[0] & 0x00FFFFFF );
	ulong midPart = static_cast<ulong>( pLValue[0] & 0xFF000000 );

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

long long
ForthEngine::UnsquishLong( ulong squishedLong )
{
	long unsquishedLong[2];

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
	return *(reinterpret_cast<long long *>( &unsquishedLong[0] ));
}

// compile an opcode
// remember the last opcode compiled so someday we can do optimizations
//   like combining "->" followed by a local var name into one opcode
void
ForthEngine::CompileOpcode(forthOpType opType, long opVal)
{
    SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x\n", COMPILED_OP(opType, opVal), mDictionary.pCurrent);
    mpOpcodeCompiler->CompileOpcode(opType, opVal);
}

#if defined(DEBUG)
void ForthEngine::CompileLong(long v)
{
    SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x\n", v, mDictionary.pCurrent);
    *mDictionary.pCurrent++ = v;
}

void ForthEngine::CompileDouble(double v)
{
    SPEW_COMPILATION("Compiling double %g @ 0x%08x\n", v, mDictionary.pCurrent);
    *((double *)mDictionary.pCurrent) = v; mDictionary.pCurrent += 2;
}
#endif

// patch an opcode - fill in the branch destination offset
void ForthEngine::PatchOpcode(forthOpType opType, long opVal, long* pOpcode)
{
    SPEW_COMPILATION("Patching 0x%08x @ 0x%08x\n", COMPILED_OP(opType, opVal), pOpcode);
    mpOpcodeCompiler->PatchOpcode(opType, opVal, pOpcode);
}

void ForthEngine::ClearPeephole()
{
    mpOpcodeCompiler->ClearPeephole();
}

void
ForthEngine::CompileOpcode(long op )
{
	CompileOpcode( FORTH_OP_TYPE( op ), FORTH_OP_VALUE( op ) );
}

void
ForthEngine::CompileBuiltinOpcode( long op )
{
	if ( op < NUM_COMPILED_OPS )
	{
		CompileOpcode( gCompiledOps[op] );
	}
}

void
ForthEngine::UncompileLastOpcode( void )
{
	long *pLastCompiledOpcode = mpOpcodeCompiler->GetLastCompiledOpcodePtr();
    if ( pLastCompiledOpcode != NULL )
    {
		SPEW_COMPILATION("Uncompiling from 0x%08x to 0x%08x\n", mDictionary.pCurrent, pLastCompiledOpcode);
		mpOpcodeCompiler->UncompileLastOpcode();
    }
    else
    {
        SPEW_ENGINE( "ForthEngine::UncompileLastOpcode called with no previous opcode\n" );
        SetError( kForthErrorMissingSize, "UncompileLastOpcode called with no previous opcode" );
    }
}

long*
ForthEngine::GetLastCompiledOpcodePtr( void )
{
	return mpOpcodeCompiler->GetLastCompiledOpcodePtr();
}

long*
ForthEngine::GetLastCompiledIntoPtr( void )
{
	return mpOpcodeCompiler->GetLastCompiledIntoPtr();
}

// interpret/compile a constant value/offset
void
ForthEngine::ProcessConstant( long value, bool isOffset )
{
    if ( mCompileState )
    {
        // compile the literal value
        if ( (value < (1 << 23)) && (value >= -(1 << 23)) )
        {
            // value fits in opcode immediate field
			CompileOpcode( (isOffset ? kOpOffset : kOpConstant), value & 0xFFFFFF );
        }
        else
        {
            // value too big, must go in next longword
            if ( isOffset )
            {
                CompileBuiltinOpcode( OP_INT_VAL );
                *mDictionary.pCurrent++ = value;
                CompileBuiltinOpcode( OP_PLUS );
            }
            else
            {
                CompileBuiltinOpcode( OP_INT_VAL );
                *mDictionary.pCurrent++ = value;
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
            *--mpCore->SP = value;
        }
    }
}

// interpret/compile a 64-bit constant
void
ForthEngine::ProcessLongConstant( long long value )
{
    if ( mCompileState )
    {
        // compile the literal value
		ulong squishedLong;
		if ( SquishLong( value, squishedLong ) )
		{
            CompileOpcode( kOpSquishedLong, squishedLong );
		}
		else
		{
			CompileBuiltinOpcode( OP_DOUBLE_VAL );
			long* pDP = mDictionary.pCurrent;
            stackInt64 val;
            val.s64 = value;
			*pDP++ = val.s32[1];
			*pDP++ = val.s32[0];
			mDictionary.pCurrent = pDP;
		}
    }
    else
    {
        // leave value on param stack
        stackInt64 val;
        val.s64 = value;
        *--mpCore->SP = val.s32[0];
        *--mpCore->SP = val.s32[1];
    }
}

// return true IFF the last compiled opcode was an integer literal
bool
ForthEngine::GetLastConstant( long& constantValue )
{
	long *pLastCompiledOpcode = mpOpcodeCompiler->GetLastCompiledOpcodePtr();
    if ( pLastCompiledOpcode != NULL )
	{
        long op = *pLastCompiledOpcode;
        if ( ((pLastCompiledOpcode + 1) == mDictionary.pCurrent)
            && (FORTH_OP_TYPE( op ) == kOpConstant) )
        {
            constantValue = FORTH_OP_VALUE( op );
            return true;
        }
    }
    return false;
}

//
// FullyExecuteOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
eForthResult
ForthEngine::FullyExecuteOp(ForthCoreState* pCore, long opCode)
{
	long opScratch[2];

	opScratch[0] = opCode;
	opScratch[1] = gCompiledOps[OP_DONE];
	eForthResult exitStatus = ExecuteOps(pCore, &(opScratch[0]));
	if (exitStatus == kResultYield)
	{
		SetError(kForthErrorIllegalOperation, " yield not allowed in FullyExecuteOp");
	}

	return exitStatus;
}

//
// ExecuteOp executes a single op.  If the op is a user-defined op or method, only the
//   very first op is executed before returning.
//
eForthResult
ForthEngine::ExecuteOp(ForthCoreState* pCore, long opCode)
{
	eForthResult exitStatus = InterpretOneOp(pCore, opCode);
	return exitStatus;
}

//
// ExecuteOps executes a sequence of forth ops.
// code at pOps must be terminated with OP_DONE
//
eForthResult
ForthEngine::ExecuteOps(ForthCoreState* pCore, long *pOps)
{
    long *savedIP;

    savedIP = pCore->IP;
    pCore->IP = pOps;
    mpMainThread->InnerLoop();
    eForthResult exitStatus = (eForthResult)pCore->state;

	pCore->IP = savedIP;
    if ( exitStatus == kResultDone )
    {
		pCore->state = kResultOk;
        exitStatus = kResultOk;
    }
    return exitStatus;
}

eForthResult
ForthEngine::FullyExecuteMethod(ForthCoreState* pCore, ForthObject& obj, long methodNum)
{
	long opScratch[2];
	long opCode = obj->pMethods[methodNum];

	RPUSH(((long)GET_TP));
	SET_THIS(obj);

	opScratch[0] = opCode;
	opScratch[1] = gCompiledOps[OP_DONE];
	eForthResult exitStatus = ExecuteOps(pCore, &(opScratch[0]));

	if (exitStatus == kResultYield)
	{
		SetError(kForthErrorIllegalOperation, " yield not allowed in FullyExecuteMethod");
	}
	return exitStatus;
}


void
ForthEngine::AddErrorText( const char *pString )
{
    strcat( mpErrorString, pString );
}

void
ForthEngine::SetError( eForthError e, const char *pString )
{
    mpCore->error = e;
    if ( pString )
    {
	    strcat( mpErrorString, pString );
    }
    if ( e == kForthErrorNone )
    {
        // previous error state is being cleared
        mpErrorString[0] = '\0';
    }
    else
    {
        mpCore->state = kResultError;
    }
}

void
ForthEngine::SetFatalError( eForthError e, const char *pString )
{
    mpCore->state = kResultFatalError;
    mpCore->error = e;
    if ( pString )
    {
        strcpy( mpErrorString, pString );
    }
}

void
ForthEngine::GetErrorString( char *pBuffer, int bufferSize )
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


eForthResult
ForthEngine::CheckStacks( void )
{
    long depth;
    eForthResult result = kResultOk;

    // check parameter stack for over/underflow
    depth = mpCore->ST - mpCore->SP;
    if ( depth < 0 )
    {
        SetError( kForthErrorParamStackUnderflow );
        result = kResultError;
    }
    else if ( depth >= (long) mpCore->SLen )
    {
        SetError( kForthErrorParamStackOverflow );
        result = kResultError;
    }
    
    // check return stack for over/underflow
    depth = mpCore->RT - mpCore->RP;
    if ( depth < 0 )
    {
        SetError( kForthErrorReturnStackUnderflow );
        result = kResultError;
    }
    else if ( depth >= (long) mpCore->RLen )
    {
        SetError( kForthErrorReturnStackOverflow );
        result = kResultError;
    }

    return result;
}


void ForthEngine::SetDefaultConsoleOut( ForthObject& newOutStream )
{
	SPEW_SHELL("SetDefaultConsoleOut pCore=%p  pMethods=%p  pData=%p\n", mpCore, newOutStream->pMethods, newOutStream);
	OBJECT_ASSIGN(mpCore, mDefaultConsoleOutStream, newOutStream);
	mDefaultConsoleOutStream = newOutStream;
}

void ForthEngine::SetConsoleOut( ForthCoreState* pCore, ForthObject& newOutStream )
{
	SPEW_SHELL("SetConsoleOut pCore=%p  pMethods=%p  pData=%p\n", pCore, newOutStream->pMethods, newOutStream);
	OBJECT_ASSIGN( pCore, pCore->consoleOutStream, newOutStream );
	pCore->consoleOutStream = newOutStream;
}

void ForthEngine::PushConsoleOut( ForthCoreState* pCore )
{
	PUSH_OBJECT( pCore->consoleOutStream );
}

void ForthEngine::PushDefaultConsoleOut( ForthCoreState* pCore )
{
	PUSH_OBJECT( mDefaultConsoleOutStream );
}

void ForthEngine::ResetConsoleOut( ForthCoreState* pCore )
{
	// TODO: there is a dilemma here - either we just replace the current output stream
	//  without doing a release, and possibly leak a stream object, or we do a release
	//  and risk a crash, since ResetConsoleOut is called when an error is detected,
	//  so the object we are releasing may already be deleted or otherwise corrupted.
	CLEAR_OBJECT(pCore->consoleOutStream);
	OBJECT_ASSIGN(pCore, pCore->consoleOutStream, mDefaultConsoleOutStream);
	pCore->consoleOutStream = mDefaultConsoleOutStream;
}


void ForthEngine::ConsoleOut( const char* pBuff )
{
    ForthConsoleStringOut( mpCore, pBuff );
}


long ForthEngine::GetTraceFlags( void )
{
	return mpCore->traceFlags;
}

void ForthEngine::SetTraceFlags( long flags )
{
	mpCore->traceFlags = flags;
}

////////////////////////////
//
// enumerated type support
//
void
ForthEngine::StartEnumDefinition( void )
{
    SetFlag( kEngineFlagInEnumDefinition );
    mNextEnum = 0;
    // remember the stack level at start of definition
    // when another enum is to be defined, if the current stack is above this level,
    // the top element on the stack will be popped and set the current value of the enum
    mpEnumStackBase = mpCore->SP;
}

void
ForthEngine::EndEnumDefinition( void )
{
    ClearFlag( kEngineFlagInEnumDefinition );
}

// return milliseconds since engine was created
unsigned long
ForthEngine::GetElapsedTime( void )
{
	unsigned long millisecondsElapsed = 0;
#if defined(WIN32)
#if defined(MSDEV)
	struct __timeb32 now;

	_ftime32_s( &now );
	__time32_t seconds = now.time - mStartTime.time;
    __time32_t milliseconds = now.millitm - mStartTime.millitm;
	millisecondsElapsed =  (unsigned long) ((seconds * 1000) + milliseconds);
#else
	struct _timeb now;
    _ftime( &now );

    long seconds = now.time - mStartTime.time;
    long milliseconds = now.millitm - mStartTime.millitm;
	millisecondsElapsed = (unsigned long) ((seconds * 1000) + milliseconds);
#endif
#else
	struct timeb now;
    ftime( &now );

    long seconds = now.time - mStartTime.time;
    long milliseconds = now.millitm - mStartTime.millitm;
	millisecondsElapsed = (unsigned long) ((seconds * 1000) + milliseconds);
#endif
	return millisecondsElapsed;
}


void
ForthEngine::DumpCrashState()
{
	char buff[256];

	long* pSP = mpCore->SP;
	if ( (pSP < mpCore->ST) && (pSP > mpCore->SB) )
	{
		int numToShow = 64;
		int depth = mpCore->ST - pSP;
		if ( depth < numToShow )
		{
			numToShow = depth;
		}
		for ( int i = 0; i < numToShow; i++ )
		{
			SNPRINTF( buff, sizeof(buff), "S[%2d] 0x%08x  %d\n", depth - (i + 1), pSP[i], pSP[i] );
			ConsoleOut( buff );
		}
	}

	SNPRINTF( buff, sizeof(buff), "\n   IP 0x%08x   ", mpCore->IP );
	ConsoleOut( buff );
	DisplayUserDefCrash( mpCore->IP, buff, sizeof(buff) );
	ConsoleOut( "\n" );

	long* pRP = mpCore->RP;
	if ( (pRP < mpCore->RT) && (pRP > mpCore->RB) )
	{
		int numToShow = 64;
		int depth = mpCore->RT - pRP;
		long* pFP = mpCore->FP;
		if ( depth < numToShow )
		{
			numToShow = depth;
		}
		for ( int i = 0; i < numToShow; i++ )
		{
			unsigned long rVal = (unsigned long)(pRP[i]);
			long* pRVal = (long *) rVal;
			SNPRINTF( buff, sizeof(buff), "R[%2d] 0x%08x   ", depth - (i + 1), rVal );
			ConsoleOut( buff );
			if ( (pRP + i) == pFP )
			{
				ConsoleOut( "<FP>" );
				pFP = (long *)pRVal;
			}
			else
			{
				DisplayUserDefCrash( pRVal, buff, sizeof(buff) );
			}
			ConsoleOut( "\n" );
		}
	}

}

void ForthEngine::DisplayUserDefCrash( long *pRVal, char* buff, int buffSize )
{
	if ( (pRVal >= mDictionary.pBase) && (pRVal < mDictionary.pCurrent) )
	{
		long* pDefBase;
		ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
		long* pClosestIP = NULL;
		long* pFoundClosest = NULL;
		ForthVocabulary* pFoundVocab = NULL;
		while ( pVocab != NULL )
		{
			long* pClosest = FindUserDefinition( pVocab, pClosestIP, pRVal, pDefBase );
			if ( pClosest != NULL )
			{
				pFoundClosest = pClosest;
				pFoundVocab = pVocab;
			}
			pVocab = pVocab->GetNextChainVocabulary();
		}
		if ( pFoundClosest != NULL )
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
			SNPRINTF( buff, buffSize, " + 0x%04x", pRVal - pDefBase );
		}
		else
		{
			strcpy( buff, "*user def* " );
		}
	}
	else
	{
		SNPRINTF( buff, buffSize, "%d", (long) pRVal );
	}
	ConsoleOut( buff );
}

long * ForthEngine::FindUserDefinition( ForthVocabulary* pVocab, long*& pClosestIP, long* pIP, long*& pBase  )
{
	long* pClosest = NULL;
	long* pEntry = pVocab->GetNewestEntry();

	for ( int i = 0; i < pVocab->GetNumEntries(); i++ )
	{
		long typeCode = pEntry[1];
		unsigned long opcode = 0;
		if ( CODE_TO_BASE_TYPE(pEntry[1]) == kBaseTypeUserDefinition )
		{
			switch ( FORTH_OP_TYPE( *pEntry ) )
			{
				case kOpUserDef:
				case kOpUserDefImmediate:
					{
						opcode = ((unsigned long)FORTH_OP_VALUE( *pEntry ));
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
			opcode = ((unsigned long)FORTH_OP_VALUE( opcode ));
		}
		if ( (opcode > 0) && (opcode < mpCore->numOps) )
		{
			long* pDef = mpCore->ops[opcode];
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


ForthBlockFileManager*
ForthEngine::GetBlockFileManager()
{
    return mBlockFileManager;
}


bool
ForthEngine::IsServer() const
{
	return mIsServer;
}

void
ForthEngine::SetIsServer(bool isServer)
{
	mIsServer = isServer;
}

void ForthEngine::DefineLabel(const char* inLabelName, long* inLabelIP)
{
	for (ForthLabel& label : mLabels)
	{
		if (label.name == inLabelName)
		{
			label.DefineLabelIP(inLabelIP);
			return;
		}
	}
	mLabels.emplace_back(ForthLabel(inLabelName, inLabelIP));
}

void ForthEngine::AddGoto(const char* inLabelName, int inBranchType, long* inBranchIP)
{
	for (ForthLabel& label : mLabels)
	{
		if (label.name == inLabelName)
		{
			label.AddReference(inBranchIP, inBranchType);
			return;
		}
	}
	ForthLabel newLabel(inLabelName);
	newLabel.AddReference(inBranchIP, inBranchType);
	mLabels.push_back(newLabel);
}

// if inText is null, string is not copied, an uninitialized space of size inNumChars+1 is allocated
// if inNumChars is -1 and inText is not null, length of input string is used for temp string size
// if both inText is null and inNumChars is -1, an uninitialized space of 255 chars is allocated
char* ForthEngine::AddTempString(const char* inText, int inNumChars)
{
	// this hooha turns mpStringBufferA into multiple string buffers
	//   so that you can use multiple interpretive string buffers
	// it is used both for quoted strings in interpretive mode and blword/$word
	// we leave space for a preceeding  length byte and a trailing null terminator
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
void ForthEngine::PushContinuation(long val)
{
    if ((size_t)mContinuationIx >= mContinuations.size())
    {
        mContinuations.resize(mContinuationIx + 32);
    }
    mContinuations[mContinuationIx++] = val;
}

long ForthEngine::PopContinuation()
{
    long result = 0;
    if (mContinuationIx > 0)
    {
        mContinuationIx--;
        result = mContinuations[mContinuationIx];
    }
    else
    {
        SetError(kForthErrorBadSyntax, "not enough continuations");
    }
    return result;
}

void ForthEngine::ResetContinuations()
{
    mContinuations.clear();
    mContinuationIx = 0;
    mContinueDestination = nullptr;
    mContinueCount = 0;
}

long* ForthEngine::GetContinuationDestination()
{
    return mContinueDestination;
}

void ForthEngine::SetContinuationDestination(long* pDest)
{
    mContinueDestination = pDest;
}

void ForthEngine::AddContinuationBranch(long* pAddr, long opType)
{
    PushContinuation((long) pAddr);
    PushContinuation(opType);
    ++mContinueCount;
}

void ForthEngine::AddBreakBranch(long* pAddr, long opType)
{
    PushContinuation(((long)pAddr) + 1);
    PushContinuation(opType);
    ++mContinueCount;
}

void ForthEngine::StartLoopContinuations()
{
    PushContinuation((long) mContinueDestination);
    PushContinuation(mContinueCount);
    mContinueDestination = nullptr;
    mContinueCount = 0;
}

void ForthEngine::EndLoopContinuations(int controlFlowType)  // actually takes a eShellTag
{
    // fixup pending continue branches for current loop
    if (mContinueCount > 0)
    {
        long *pDP = GetDP();

        for (int i = 0; i < mContinueCount; ++i)
        {
            if (mContinuationIx >= 2)
            {
                long opType = PopContinuation();
                long target = PopContinuation();
                if ((target & 1) != 0)
                {
                    // this is actually a break
                    if (controlFlowType != kShellTagDo)
                    {
                        long *pBreak = (long *)(target & ~1);
                        *pBreak = COMPILED_OP(opType, (pDP - pBreak) - 1);
                    }
                    else
                    {
                        SetError(kForthErrorBadSyntax, "break not allowed in do loop, use leave");
                        break;
                    }
                }
                else
                {
                    if (controlFlowType != kShellTagCase)
                    {
                        if (mContinueDestination != nullptr)
                        {
                            long *pContinue = (long *)target;
                            *pContinue = COMPILED_OP(opType, (mContinueDestination - pContinue) - 1);
                        }
                        else
                        {
                            SetError(kForthErrorBadSyntax, "end loop with unresolved continues");
                            break;
                        }
                    }
                    else
                    {
                        SetError(kForthErrorBadSyntax, "continue not allowed in case statement");
                        break;
                    }
                }
            }
            else
            {
                // report error - end loop with continuation stack empty
                SetError(kForthErrorBadSyntax, "end loop with continuation stack empty");
                break;
            }
        }
    }
    mContinueCount = PopContinuation();
    mContinueDestination = (long *)PopContinuation();
}

bool ForthEngine::HasPendingContinuations()
{
    return mContinueCount != 0;
}


void ForthEngine::AddGlobalObjectVariable(ForthObject* pObject)
{
    mGlobalObjectVariables.push_back(pObject);
}

void ForthEngine::CleanupGlobalObjectVariables(long* pNewDP)
{
    int objectIndex = mGlobalObjectVariables.size() - 1;
    while (objectIndex >= 0)
    {
        if (mGlobalObjectVariables[objectIndex] < (ForthObject *)pNewDP)
        {
            // we are done releasing objects, all remaining objects are below new DP
            break;
        }
        ForthObject& o = *(mGlobalObjectVariables[objectIndex]);
        if (o != nullptr)
        {
            ForthClassObject* pClassObj = (ForthClassObject *)(o->pMethods[-1]);
            TraceOut("Releasing object of class %s, refcount %d\n", pClassObj->pVocab->GetName(), o->refCount);
            SAFE_RELEASE(mpCore, o);
        }
        objectIndex--;
    }
    mGlobalObjectVariables.resize(objectIndex + 1);
}

char* ForthEngine::GrabTempBuffer()
{
#ifdef WIN32
    EnterCriticalSection(mpTempBufferLock);
#else
    pthread_mutex_lock(mpTempBufferLock);
#endif

    return mpTempBuffer;
}

void ForthEngine::UngrabTempBuffer()
{
#ifdef WIN32
    EnterCriticalSection(mpTempBufferLock);
#else
    pthread_mutex_lock(mpTempBufferLock);
#endif
}

void ForthEngine::RaiseException(ForthCoreState *pCore, long newExceptionNum)
{
    char errorMsg[64];
    ForthExceptionFrame *pExceptionFrame = pCore->pExceptionFrame;
    if (pExceptionFrame != nullptr)
    {
        // exception frame:
        //  0   old exception frame ptr
        //  1   saved pstack ptr
        //  2   ptr to catchIP,finallyIP
        //  3   saved frame ptr
        //  4   exception number
        //  5   exception state
        if (newExceptionNum)
        {
            long oldExceptionNum = pExceptionFrame->exceptionNumber;
            pExceptionFrame->exceptionNumber = newExceptionNum;
            if (oldExceptionNum)
            {
                if (pExceptionFrame->exceptionState == kForthExceptionStateFinally)
                {
                    // exception inside a finally section, avoid infinite loop
                    // ? should this be a reraise to surrounding handler instead
                    snprintf(errorMsg, sizeof(errorMsg), "Reraised exception of type %d in finally section", newExceptionNum);
                    SetError(kForthErrorException, errorMsg);
                }
                else
                {
                    // re-raise inside an exception handler - transfer control to finally body
                    SET_SP(pExceptionFrame->pSavedSP);
                    SET_IP(pExceptionFrame->pHandlerIPs[1]);
                }
            }
            else
            {
                // raise in try body
                SET_SP(pExceptionFrame->pSavedSP);
                SPUSH(newExceptionNum);
                SET_IP(pExceptionFrame->pHandlerIPs[0]);
                pExceptionFrame->exceptionState = kForthExceptionStateExcept;
            }
        }
        pExceptionFrame->exceptionNumber = newExceptionNum;
    }
    else
    {
        if (newExceptionNum)
        {
            snprintf(errorMsg, sizeof(errorMsg), "Unhandled exception of type %d", newExceptionNum);
            SetError(kForthErrorException, errorMsg);
        }
    }
}

//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################


// return true to exit forth shell
eForthResult
ForthEngine::ProcessToken( ForthParseInfo   *pInfo )
{
    long *pEntry, value;
    long long lvalue;
    eForthResult exitStatus = kResultOk;
    float fvalue;
    double dvalue;
    char *pToken = pInfo->GetToken();
    int len = pInfo->GetTokenLength();
    bool isAString = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_STRING) != 0;
	bool isAQuotedCharacter = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_CHARACTER) != 0;
    bool isSingle, isOffset, isApproximate;
    double* pDPD;
    ForthVocabulary* pFoundVocab = NULL;

    mpLastToken = pToken;
    if ( (pToken == NULL)   ||   ((len == 0) && !(isAString || isAQuotedCharacter)) )
    {
        // ignore empty tokens, except for the empty quoted string and null character
        return kResultOk;
    }
    
    if (mCompileState)
    {
        SPEW_OUTER_INTERPRETER("Compile {%s} flags[%x] @0x%08x\t", pToken, pInfo->GetFlags(), mDictionary.pCurrent);
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
            strcpy( (char *) mDictionary.pCurrent, pToken );
            mDictionary.pCurrent += lenLongs;
        }
        else
        {
            // in interpret mode, stick the string in string buffer A
            //   and leave the address on param stack
            *--mpCore->SP = (long) AddTempString(pToken, len);
        }
        return kResultOk;
        
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
		int tokenLen = strlen(pToken);
		if (isALongQuotedCharacter || (tokenLen > 4))
		{
			lvalue = 0;
			char* cval = (char *)&lvalue;
			for (int i = 0; i < len; i++)
			{
				cval[i] = pToken[i];
			}
			ProcessLongConstant(lvalue);
		}
		else
		{
			value = 0;
			char* cval = (char *)&value;
			for (int i = 0; i < len; i++)
			{
				cval[i] = pToken[i];
			}
			ProcessConstant(value, false);
		}
		return kResultOk;
    }
    
    if ( mpInterpreterExtension != NULL )
    {
        if ( (*mpInterpreterExtension)( pToken ) )
        {
            ////////////////////////////////////
            //
            // symbol was processed by user-defined interpreter extension
            //
            ////////////////////////////////////
            return kResultOk;
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
            return kResultOk;
        }
        if ( (pToken[0] == '&') && (pToken[1] != '\0') )
        {
            pEntry = mpLocalVocab->FindSymbol( pToken + 1 );
            if ( pEntry )
            {
                ////////////////////////////////////
                //
                // symbol is a reference to a local variable
                //
                ////////////////////////////////////
                SPEW_OUTER_INTERPRETER( "Local variable reference {%s}\n", pToken + 1);
                long varOffset = FORTH_OP_VALUE( *pEntry );
                CompileOpcode( COMPILED_OP( kOpLocalRef, varOffset ) );
                return kResultOk;
            }
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

    pEntry = NULL;
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
            if ( pVocab != NULL )
            {
                pEntry = pVocab->FindSymbol( pColon + 1 );
                if ( pEntry != NULL )
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
                    *--mpCore->SP = (long)(pColon + 1);
                    exitStatus = FullyExecuteOp(mpCore, *pEntry);
                    return exitStatus;
                }
            }
            *pColon = ':';
        }
    }
    if ( pEntry == NULL )
    {
#ifdef MAP_LOOKUP
        pEntry = mpVocabStack->FindSymbol( pToken, &pFoundVocab );
#else
        pEntry = mpVocabStack->FindSymbol( pInfo, &pFoundVocab );
#endif
    }
    if ( pEntry != NULL )
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
        if ( pStructVocab != NULL )
        {
			elementSize = pStructVocab->GetSize();
        }
		else
		{
			ForthNativeType *pNative = mpTypesManager->GetNativeTypeFromName( pToken );
			if ( pNative != NULL )
			{
				// string[] is not supported
				if ( pNative->GetBaseType() != kBaseTypeString )
				{
					elementSize = pNative->GetSize();
				}
			}
			else if ( ScanIntegerToken( pToken, value, lvalue, mpCore->base, isOffset, isSingle ) && isSingle )
			{
				elementSize = value;
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
				value = *mpCore->SP++;		// get base address
				*mpCore->SP = value + (elementSize * (*mpCore->SP));
			}
			return kResultOk;
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
			  ulong squishedFloat;
			  if ( SquishFloat( fvalue, isApproximate, squishedFloat ) )
			  {
	              CompileOpcode( kOpSquishedFloat, squishedFloat );
			  }
			  else
			  {
				  CompileBuiltinOpcode( OP_FLOAT_VAL );
				  *(float *) mDictionary.pCurrent++ = fvalue;
			  }
          }
          else
          {
              --mpCore->SP;
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
			  ulong squishedDouble;
			  if (  SquishDouble( dvalue, isApproximate, squishedDouble ) )
			  {
	              CompileOpcode( kOpSquishedDouble, squishedDouble );
			  }
			  else
			  {
				  CompileBuiltinOpcode( OP_DOUBLE_VAL );
				  pDPD = (double *) mDictionary.pCurrent;
				  *pDPD++ = dvalue;
				  mDictionary.pCurrent = (long *) pDPD;
			  }
          }
          else
          {
              mpCore->SP -= 2;
              *(double *) mpCore->SP = dvalue;
          }
       }
        
    }
    else if ( ScanIntegerToken( pToken, value, lvalue, mpCore->base, isOffset, isSingle ) )
    {

        ////////////////////////////////////
        //
        // symbol is an integer literal
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Integer literal %d\n", value );
        if ( isSingle )
        {
            ProcessConstant( value, isOffset );
        }
        else
        {
            ProcessLongConstant( lvalue );
        }
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
            mpDefinitionVocab->AddSymbol( pToken, kOpConstant, mNextEnum & 0x00FFFFFF, false );
			long* pNewEnum = mpDefinitionVocab->GetNewestEntry();
		    pNewEnum[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
        }
        else
        {
            // number is out of range of kOpConstant, need to define a user op
            StartOpDefinition( pToken );
            CompileBuiltinOpcode( OP_DO_CONSTANT );
            CompileLong( mNextEnum );
        }
        mNextEnum++;
    }
    else
    {
		SPEW_ENGINE( "Unknown symbol %s\n", pToken );
		mpCore->error = kForthErrorUnknownSymbol;
		exitStatus = kResultError;
    }

    // TODO: return exit-shell flag
    return exitStatus;
}



//############################################################################
