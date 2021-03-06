#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthEngine.h: interface for the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <sys/timeb.h>
#include <map>
#include <string>

#include "Forth.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthInner.h"
#include "ForthVocabulary.h"
#include "ForthStructs.h"

class ForthThread;
class ForthFiber;
class ForthShell;
class ForthExtension;
class ForthOpcodeCompiler;
class ForthBlockFileManager;

#define DEFAULT_USER_STORAGE 16384

#define MAIN_THREAD_PSTACK_LONGS   8192
#define MAIN_THREAD_RSTACK_LONGS   8192

// this is the size of the buffer returned by GetTmpStringBuffer()
//  which is the buffer used by word and blword
#define TMP_STRING_BUFFER_LEN MAX_STRING_SIZE

typedef enum {
    //kEngineFlagHasLocalVars              = 0x01,
    kEngineFlagInStructDefinition        = 0x02,
    kEngineFlagIsPointer                 = 0x04,
    kEngineFlagInEnumDefinition          = 0x08,
    kEngineFlagIsMethod                  = 0x10,
    kEngineFlagInClassDefinition         = 0x20,
    //kEngineFlagAnsiMode                = 0x40,
    kEngineFlagNoNameDefinition          = 0x80,
} FECompileFlags;

    //long                *DP;            // dictionary pointer
    //long                *DBase;         // base of dictionary
    //ulong               DLen;           // max size of dictionary memory segment

class ForthEngineTokenStack
{
public:
	ForthEngineTokenStack();
	~ForthEngineTokenStack();
	void Initialize(ulong numBytes);
	inline bool IsEmpty() const { return mpCurrent == mpLimit; };
	void Push(const char* pToken);
	char* Pop();
	char* Peek();
	void Clear();

private:
	char*               mpCurrent;
	char*               mpBase;
	char*				mpLimit;
	size_t              mNumBytes;
};

struct ForthLabelReference
{
	ForthLabelReference(forthop *inBranchIP, int inBranchType)
		: branchIP(inBranchIP)
		, branchType(inBranchType)
	{
	}

    forthop *branchIP;
	int branchType;		//  kOpBranch, kOpBranchNZ or kOpBranchZ
};

class ForthLabel
{
public:
	ForthLabel(const char* pName)
		: name(pName)
		, labelIP(nullptr)
	{
	}

	ForthLabel(const char* pName, forthop* inLabelIP)
		: name(pName)
		, labelIP(inLabelIP)
	{
	}

	void CompileBranch(forthop *inBranchIP, int inBranchType)
	{
		cell offset = (labelIP - inBranchIP) - 1;
		*inBranchIP = COMPILED_OP(inBranchType, offset);
	}

	void AddReference(forthop *inBranchIP, int inBranchType)
	{
		if (labelIP == nullptr)
		{
			references.emplace_back(ForthLabelReference(inBranchIP, inBranchType));
		}
		else
		{
			CompileBranch(inBranchIP, inBranchType);
		}
	}

	void DefineLabelIP(forthop *inLabelIP)
	{
		labelIP = inLabelIP;
		if (references.size() > 0)
		{
			for (ForthLabelReference& labelReference : references)
			{
				CompileBranch(labelReference.branchIP, labelReference.branchType);
			}
		}
		references.clear();
	}

	std::string name;
	std::vector<ForthLabelReference> references;
    forthop *labelIP;
};

// ForthEnumInfo is compiled with each enum defining word
struct ForthEnumInfo
{
    ForthVocabulary* pVocab;    // ptr to vocabulary enum is defined in
    long size;                  // enum size in bytes (default to 4)
    long numEnums;              // number of enums defined
    long vocabOffset;           // offset in longs from top of vocabulary to last enum symbol defined
    char nameStart;             // enum name string (including null terminator) plus zero padding to next longword
};

class ForthEngine
{
public:
                    ForthEngine();
    virtual         ~ForthEngine();

    void            Initialize( ForthShell* pShell,
                                int storageLongs=DEFAULT_USER_STORAGE,
                                bool bAddBaseOps=true,
                                ForthExtension* pExtension=NULL );
    void            Reset( void );
    void            ErrorReset( void );

    void            SetFastMode( bool goFast );
    void            ToggleFastMode( void );
    bool            GetFastMode( void );

    //
    // FullyExecuteOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
    // execute forth ops, and is also how systems external to forth execute ops
    //
	eForthResult        FullyExecuteOp(ForthCoreState* pCore, forthop opCode);
	// ExecuteOp will start execution of an op, but will not finish user defs or methods
	eForthResult        ExecuteOp(ForthCoreState* pCore, forthop opCode);
	// ExecuteOps executes a sequence of forth ops
    // The sequence must be terminated with an OP_DONE
	eForthResult        ExecuteOps(ForthCoreState* pCore, forthop* pOps);

	eForthResult		FullyExecuteMethod(ForthCoreState* pCore, ForthObject& obj, int methodNum);

    // add an op to the operator dispatch table. returns the assigned opcode (without type field)
    forthop         AddOp( const void *pOp );
    forthop         AddUserOp( const char *pSymbol, forthop** pEntryOut=NULL, bool smudgeIt=false );
    forthop*        AddBuiltinOp( const char* name, ulong flags, void* value );
    void            AddBuiltinOps( baseDictionaryEntry *pEntries );

	ForthClassVocabulary*   StartClassDefinition(const char* pClassName, eBuiltinClassIndex classIndex = kNumBuiltinClasses);
	void					EndClassDefinition();
	ForthClassVocabulary*   AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry *pEntries);

    // forget the specified op and all higher numbered ops, and free the memory where those ops were stored
    void            ForgetOp(forthop opNumber, bool quietMode=true );
    // forget the named symbol - return false if symbol not found
    bool            ForgetSymbol( const char *pSym, bool quietMode=true );

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    ForthThread *   CreateThread(forthop fiberOp = OP_DONE, int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE );
	void            DestroyThread(ForthThread *pThread);

    void InitCoreState(ForthCoreState& core);

    // return true IFF the last compiled opcode was an integer literal
    bool            GetLastConstant( long& constantValue );

    // add a user-defined extension to the outer interpreter
    inline void     SetInterpreterExtension( interpreterExtensionRoutine *pRoutine )
    {
        mpInterpreterExtension = pRoutine;
    };

    // add a user-defined forthop type
    // opType should be in range kOpLocalUserDefined ... kOpMaxLocalUserDefined
    // return false IFF opType is out of range
    bool            AddOpType( forthOpType opType, optypeActionRoutine opAction );

    char *          GetNextSimpleToken( void );

    // returns true IFF file opened successfully
    bool            PushInputFile( const char *pInFileName );
    void            PushInputBuffer( const char *pDataBuffer, int dataBufferLen );
    void            PushInputBlocks(ForthBlockFileManager*  pManager, unsigned int firstBlock, unsigned int lastBlock);
    void            PopInputStream( void );

    // returns pointer to new vocabulary entry
    forthop*        StartOpDefinition(const char *pName = NULL, bool smudgeIt = false, forthOpType opType = kOpUserDef, ForthVocabulary* pDefinitionVocab = nullptr);
    void            EndOpDefinition(bool unsmudgeIt = false);
    // return pointer to symbol entry, NULL if not found
    forthop*        FindSymbol( const char *pSymName );
    void            DescribeSymbol( const char *pSymName );
    void            DescribeOp( const char *pSymName, forthop op, long auxData );

    void            StartStructDefinition( void );
    void            EndStructDefinition( void );
    // returns size of local stack frame in bytes after adding local var
    long            AddLocalVar( const char *pName, long typeCode, long varSize );
    long            AddLocalArray( const char *pName, long typeCode, long varSize );
	bool			HasLocalVariables();

    eForthResult    ProcessToken( ForthParseInfo *pInfo );
    char *          GetLastInputToken( void );

    const char *            GetOpTypeName( long opType );
	void                    TraceOp(forthop *pOp, forthop op);
	void                    TraceStack(ForthCoreState* pCore);
    void                    DescribeOp(forthop *pOp, char *pBuffer, int buffSize, bool lookupUserDefs=false );
    forthop*                NextOp(forthop *pOp );
    void                    AddOpExecutionToProfile(forthop op);
    void                    DumpExecutionProfile();
    void                    ResetExecutionProfile();

    inline forthop*         GetDP() { return mDictionary.pCurrent; };
    inline void             SetDP( forthop* pNewDP ) { mDictionary.pCurrent = pNewDP; };
#if defined(DEBUG)
    void                    CompileInt(long v);
    void                    CompileDouble(double v);
    void                    CompileCell(cell v);
#else
    inline void             CompileInt(long v) { *mDictionary.pCurrent++ = v; };
    inline void             CompileDouble(double v) { *((double *)mDictionary.pCurrent) = v; mDictionary.pCurrent += 2; };
    inline void             CompileCell(cell v) { *((cell*)mDictionary.pCurrent) = v; mDictionary.pCurrent += CELL_LONGS; };
#endif
	void					CompileOpcode( forthOpType opType, forthop opVal );
    void			        PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode);
    void                    ClearPeephole();
    void					CompileOpcode(forthop op );
    void                    CompileBuiltinOpcode(forthop v );
    void                    UncompileLastOpcode( void );
    forthop*				GetLastCompiledOpcodePtr( void );
    forthop*				GetLastCompiledIntoPtr( void );
    void                    ProcessConstant( int64_t value, bool isOffset=false, bool isSingle=true );
    inline void             AllotLongs( int n ) { mDictionary.pCurrent += n; };
	inline void             AllotBytes( int n )	{ mDictionary.pCurrent = reinterpret_cast<forthop*>(reinterpret_cast<cell>(mDictionary.pCurrent) + n); };
    inline void             AlignDP( void ) { mDictionary.pCurrent = (forthop*)(( ((cell)mDictionary.pCurrent) + (sizeof(forthop) - 1))
                                                                                    & ~(sizeof(forthop) - 1)); };
    inline ForthMemorySection* GetDictionaryMemorySection() { return &mDictionary; };

	inline ForthEngineTokenStack* GetTokenStack() { return &mTokenStack; };

    inline ForthVocabulary  *GetSearchVocabulary( void )   { return mpVocabStack->GetTop(); };
    inline void             SetSearchVocabulary( ForthVocabulary* pVocab )  { mpVocabStack->SetTop( pVocab ); };
    inline ForthVocabulary  *GetDefinitionVocabulary( void )   { return mpDefinitionVocab; };
    inline void             SetDefinitionVocabulary( ForthVocabulary* pVocab )  { mpDefinitionVocab = pVocab; };
    inline ForthLocalVocabulary  *GetLocalVocabulary( void )   { return mpLocalVocab; };
	void					ShowSearchInfo();
    inline ForthShell       *GetShell( void ) { return mpShell; };
	inline void				SetShell( ForthShell *pShell ) { mpShell = pShell; };
    inline ForthVocabulary  *GetForthVocabulary(void) { return mpForthVocab; };
    inline ForthVocabulary  *GetLiteralsVocabulary(void) { return mpLiteralsVocab; };
    inline ForthFiber       *GetMainFiber( void )  { return mpMainThread->GetFiber(0); };

    inline cell             *GetCompileStatePtr( void ) { return &mCompileState; };
    inline void             SetCompileState( cell v ) { mCompileState = v; };
    inline cell             IsCompiling( void ) { return mCompileState; };
    inline bool             InStructDefinition( void ) { return ((mCompileFlags & kEngineFlagInStructDefinition) != 0); };
    inline bool             HasLocalVars( void ) { return (mpLocalAllocOp != NULL); };
    inline long             GetFlags( void ) { return mCompileFlags; };
    inline void             SetFlags( long flags ) { mCompileFlags = flags; };
    inline void             SetFlag( long flags ) { mCompileFlags |= flags; };
    inline void             ClearFlag( long flags ) { mCompileFlags &= (~flags); };
    inline long             CheckFlag( long flags ) { return mCompileFlags & flags; };
    inline long&            GetFeatures( void ) { return mFeatures; };
    inline void             SetFeatures( long features ) { mFeatures = features; };
    inline void             SetFeature( long features ) { mFeatures |= features; };
    inline void             ClearFeature( long features ) { mFeatures &= (~features); };
    inline long             CheckFeature( long features ) { return mFeatures & features; };
    inline char *           GetTempBuffer(void) { return mpTempBuffer; };
	inline int				GetTempBufferSize( void ) { return MAX_STRING_SIZE; };
    char *                  GrabTempBuffer(void);
    void                    UngrabTempBuffer(void);
    inline void             SetArraySize(long numElements)        { mNumElements = numElements; };
    inline long             GetArraySize( void )                    { return mNumElements; };
    inline ForthEnumInfo*   GetNewestEnumInfo(void) { return mpNewestEnum; };
    void                    SetNewestEnumInfo(ForthEnumInfo *pInfo) { mpNewestEnum = pInfo; };
    void                    GetErrorString( char *pBuffer, int bufferSize );
    eForthResult            CheckStacks( void );
    void                    SetError( eForthError e, const char *pString = NULL );
    void                    AddErrorText( const char *pString );
    void                    SetFatalError( eForthError e, const char *pString = NULL );
    inline eForthError      GetError( void ) { return (eForthError) (mpCore->error); };
    // for some reason, inlining this makes it return bogus data in some cases - WTF?
	//inline ForthCoreState*  GetCoreState( void ) { return mpCore; };
    ForthCoreState*			GetCoreState( void );

    void                    StartEnumDefinition( void );
    void                    EndEnumDefinition( void );

    ForthVocabularyStack*   GetVocabularyStack( void )              { return mpVocabStack; };

    static ForthEngine*     GetInstance( void );

	void					SetDefaultConsoleOut( ForthObject& newOutStream );
    void					SetConsoleOut(ForthCoreState* pCore, ForthObject& newOutStream);
    void					SetAuxOut(ForthCoreState* pCore, ForthObject& newOutStream);
    void					PushConsoleOut( ForthCoreState* pCore );
	void					PushDefaultConsoleOut( ForthCoreState* pCore );
    void                    PushAuxOut(ForthCoreState* pCore);
	void					ResetConsoleOut( ForthCoreState& core );

    // return milliseconds since engine was created
    unsigned long           GetElapsedTime( void );

	void                    ConsoleOut( const char* pBuffer );
	void                    TraceOut( const char* pFormat, ... );

	long					GetTraceFlags( void );
	void					SetTraceFlags( long flags );

	void					SetTraceOutRoutine(traceOutRoutine traceRoutine, void* pTraceData);
	void					GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const;

	void					DumpCrashState();

	// squish float/double down to 24-bits, returns true IFF number can be represented exactly OR approximateOkay==true and number is within range of squished float
	bool					SquishFloat( float fvalue, bool approximateOkay, ulong& squishedFloat );
	float					UnsquishFloat( ulong squishedFloat );

	bool					SquishDouble( double dvalue, bool approximateOkay, ulong& squishedDouble );
	double					UnsquishDouble( ulong squishedDouble );
	// squish 64-bit int to 24 bits, returns true IFF number can be represented in 24 bits
	bool					SquishLong( int64_t lvalue, ulong& squishedLong );
	int64_t				UnsquishLong( ulong squishedLong );

    ForthBlockFileManager*  GetBlockFileManager();

	bool					IsServer() const;
	void					SetIsServer(bool isServer);

	void					DefineLabel(const char* inLabelName, forthop* inLabelIP);
	void					AddGoto(const char* inName, int inBranchType, forthop* inBranchIP);

	// if inText is null, string is not copied, an uninitialized space of size inNumChars+1 is allocated
	// if inNumChars is null and inText is not null, strlen(inText) is used for temp string size
	// if both inText and inNumChars are null, an uninitialized space of 255 chars is allocated
	char*					AddTempString(const char* inText = nullptr, int inNumChars = -1);
    inline cell             UnusedTempStringSpace() { return (mStringBufferASize - (mpStringBufferANext - mpStringBufferA)); }

    void                    AddGlobalObjectVariable(ForthObject* pObject);
    void                    CleanupGlobalObjectVariables(forthop* pNewDP);

    void                    RaiseException(ForthCoreState* pCore, cell exceptionNum);

protected:
    // NOTE: temporarily modifies string @pToken
    bool                    ScanIntegerToken( char* pToken, int64_t& value, int base, bool& isOffset, bool& isSingle );
    // NOTE: temporarily modifies string @pToken
    bool                    ScanFloatToken( char *pToken, float& fvalue, double& dvalue, bool& isSingle, bool& isApproximate );


    forthop*                FindUserDefinition( ForthVocabulary* pVocab, forthop*& pClosestIP, forthop* pIP, forthop*& pBase );
	void					DisplayUserDefCrash(forthop *pRVal, char* buff, int buffSize );

protected:
    ForthCoreState*  mpCore;             // core inner interpreter state

    ForthMemorySection mDictionary;

	ForthEngineTokenStack mTokenStack;		// contains tokens which will be gotten by GetNextSimpleToken instead of from input stream

    ForthVocabulary * mpForthVocab;              // main forth vocabulary
    ForthVocabulary * mpLiteralsVocab;            // user-defined literals vocabulary
    ForthLocalVocabulary * mpLocalVocab;         // local variable vocabulary

    ForthVocabulary * mpDefinitionVocab;    // vocabulary which new definitions are added to
    ForthVocabularyStack * mpVocabStack;

	ForthOpcodeCompiler* mpOpcodeCompiler;
    ForthBlockFileManager* mBlockFileManager;

    char        *mpStringBufferA;       // string buffer A is used for quoted strings when in interpreted mode
    char        *mpStringBufferANext;   // one char past last used in A
    int         mStringBufferASize;

    char        *mpTempBuffer;
#if defined(WINDOWS_BUILD)
    CRITICAL_SECTION* mpTempBufferLock;
#else
    pthread_mutex_t* mpTempBufferLock;
#endif

    cell        mCompileState;          // true iff compiling

	ForthThread * mpThreads;
	ForthThread * mpMainThread;
    ForthShell  *   mpShell;
    long *          mpEngineScratch;
    char *          mpLastToken;
    long            mLocalFrameSize;
    forthop*        mpLocalAllocOp;
    char *          mpErrorString;  // optional error information from shell

    ForthTypesManager *mpTypesManager;

    interpreterExtensionRoutine *mpInterpreterExtension;

    long            mCompileFlags;
    long            mFeatures;
    long            mNumElements;       // number of elements in next array declared

	traceOutRoutine	mTraceOutRoutine;
	void*			mpTraceOutData;

    cell*           mpEnumStackBase;
    cell            mNextEnum;

	ForthObject		mDefaultConsoleOutStream;
    ForthObject     mAuxOutStream;

	std::vector<ForthLabel> mLabels;

    std::vector<ForthObject*> mGlobalObjectVariables;

    struct opcodeProfileInfo {
        forthop op;
        ucell count;
        ForthVocabulary* pVocabulary;
        forthop* pEntry;
    };
    std::vector<opcodeProfileInfo> mProfileOpcodeCounts;
    cell mProfileOpcodeTypeCounts[256];

public:
    void                    PushContinuationAddress(forthop* pOP);
    void                    PushContinuationType(cell val);
    forthop*                PopContinuationAddress();
    cell                    PopContinuationType();
    void                    ResetContinuations();
    forthop*                GetContinuationDestination();
    void                    SetContinuationDestination(forthop* pDest);
    void                    AddContinuationBranch(forthop* pAddr, cell opType);
    void                    AddBreakBranch(forthop* pAddr, cell opType);
    void                    StartLoopContinuations();
    void                    EndLoopContinuations(int controlFlowType);  // actually takes a eShellTag
    bool                    HasPendingContinuations();

protected:
    std::vector<forthop*> mContinuations;
    long            mContinuationIx;
    forthop*        mContinueDestination;
    long            mContinueCount;

    ForthEnumInfo*  mpNewestEnum;

#if defined(WINDOWS_BUILD)
#ifdef MSDEV
    //struct _timeb   mStartTime;
	struct __timeb32	mStartTime;
#else
    struct _timeb    mStartTime;
#endif
#else
    struct timeb    mStartTime;
#endif

    ForthExtension* mpExtension;

    static ForthEngine* mpInstance;
    bool            mFastMode;
	bool			mIsServer;
};

