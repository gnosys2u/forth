//////////////////////////////////////////////////////////////////////
//
// OuterInterpreter.cpp: implementation of the OuterInterpreter class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
// use stdint.h for INT_MAX instead of C++ <limits> because windows always defines max(A,B) macro
#include <stdint.h>

#if defined(LINUX) || defined(MACOSX)
#include <ctype.h>
#endif

#include "Engine.h"
#include "OuterInterpreter.h"
#include "Extension.h"
#include "Thread.h"
#include "Shell.h"
#include "Vocabulary.h"
#include "ForthInner.h"
#include "StructVocabulary.h"
#include "OpcodeCompiler.h"
#include "ForthPortability.h"
#include "BuiltinClasses.h"
#include "ParseInfo.h"
#include "LocalVocabulary.h"
#include "VocabularyStack.h"
#include "NativeType.h"
#include "TypesManager.h"
#include "ClassVocabulary.h"
#include "InputStack.h"
#include "UsingVocabulary.h"
#include "ControlStack.h"

// switch checkOpResult on to provide a convenient place to put a breakpoint when trying
// down where an error is coming from in the debugger
#if 0
extern void checkOpResult(OpResult result);
#else
#define checkOpResult(RESULT)
#endif

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

///////////////////////////////////////////////////////////////////////
//
// errors must be kept in sync with ForthError enum in forth.h
//
struct errorEntry {
    ForthError code;
    const char* symbol;
    const char* text;
};

static const struct errorEntry errorList[] = {
    // ANSI defined errors
    { ForthError::none, "none", "none" },
    { ForthError::abort, "abort", "abort" },
    { ForthError::abortQuote, "abortQuote", "abort\"" },
    { ForthError::stackOverflow, "stackOverflow", "parameter stack overflow" },
    { ForthError::stackUnderflow, "stackUnderflow", "parameter stack underflow" },
    { ForthError::returnStackOverflow, "returnStackOverflow", "return stack overflow" },
    { ForthError::returnStackUnderflow, "returnStackUnderflow", "return stack underflow" },
    { ForthError::doLoopNestTooDeep, "doLoopNestTooDeep", "do loop nested too deep" },
    { ForthError::dictionaryOverflow, "dictionaryOverflow", "dictionary overflow" },
    { ForthError::invalidMemoryAddress, "invalidMemoryAddress", "invalid memory address" },
    { ForthError::divideByZero, "divideByZero", "divide by zero" },
    { ForthError::resultOutOfRange, "resultOutOfRange", "result out of range" },
    { ForthError::argumentTypeMismatch, "argumentTypeMismatch", "argument type mismatch" },
    { ForthError::undefinedWord, "undefinedWord", "undefined word" },
    { ForthError::interpretingCompileOnlyWord, "interpretingCompileOnlyWord", "interpreting compile-only word" },
    { ForthError::invalidForget, "invalidForget", "invalid forget" },
    { ForthError::zeroLengthName, "zeroLengthName", "zero length name" },
    { ForthError::picturedOutputStringOverflow, "picturedOutputStringOverflow", "picturedOutputStringOverflow" },
    { ForthError::parsedStringOverflow, "parsedStringOverflow", "parsedStringOverflow" },
    { ForthError::definitionNameTooLong, "definitionNameTooLong", "definitionNameTooLong" },
    { ForthError::writeToAReadOnlyLocation, "writeToAReadOnlyLocation", "writeToAReadOnlyLocation" },
    { ForthError::unsupportedOperation, "unsupportedOperation", "unsupported operation" },
    { ForthError::controlStructureMismatch, "controlStructureMismatch", "control structure mismatch" },
    { ForthError::addressAlignmentException, "addressAlignmentException", "address alignment exception" },
    { ForthError::invalidNumericArgument, "invalidNumericArgument", "invalid numeric argument" },
    { ForthError::returnStackImbalance, "returnStackImbalance", "return stack imbalance" },
    { ForthError::loopParametersUnavailable, "loopParametersUnavailable", "loop parameters unavailable" },
    { ForthError::invalidRecursion, "invalidRecursion", "invalid recursion" },
    { ForthError::userInterrupt, "userInterrupt", "user interrupt" },
    { ForthError::compilerNesting, "compilerNesting", "compiler nesting" },
    { ForthError::obsolescentFeature, "obsolescentFeature", "obsolescent feature" },
    { ForthError::invalidToBody, "invalidToBody", "invalidToBody" },
    { ForthError::invalidNameArgument, "invalidNameArgument", "invalidNameArgument" },
    { ForthError::blockReadException, "blockReadException", "blockReadException" },
    { ForthError::blockWriteException, "blockWriteException", "blockWriteException" },
    { ForthError::invalidBlockNumber, "invalidBlockNumber", "invalidBlockNumber" },
    { ForthError::invalidFilePosition, "invalidFilePosition", "invalidFilePosition" },
    { ForthError::fileIOException, "fileIOException", "fileIOException" },
    { ForthError::nonExistentFile, "nonExistentFile", "nonExistentFile" },
    { ForthError::unexpectedEndOfFile, "unexpectedEndOfFile", "unexpectedEndOfFile" },
    { ForthError::invalidBaseForFloatingPointConversion, "invalidBaseForFloatingPointConversion", "invalidBaseForFloatingPointConversion" },
    { ForthError::lossOfPrecision, "lossOfPrecision", "lossOfPrecision" },
    { ForthError::fpDivideByZero, "fpDivideByZero", "fpDivideByZero" },
    { ForthError::fpResultOutOfRange, "fpResultOutOfRange", "fpResultOutOfRange" },
    { ForthError::fpStackOverflow, "fpStackOverflow", "fpStackOverflow" },
    { ForthError::fpStackUnderflow, "fpStackUnderflow", "fpStackUnderflow" },
    { ForthError::fpInvalidArgument, "fpInvalidArgument", "fpInvalidArgument" },
    { ForthError::compilationWordListDeleted, "compilationWordListDeleted", "compilationWordListDeleted" },
    { ForthError::invalidPostpone, "invalidPostpone", "invalidPostpone" },
    { ForthError::searchOrderOverflow, "searchOrderOverflow", "searchOrderOverflow" },
    { ForthError::searchOrderUnderflow, "searchOrderUnderflow", "searchOrderUnderflow" },
    { ForthError::compilationWordListChanged, "compilationWordListChanged", "compilationWordListChanged" },
    { ForthError::controlFlowStackOverflow, "controlFlowStackOverflow", "controlFlowStackOverflow" },
    { ForthError::exceptionStackOverflow, "exceptionStackOverflow", "exceptionStackOverflow" },
    { ForthError::fpUnderflow, "fpUnderflow", "fpUnderflow" },
    { ForthError::fpUnidentifiedFault, "fpUnidentifiedFault", "fpUnidentifiedFault" },
    { ForthError::quit, "quit", "quit" },
    { ForthError::sendingOrReceivingCharacter, "sendingOrReceivingCharacter", "sending or receiving character" },
    { ForthError::preprocessorError, "preprocessorError", "[if], [else] or [then] exception" },
    { ForthError::allocate, "allocate", "allocate" },
    { ForthError::free, "free", "free" },
    { ForthError::resize, "resize", "resize" },
    { ForthError::closeFile, "closeFile", "closeFile" },
    { ForthError::createFile, "createFile", "createFile" },
    { ForthError::deleteFile, "deleteFile", "deleteFile" },
    { ForthError::filePosition, "filePosition", "filePosition" },
    { ForthError::fileSize, "fileSize", "fileSize" },
    { ForthError::fileStatus, "fileStatus", "fileStatus" },
    { ForthError::flushFile, "flushFile", "flushFile" },
    { ForthError::openFile, "openFile", "openFile" },
    { ForthError::readFile, "readFile", "readFile" },
    { ForthError::readLine, "readLine", "readLine" },
    { ForthError::renameFile, "renameFile", "renameFile" },
    { ForthError::repositionFile, "repositionFile", "repositionFile" },
    { ForthError::resizeFile, "resizeFile", "resizeFile" },
    { ForthError::writeFile, "writeFile", "writeFile" },
    { ForthError::writeLine, "writeLine", "writeLine" },
    { ForthError::malformedXChar, "malformedXChar", "malformed XChar" },
    { ForthError::substitute, "substitute", "substitute" },
    { ForthError::replaces, "replaces", "replaces" },
    { ForthError::invalidParameter, "invalidParameter", "invalid parameter" },
    { ForthError::illegalOperation, "illegalOperation", "illegal operation" },
    { ForthError::badReferenceCount, "badReferenceCount", "bad reference count" },
    { ForthError::invalidType, "invalidType", "invalid type" },
    { ForthError::badArrayIndex, "badArrayIndex", "bad array index" },
    { ForthError::badVarOperation, "badVarOperation", "bad variable operation" },
    { ForthError::exceptionInFinally, "exceptionInFinally", "exception in ]finally[ section" },
    { ForthError::badSyntax, "badSyntax", "bad syntax" },
    { ForthError::unimplementedMethod, "unimplementedMethod", "unimplemented method" },
    { ForthError::badOpcode, "badOpcode", "bad opcode" },
    { ForthError::badOpcodeType, "badOpcodeType", "bad opcode type" },
    { ForthError::missingSize, "missingSize", "missing size" },
    { ForthError::genericUserError, "genericUserError", "genericUserError" },
    { ForthError::badBlockNumber, "badBlockNumber", "bad block number" },
    { ForthError::unknownType, "unknownType", "unknown type" },
    { ForthError::brokenObject, "brokenObject", "broken object" },
    { ForthError::controlStackUnderflow, "controlStackUnderflow", "control stack underflow" },
    { ForthError::controlStackIndexRange, "controlStackIndexRange", "control stack index out of range" },
    { ForthError::structError, "structError", "struct error" },
    { ForthError::illegalMethod, "illegalMethod", "illegal method" },
    { ForthError::stringOverflow, "stringOverflow", "string overflow" },
    { ForthError::badObject, "badObject", "bad object" },
    { ForthError::missingExceptionHandler, "missingExceptionHandler", "missing exception handler" },
    { ForthError::invalidWordlist, "invalidWordlist", "invalid wordlist" },
};

FORTHOP(getErrorTextOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthError errorNum = (ForthError)(SPOP);
    const char* pText = pOuter->GetErrorString(errorNum);
    SPUSH((cell)pText);
}

FORTHOP(getOptypeNameOp)
{
    uint32_t opType = (uint32_t)(SPOP);
    const char* pText = GET_ENGINE->GetOpTypeName(opType);
    SPUSH((cell)pText);
}

//////////////////////////////////////////////////////////////////////
////
///
//                     OuterInterpreter
// 

OuterInterpreter::OuterInterpreter(Engine* pEngine)
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
    , mFeatures(kFFMultiCharacterLiterals | kFFCStringLiterals
        | kFFParenIsExpression | kFFAllowContinuations | kFFAllowVaropSuffix)
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
    , mpUsingVocab(nullptr)
    , mNewestDefinedOp(0)
{
    mpShell = mpEngine->GetShell();
    mpCore = mpEngine->GetMainFiber()->GetCore();
    mpDictionary = mpEngine->GetDictionaryMemorySection();

    mTokenStack.Initialize(4);

    mpOpcodeCompiler = new OpcodeCompiler(mpEngine->GetDictionaryMemorySection());

    if (mpTypesManager == nullptr)
    {
        mpTypesManager = new TypesManager();
    }

    mpForthVocab = new Vocabulary("forth", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpRootVocab = new Vocabulary("_root", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpLiteralsVocab = new Vocabulary("literals", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpErrorsVocab = new Vocabulary("errors", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpOptypesVocab = new Vocabulary("optypes", NUM_FORTH_VOCAB_VALUE_LONGS);
    mpLocalVocab = new LocalVocabulary(NUM_LOCALS_VOCAB_VALUE_LONGS);
    mpUsingVocab = new UsingVocabulary();

    mStringBufferASize = 3 * MAX_STRING_SIZE;
    mpStringBufferA = new char[mStringBufferASize];
    mpTempBuffer = new char[MAX_STRING_SIZE];

    mpDefinitionVocab = mpForthVocab;
}

void OuterInterpreter::Initialize()
{
    mpVocabStack = new VocabularyStack;
    mpVocabStack->Initialize(mpRootVocab);
    mpVocabStack->Clear(mpForthVocab);
}

void OuterInterpreter::InitializeVocabulariesAndClasses()
{
    for (struct errorEntry err : errorList)
    {
        mErrorMap[err.code] = err.text;
        forthop errorOp = COMPILED_OP(kOpConstant, (forthop)err.code);
        mpErrorsVocab->AddSymbol(err.symbol, errorOp);
    }
    mpDefinitionVocab = mpErrorsVocab;
    AddBuiltinOp("getText", kOpCCode, (void *)getErrorTextOp);

    const char* unknownTypeName = mpEngine->GetOpTypeName(256);
    for (int32_t i = 0; i < 256; ++i)
    {
        const char* typeName = mpEngine->GetOpTypeName(i);
        if (typeName != nullptr && strcmp(typeName, unknownTypeName) != 0)
        {
            forthop typeOp = COMPILED_OP(kOpConstant, (forthop)i);
            mpOptypesVocab->AddSymbol(typeName, typeOp);
        }
    }
    mpDefinitionVocab = mpOptypesVocab;
    AddBuiltinOp("getName", kOpCCode, (void *)getOptypeNameOp);

    mpDefinitionVocab = mpForthVocab;

    AddBuiltinClasses();
}

OuterInterpreter::~OuterInterpreter()
{
    delete mpForthVocab;
    delete mpRootVocab;
    delete mpLiteralsVocab;
    delete mpErrorsVocab;
    delete mpOptypesVocab;
    delete mpLocalVocab;
    delete mpUsingVocab;
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

void OuterInterpreter::Reset(void)
{
    mpVocabStack->Clear(mpForthVocab);

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
    if (name == nullptr)
    {
        return nullptr;
    }
    forthop newestOp = AddOp(value);
    newestOp = COMPILED_OP(flags, newestOp);
    // AddSymbol will call OuterInterpreter::AddOp to add the operators to op table
    forthop *pEntry = mpDefinitionVocab->AddSymbol(name, newestOp);

//#ifdef TRACE_INNER_INTERPRETER
    // add built-in op names to table for TraceOp
    mpEngine->AddOpNameForTracing(name);
	mpCore->numBuiltinOps = mpCore->numOps;
//#endif
	return pEntry;
}


void OuterInterpreter::AddBuiltinOps( baseDictionaryEntry *pEntries )
{
    // I think this assert is a holdover from when userOps and builtinOps were in a single dispatch table
    // assert if this is called after any user ops have been defined
    //ASSERT( mpCore->numUserOps == 0 );

    mpDefinitionVocab = mpForthVocab;
    while ( pEntries->value != nullptr )
    {

        AddBuiltinOp( pEntries->name, (uint32_t)(pEntries->flags), pEntries->value);
        pEntries++;
		//mpCore->numOps++;
    }
}


void OuterInterpreter::AddRootOps(baseDictionaryEntry* pEntries)
{
    // I think this assert is a holdover from when userOps and builtinOps were in a single dispatch table
    // assert if this is called after any user ops have been defined
    //ASSERT( mpCore->numUserOps == 0 );

    mpDefinitionVocab = mpRootVocab;
    while (pEntries->value != nullptr)
    {

        AddBuiltinOp(pEntries->name, (uint32_t)(pEntries->flags), pEntries->value);
        pEntries++;
        //mpCore->numOps++;
    }
    mpDefinitionVocab = mpForthVocab;
}


ClassVocabulary* OuterInterpreter::StartClassDefinition(const char* pClassName, eBuiltinClassIndex classIndex)
{
    SetFlag( kEngineFlagInStructDefinition );
    SetFlag( kEngineFlagInClassDefinition );
	
    TypesManager* pManager = TypesManager::GetInstance();
	ClassVocabulary* pVocab = pManager->StartClassDefinition(pClassName, classIndex);

	// add new class vocab to top of search order
	mpVocabStack->DupTop();
	mpVocabStack->SetTop( pVocab );

    CompileBuiltinOpcode( OP_DO_CLASS_TYPE );
    CompileCell((cell) pVocab);

    return pVocab;
}

void OuterInterpreter::EndClassDefinition()
{
	ClearFlag( kEngineFlagInStructDefinition );
    ClearFlag( kEngineFlagInClassDefinition );

    TypesManager* pManager = TypesManager::GetInstance();
	pManager->EndClassDefinition();
	mpVocabStack->DropTop();
}


ClassVocabulary* OuterInterpreter::StartInterfaceDefinition(const char* pInterfaceName, eBuiltinClassIndex classIndex)
{
    SetFlag(kEngineFlagInClassDefinition);
    SetFlag(kEngineFlagInInterfaceDeclaration);

    TypesManager* pManager = TypesManager::GetInstance();
    ClassVocabulary* pVocab = pManager->StartClassDefinition(pInterfaceName, classIndex, true);

    // add new  vocab to top of search order
    //mpVocabStack->DupTop();
    //mpVocabStack->SetTop(pVocab);

    CompileBuiltinOpcode(OP_DO_CLASS_TYPE);
    CompileCell((cell)pVocab);

    ClassVocabulary* pInterfaceClassVocab = pManager->GetClassVocabulary(kBCIInterface);
    pVocab->Extends(pInterfaceClassVocab);

    return pVocab;
}

void OuterInterpreter::EndInterfaceDefinition()
{
    ClearFlag(kEngineFlagInInterfaceDeclaration);
    ClearFlag(kEngineFlagInClassDefinition);

    TypesManager* pManager = TypesManager::GetInstance();
    pManager->EndClassDefinition();
    //mpVocabStack->DropTop();
}


ClassVocabulary* OuterInterpreter::AddBuiltinClass(
    const char* pClassName,
    eBuiltinClassIndex classIndex,
    eBuiltinClassIndex parentClassIndex, 
    baseMethodEntry *pEntries)
{
    // do "class:" - define class subroutine
	ClassVocabulary* pVocab = StartClassDefinition(pClassName, classIndex);
    TypesManager* pManager = TypesManager::GetInstance();
	ClassVocabulary* pParentClass = pManager->GetClassVocabulary(parentClassIndex);

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
                int32_t methodOp = gCompiledOps[OP_UNIMPLEMENTED];
                // do "method:"
                int32_t methodIndex = pVocab->FindMethod( pMemberName );
                StartOpDefinition( pMemberName, false );
                forthop* pEntry = pVocab->GetNewestEntry();
                methodOp = FORTH_OP_VALUE(*pEntry);
                methodOp = COMPILED_OP(kOpCCode, methodOp);
                if (pEntries->value != nullptr)
                {
                    mpEngine->AddOpNameForTracing(pMemberName);
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
        // add built-in class op name to table for TraceOp
        mpEngine->AddOpNameForTracing(pEntries->name);
#endif
        pEntries++;
    }

    // do ";class"
    ClearFlag( kEngineFlagInStructDefinition );
    pManager->EndClassDefinition();

    return pVocab;
}


// forget the specified op and all higher numbered ops, and free the memory where those ops were stored
void OuterInterpreter::ForgetOp(forthop opNumber, bool quietMode )
{
    if ( opNumber < mpCore->numOps )
    {
        forthop* pNewDP = mpCore->ops[opNumber];
        CleanupGlobalObjectVariables(pNewDP);
        mpDictionary->pCurrent = pNewDP;
        mpCore->numOps = opNumber;
        mpShell->OnForget();
        Forgettable::ForgetPropagate(mpDictionary->pCurrent, opNumber);
    }
    else
    {
        if ( !quietMode )
        {
            SPEW_ENGINE( "OuterInterpreter::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, (int)mpCore->numOps );
            printf( "OuterInterpreter::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, (int)mpCore->numOps );
        }
    }

    Extension* pExtension = mpEngine->GetExtension();
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

    Vocabulary* pFoundVocab = nullptr;
    pEntry = GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pFoundVocab != nullptr )
    {
        op = Vocabulary::GetEntryValue( pEntry );
        opType = Vocabulary::GetEntryType( pEntry );
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
					Forgettable::ForgetPropagate( mpDictionary->pCurrent, op );
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

    // reset search & definitions vocabs in case we deleted a vocab we were using
    SetDefinitionVocabulary(mpForthVocab);
    mpVocabStack->Clear(mpForthVocab);

    return forgotIt;
}

void OuterInterpreter::ShowSearchInfo()
{
	VocabularyStack* pVocabStack = GetVocabularyStack();
	int depth = 0;
	ForthConsoleStringOut(mpCore, "vocab stack:");
	while (true)
	{
		Vocabulary* pVocab = pVocabStack->GetElement(depth);
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

char * OuterInterpreter::GetNextSimpleToken( void )
{
	return mTokenStack.IsEmpty() ? mpShell->GetNextSimpleToken() : mTokenStack.Pop();
}

forthop* OuterInterpreter::StartOpDefinition(const char *pName, bool smudgeIt,
    forthOpType opType, Vocabulary* pDefinitionVocab)
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

    forthop newestOp = gCompiledOps[OP_UNIMPLEMENTED];
    if (pDefinitionVocab->GetType() != VocabularyType::kInterface)
    {
        newestOp = AddOp(mpDictionary->pCurrent);
        newestOp = COMPILED_OP(opType, newestOp);
        mNewestDefinedOp = newestOp;
    }

    forthop* pEntry = pDefinitionVocab->AddSymbol(pName, newestOp);
    if ( smudgeIt )
    {
        pDefinitionVocab->SmudgeNewestSymbol();
    }
	mLabels.clear();

    return pEntry;
}


void OuterInterpreter::EndOpDefinition( bool unsmudgeIt )
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


forthop* OuterInterpreter::FindSymbol( const char *pSymName )
{
    Vocabulary* pFoundVocab = nullptr;
    return GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
}

void OuterInterpreter::DescribeOp( const char* pSymName, forthop op, int32_t auxData )
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
        StructVocabulary::TypecodeToString( auxData, buff2, sizeof(buff2) );
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
            Vocabulary* pVocab = pEnumInfo->pVocab;
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
            char chars[20];
            chars[16] = '\0';
            int numDataToDump = 0;
            int dumpCounter;
            int numDumped = 0;
            while ((curIP < endIP) && notDone)
            {
                SNPRINTF(buff, sizeof(buff), "  +%04x  %p  ", (int)(curIP - baseIP), curIP);
                mpEngine->ConsoleOut(buff);
                if (numDataToDump != 0)
                {
                    char* tbuff = buff;
                    size_t tlen = sizeof(buff);
                    while (numDataToDump > 0)
                    {
                        unsigned char* pc = (unsigned char*)curIP;
                        SNPRINTF(tbuff, tlen, "%02x %02x %02x %02x  ", pc[0], pc[1], pc[2], pc[3] );
                        ((forthop*)chars)[numDumped & 3] = *curIP;
                        tbuff += 13;
                        tlen -= 13;
                        numDumped++;
                        numDataToDump--;
                        dumpCounter += 4;
                        curIP++;
                        if (dumpCounter == 16)
                        {
                            dumpCounter = 0;
                            break;
                        }
                    }
                    mpEngine->ConsoleOut(buff);
                    mpEngine->ConsoleOut("    ");
                    for (int ii = (numDumped & 3); (ii & 3) != 0; ii++)
                    {
                        mpEngine->ConsoleOut("             ");
                    }
                    mpEngine->ConsoleOut("|");
                    mpEngine->ConsoleOut(chars);
                    mpEngine->ConsoleOut("|");

                }
                else
                {
                    mpEngine->DescribeOp(curIP, buff, sizeof(buff), true, numDataToDump);
                    if (numDataToDump != 0)
                    {
                        dumpCounter = 0;
                        numDumped = 0;
                    }
                    curIP++;
                    mpEngine->ConsoleOut(buff);
                }
                mpEngine->ConsoleOut("\n");
                if (((line & 31) == 0) && (mpShell != nullptr) && mpShell->GetInput()->Top()->IsInteractive())
                {
                    mpEngine->ConsoleOut("Hit ENTER to continue, 'q' & ENTER to quit\n");
                    c = mpShell->GetChar();

                    if ((c == 'q') || (c == 'Q'))
                    {
                        c = mpShell->GetChar();
                        notDone = false;
                    }
                }
                //curIP = NextOp(curIP);
                line++;
            }
        }
    }
}

void OuterInterpreter::DescribeSymbol( const char *pSymName )
{
    forthop *pEntry = nullptr;
    char buff[256];

    Vocabulary* pFoundVocab = nullptr;
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

#if 0
// I'm not convinced this is completely unnecessary, the changes to Engine::DescribeOp make it unused right now,
// it would be good to have a single place that contains knowledge about optypes and ops which have immediately
// following data
forthop* OuterInterpreter::NextOp(forthop *pOp )
{
    forthop op = *pOp++;
    int32_t opType = FORTH_OP_TYPE( op );
    int32_t opVal = FORTH_OP_VALUE( op );

    switch ( opType )
    {
        case kOpNative:
        case kOpCCode:
			if ( (opVal == gCompiledOps[OP_INT_VAL]) || (opVal == gCompiledOps[OP_UINT_VAL])
                || (opVal == gCompiledOps[OP_FLOAT_VAL])
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

        case kOpNativeU32: case kOpNativeS32: case kOpNativeF32:
        case kOpCCodeU32: case kOpCCodeS32: case kOpCCodeF32:
        case kOpUserDefU32: case kOpUserDefS32: case kOpUserDefF32:
            pOp++;
            break;

        case kOpNativeS64: case kOpNativeF64:
        case kOpCCodeS64: case kOpCCodeF64:
        case kOpUserDefS64: case kOpUserDefF64:
            pOp += 2;
            break;

        default:
            break;
    }
    return pOp;
}
#endif

void OuterInterpreter::StartStructDefinition( void )
{
    mCompileFlags |= kEngineFlagInStructDefinition;
}

void OuterInterpreter::EndStructDefinition( void )
{
    mCompileFlags &= (~kEngineFlagInStructDefinition);
}

int32_t OuterInterpreter::AddLocalVar( const char        *pVarName,
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
        ControlStackEntry* pEntry = mpShell->GetControlStack()->Peek();
        ucell thingsThatCanHaveLocals = kCSTagDefColon | kCSTagDefNoName | kCSTagDefMethod | kCSTagDefFunction;

        if (!mpShell->CheckSyntaxError("localCheck", pEntry->tag, (ControlStackTag)thingsThatCanHaveLocals))
        {
            mpEngine->SetError(ForthError::badSyntax, "First local variable definition inside control structure");
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
        TypesManager* pManager = TypesManager::GetInstance();
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

bool OuterInterpreter::HasLocalVariables()
{
	return mpLocalVocab->GetFrameCells() != 0;
}

bool OuterInterpreter::AddOpType( forthOpType opType, optypeActionRoutine opAction )
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


char* OuterInterpreter::GetLastInputToken( void )
{
    return mpLastToken;
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
    SPEW_COMPILATION("Compiling int 0x%08x @ 0x%08x\n", v, mpDictionary->pCurrent);
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

void OuterInterpreter::CompileBuiltinOpcode(forthop op )
{
	if ( op < NUM_COMPILED_OPS )
	{
		CompileOpcode( gCompiledOps[op] );
	}
}

void OuterInterpreter::CompileDummyOpcode()
{
    // compile an ABORT opcode, with no peephole optimizations allowed
    ClearPeephole();
    CompileOpcode(gCompiledOps[OP_ABORT]);
    ClearPeephole();
}

void OuterInterpreter::UncompileLastOpcode( void )
{
    forthop *pLastCompiledOpcode = mpOpcodeCompiler->GetLastCompiledOpcodePtr();
    if ( pLastCompiledOpcode != nullptr )
    {
		SPEW_COMPILATION("Uncompiling from %p to %p\n", mpDictionary->pCurrent, pLastCompiledOpcode);
		mpOpcodeCompiler->UncompileLastOpcode();
    }
    else
    {
        SPEW_ENGINE( "OuterInterpreter::UncompileLastOpcode called with no previous opcode\n" );
        mpEngine->SetError( ForthError::missingSize, "UncompileLastOpcode called with no previous opcode" );
    }
}

forthop* OuterInterpreter::GetLastCompiledOpcodePtr( void )
{
	return mpOpcodeCompiler->GetLastCompiledOpcodePtr();
}

forthop* OuterInterpreter::GetLastCompiledIntoPtr( void )
{
	return mpOpcodeCompiler->GetLastCompiledIntoPtr();
}

// interpret/compile a constant value/offset
void OuterInterpreter::ProcessConstant(int64_t value, bool isOffset, bool isSingle)
{
    if ( mCompileState )
    {
        // compile the literal value
        int32_t lvalue = (int32_t)value;
        if (isOffset)
        {
            // TODO: throw error if isSingle is false
            if ((lvalue < (1 << 23)) && (lvalue >= -(1 << 23)))
            {
                // value fits in opcode immediate field
                CompileOpcode(kOpOffset, lvalue & 0xFFFFFF);
            }
            else
            {
                // value too big, must go in next longword
                ClearPeephole();
                CompileBuiltinOpcode(OP_INT_VAL);
                *mpDictionary->pCurrent++ = lvalue;
                CompileBuiltinOpcode(OP_PLUS);
            }
        }
        else
        {
            ClearPeephole();

            // TODO: this makes all small integer constants be compiled as ILIT + 32-bit data on 64-bit systems
            //   since there isSingle is always false
            if (isSingle && (lvalue < (1 << 23)) && (lvalue >= -(1 << 23)))
            {
                // value fits in opcode immediate field
                CompileOpcode((isOffset ? kOpOffset : kOpConstant), lvalue & 0xFFFFFF);
            }
#if defined(FORTH64)
            else if (value < INT_MIN || value > UINT_MAX)
#else
            // on 32-bit system, user specifying 'L' forces a 64-bit value, even
            //  if it can be represented in 32-bits
            else if (!isSingle || value < INT_MIN || value > UINT_MAX)
#endif
            {
                // too big for 32-bits, must compile as 64-bit
                CompileBuiltinOpcode(OP_LONG_VAL);
                forthop* pDP = mpDictionary->pCurrent;
#if defined(FORTH64)
                * (int64_t*)pDP = value;
                pDP += 2;
#else
                stackInt64 val;
                val.s64 = value;
#if 0
                // if long type is reverse ordered on stack in 32-bit mode
                *pDP++ = val.s32[1];
                *pDP++ = val.s32[0];
#else
                *pDP++ = val.s32[0];
                *pDP++ = val.s32[1];
#endif
#endif
                mpDictionary->pCurrent = pDP;
            }
            else if (value <= INT_MAX)
            {
                // value fits in 32-bit signed value
                CompileBuiltinOpcode(OP_INT_VAL);
                *mpDictionary->pCurrent++ = lvalue;
            }
            else
            {
                // value fits in 32-bit unsigned value
                CompileBuiltinOpcode(OP_UINT_VAL);
                *mpDictionary->pCurrent++ = lvalue;
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
#if 0
                // if long type is reverse ordered on stack in 32-bit mode
                *--mpCore->SP = val.s32[0];
                *--mpCore->SP = val.s32[1];
#else
                *--mpCore->SP = val.s32[1];
                *--mpCore->SP = val.s32[0];
#endif
            }
#endif
        }
    }
}

void OuterInterpreter::ProcessDoubleCellConstant(const doubleCell& value)
{
    if (mCompileState)
    {
        // compile the literal value
        ClearPeephole();

#if defined(FORTH64)
        CompileBuiltinOpcode(OP_I128_VAL);
        ucell* pDP = (ucell*)(mpDictionary->pCurrent);
        *pDP++ = value.ucells[1];
        *pDP++ = value.ucells[0];
#else
        CompileBuiltinOpcode(OP_LONG_VAL);
        ucell* pDP = (ucell*)(mpDictionary->pCurrent);
        *pDP++ = value.ucells[0];
        *pDP++ = value.ucells[1];
#endif
        mpDictionary->pCurrent = (forthop*)pDP;
    }
    else
    {
        // leave value on param stack
        *--mpCore->SP = value.ucells[1];
        *--mpCore->SP = value.ucells[0];
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
        forthop opType = FORTH_OP_TYPE(op);
        forthop opValue = FORTH_OP_VALUE(op);
        int dpOffset = mpDictionary->pCurrent - pLastCompiledOpcode;
        if (opType == kOpConstant && dpOffset == 1)
        {
            constantValue = opValue;
            return true;
        }
        else if (op == gCompiledOps[OP_INT_VAL] && dpOffset == 2)
        {
            constantValue = pLastCompiledOpcode[1];
            return true;
        }
    }
    return false;
}

////////////////////////////
//
// enumerated type support
//
void OuterInterpreter::StartEnumDefinition( void )
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

forthop * OuterInterpreter::FindUserDefinition( Vocabulary* pVocab, forthop*& pClosestIP, forthop* pIP, forthop*& pBase  )
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
			ClassVocabulary* pClassVocab = (ClassVocabulary*) pVocab;
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
        mpEngine->SetError(ForthError::badSyntax, "not enough continuations");
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
    if (CheckFeature(kFFAnsiControlOps))
    {
        // no loop continuations (continue/break) in ANSI mode
        return;
    }

    PushContinuationAddress(mContinueDestination);
    PushContinuationType(mContinueCount);
    mContinueDestination = nullptr;
    mContinueCount = 0;
}

void OuterInterpreter::EndLoopContinuations(ucell controlFlowType)  // actually takes a ControlStackTag
{
    if (CheckFeature(kFFAnsiControlOps))
    {
        // no loop continuations (continue/break) in ANSI mode
        return;
    }

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
                    if ((ControlStackTag)controlFlowType != kCSTagDo)
                    {
                        forthop *pBreak = (forthop *)((cell)target & ~1);
                        *pBreak = (forthop)COMPILED_OP(opType, (int32_t)(pDP - pBreak) - 1);
                    }
                    else
                    {
                        mpEngine->SetError(ForthError::badSyntax, "break not allowed in do loop, use leave");
                        break;
                    }
                }
                else
                {
                    if ((ControlStackTag)controlFlowType != kCSTagCase)
                    {
                        if (mContinueDestination != nullptr)
                        {
                            forthop *pContinue = target;
                            *pContinue = (forthop)COMPILED_OP(opType, (int32_t)(mContinueDestination - pContinue) - 1);
                        }
                        else
                        {
                            mpEngine->SetError(ForthError::badSyntax, "end loop with unresolved continues");
                            break;
                        }
                    }
                    else
                    {
                        mpEngine->SetError(ForthError::badSyntax, "continue not allowed in case statement");
                        break;
                    }
                }
            }
            else
            {
                // report error - end loop with continuation stack empty
                mpEngine->SetError(ForthError::badSyntax, "end loop with continuation stack empty");
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

NumberParser& OuterInterpreter::GetNumberParser()
{
    return mNumberParser;
}


void OuterInterpreter::AddGlobalObjectVariable(ForthObject* pObject, Vocabulary* pVocab, const char* pName)
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

// return true IFF compilation occured
bool OuterInterpreter::CompileLocalVariableOpcode(forthop* pEntry, VarOperation varop)
{
    if (varop < VarOperation::numBasicVarops)
    {
        CompileOpcode(*pEntry | (((int)varop) << 20));
        return true;
    }

    // TODO: handle pointer varops
    return false;
}

void OuterInterpreter::AddUsingVocabulary(Vocabulary* vocab)
{
    mpUsingVocab->Push(vocab);
}

void OuterInterpreter::ClearUsingVocabularies()
{
    mpUsingVocab->Empty();
}



//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################

// return true to exit forth shell
OpResult OuterInterpreter::ProcessToken( ParseInfo   *pInfo )
{
    forthop* pEntry;
    int64_t lvalue;
    OpResult exitStatus = OpResult::kOk;
    char *pToken = pInfo->GetToken();
    int len = pInfo->GetTokenLength();
    bool isAString = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_STRING) != 0;
	bool isAQuotedCharacter = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_CHARACTER) != 0;
    bool isSingle, isOffset;
    double* pDPD;
    Vocabulary* pFoundVocab = nullptr;

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
            checkOpResult(exitStatus);
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
            Vocabulary* pVocab = Vocabulary::FindVocabulary( pToken );
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
                    checkOpResult(exitStatus);
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
        exitStatus = pFoundVocab->ProcessEntry( pEntry );
        checkOpResult(exitStatus);
        return exitStatus;
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
            checkOpResult(exitStatus);
            return exitStatus;
        }
    }

	// see if this is an array indexing op like structType[]
    if ( (len > 2) && (strcmp( "[]", &(pToken[len - 2]) ) == 0) )
    {
		// symbol ends with [], see if preceeding token is either a number or a structure type
		pToken[len - 2] = '\0';
		int elementSize = 0;
        StructVocabulary* pStructVocab = mpTypesManager->GetStructVocabulary( pToken );
        if ( pStructVocab != nullptr )
        {
			elementSize = pStructVocab->GetSize();
        }
		else
		{
			NativeType *pNative = mpTypesManager->GetNativeTypeFromName( pToken );
			if ( pNative != nullptr )
			{
				// string[] is not supported
				if ( pNative->GetBaseType() != BaseType::kString )
				{
					elementSize = pNative->GetSize();
				}
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
    NumberType numberType = mNumberParser.ScanNumber(pToken, mpCore->base);
    if (numberType != NumberType::kInvalid)
    {
        ////////////////////////////////////
        //
        // symbol is some type of number
        //
        ////////////////////////////////////
        if (numberType == NumberType::kSingleFloat)
        {
            ////////////////////////////////////
            //
            // symbol is a single precision fp literal
            //
            ////////////////////////////////////
            float fvalue = mNumberParser.GetSingleFloatValue();
            SPEW_OUTER_INTERPRETER("Floating point literal %f\n", fvalue);
            if (mCompileState)
            {
                // compile the literal value
                uint32_t squishedFloat;
                if (SquishFloat(fvalue, false, squishedFloat))
                {
                    CompileOpcode(kOpSquishedFloat, squishedFloat);
                }
                else
                {
                    CompileBuiltinOpcode(OP_FLOAT_VAL);
                    *(float*)mpDictionary->pCurrent++ = fvalue;
                }
            }
            else
            {
                --mpCore->SP;
#if defined(FORTH64)
                *mpCore->SP = 0;
#endif
                * (float*)mpCore->SP = fvalue;
            }
        }
        else if (numberType == NumberType::kDoubleFloat)
        {
            ////////////////////////////////////
            //
            // symbol is a double precision fp literal
            //
            ////////////////////////////////////
            double dvalue = mNumberParser.GetDoubleFloatValue();
            SPEW_OUTER_INTERPRETER("Floating point double literal %g\n", dvalue);
#if defined(SUPPORT_FP_STACK)
            if (mCompileState)
            {
                // compile the literal value
                CompileBuiltinOpcode(OP_DOUBLE_VAL);
                pDPD = (double*)mpDictionary->pCurrent;
                *pDPD++ = dvalue;
                mpDictionary->pCurrent = (forthop*)pDPD;
                if (CheckFeature(kFFFloatingPointStack) != 0)
                {
                    CompileBuiltinOpcode(OP_FPUSH);
                }
            }
            else
            {
                if (CheckFeature(kFFFloatingPointStack) != 0)
                {
                    pushFPStack(mpCore, dvalue);
                }
                else
                {
#if defined(FORTH64)
                    mpCore->SP -= 1;
#else
                    mpCore->SP -= 2;
#endif
                    * (double*)mpCore->SP = dvalue;
                }
            }
#else
            if (mCompileState)
            {
                // compile the literal value
                uint32_t squishedDouble;
                if (SquishDouble(dvalue, false, squishedDouble))
                {
                    CompileOpcode(kOpSquishedDouble, squishedDouble);
                }
                else
                {
                    CompileBuiltinOpcode(OP_DOUBLE_VAL);
                    pDPD = (double*)mpDictionary->pCurrent;
                    *pDPD++ = dvalue;
                    mpDictionary->pCurrent = (forthop*)pDPD;
                }
            }
            else
            {
                if (CheckFeature(kFFFloatingPointStack) != 0)
                {
                    pushFPStack(mpCore, dvalue);
                }
                else
                {
#if defined(FORTH64)
                    mpCore->SP -= 1;
#else
                    mpCore->SP -= 2;
#endif
                    * (double*)mpCore->SP = dvalue;
                }
            }
#endif
        }
        else if (numberType == NumberType::kLong || numberType == NumberType::kInt)
        {
            lvalue = mNumberParser.GetLongValue();
            SPEW_OUTER_INTERPRETER("Integer literal %lld\n", lvalue);
            ProcessConstant(lvalue, mNumberParser.IsOffset(), numberType == NumberType::kInt);
        }
        else if (numberType == NumberType::kDoubleCell)
        {
            ProcessDoubleCellConstant(mNumberParser.GetDoubleCellValue());
        }
       return OpResult::kOk;
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
            CompileBuiltinOpcode( OP_DO_ICONSTANT );
            CompileCell( mNextEnum );
        }
        mNextEnum++;
        return OpResult::kOk;
    }
    else if (CheckFeature(kFFAllowVaropSuffix))
    {
        ////////////////////////////////////
        //
        // last chance - is it something with a varop suffix?
        //
        ////////////////////////////////////
        VarOperation varop = pInfo->CheckVaropSuffix();
        if (varop != VarOperation::varDefaultOp)
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
                    SPEW_OUTER_INTERPRETER("Local variable with varop {%s%s}\n", pToken, ParseInfo::GetVaropSuffix(varop));
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
                            checkOpResult(exitStatus);
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
            if (pEntry != nullptr && 
                ((CODE_TO_BASE_TYPE(pEntry[1]) <= BaseType::kObject) || CODE_IS_PTR(pEntry[1])))
            {
                ////////////////////////////////////
                //
                // symbol is a global variable with varop suffix
                //
                ////////////////////////////////////
                SPEW_OUTER_INTERPRETER("Global variable with varop {%s%s}\n", pToken, ParseInfo::GetVaropSuffix(varop));

                if (mCompileState)
                {
                    // compile NumOpCombo, op is setVarop, num is varop
                    uint32_t opVal = ((gCompiledOps[OP_SETVAROP] & 0xFFFFFF) << 13) | ((uint32_t)varop);
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

    }
    SPEW_ENGINE("Unknown symbol %s\n", pToken);
    mpCore->error = ForthError::undefinedWord;
    exitStatus = OpResult::kError;
    checkOpResult(exitStatus);

    // TODO: return exit-shell flag
    return exitStatus;
}

forthop OuterInterpreter::GetNewestDefinedOp() const
{
    return mNewestDefinedOp;
}

void OuterInterpreter::SetNewestDefinedOp(forthop op)
{
    mNewestDefinedOp = op;
}

const char* OuterInterpreter::GetErrorString(ForthError errorNum) const
{
    std::map<ForthError, const char*>::const_iterator iter = mErrorMap.find(errorNum);
    if (iter != mErrorMap.end())
    {
        return iter->second;
    }
    return "Unknown Error";
}

