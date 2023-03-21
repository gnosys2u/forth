#pragma once
//////////////////////////////////////////////////////////////////////
//
// OuterInterpreter.h: interface for the OuterInterpreter class.
//
//////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <map>
#include <string>

#include "Forth.h"
#include "Shell.h"
#include "ForthInner.h"
#include "StructVocabulary.h"
#include "VocabularyStack.h"
#include "NumberParser.h"

class Fiber;
class Shell;
class OpcodeCompiler;
class Vocabulary;
class LocalVocabulary;
class UsingVocabulary;

typedef enum {
    //kEngineFlagHasLocalVars              = 0x01,
    kEngineFlagInStructDefinition        = 0x02,
    kEngineFlagIsPointer                 = 0x04,
    kEngineFlagInEnumDefinition          = 0x08,
    kEngineFlagIsMethod                  = 0x10,
    kEngineFlagInClassDefinition         = 0x20,
    //kEngineFlagAnsiMode                = 0x40,
    kEngineFlagNoNameDefinition          = 0x80,
    kEngineFlagInInterfaceImplementation = 0x100,       // in implements: ... ;implements
    kEngineFlagInInterfaceDeclaration    = 0x200,       // in interface: ... ;interface
} FECompileFlags;

    //int32_t                *DP;            // dictionary pointer
    //int32_t                *DBase;         // base of dictionary
    //uint32_t               DLen;           // max size of dictionary memory segment

class TokenStack
{
public:
	TokenStack();
	~TokenStack();
	void Initialize(uint32_t numBytes);
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

struct LabelReference
{
	LabelReference(forthop *inBranchIP, cell inBranchType)
		: branchIP(inBranchIP)
		, branchType(inBranchType)
	{
	}

    forthop *branchIP;
	cell branchType;		//  kOpBranch, kOpBranchNZ or kOpBranchZ
};

class Label
{
public:
	Label(const char* pName)
		: name(pName)
		, labelIP(nullptr)
	{
	}

	Label(const char* pName, forthop* inLabelIP)
		: name(pName)
		, labelIP(inLabelIP)
	{
	}

	void CompileBranch(forthop *inBranchIP, cell inBranchType)
	{
		cell offset = (labelIP - inBranchIP) - 1;
		*inBranchIP = COMPILED_OP(inBranchType, offset);
	}

	void AddReference(forthop *inBranchIP, cell inBranchType)
	{
		if (labelIP == nullptr)
		{
			references.emplace_back(LabelReference(inBranchIP, inBranchType));
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
			for (LabelReference& labelReference : references)
			{
				CompileBranch(labelReference.branchIP, labelReference.branchType);
			}
		}
		references.clear();
	}

	std::string name;
	std::vector<LabelReference> references;
    forthop *labelIP;
};

// ForthEnumInfo is compiled with each enum defining word
struct ForthEnumInfo
{
    Vocabulary* pVocab;    // ptr to vocabulary enum is defined in
    int32_t size;                  // enum size in bytes (default to 4)
    int32_t numEnums;              // number of enums defined
    int32_t vocabOffset;           // offset in longs from top of vocabulary to last enum symbol defined
    char nameStart;             // enum name string (including null terminator) plus zero padding to next longword
};

class OuterInterpreter
{
public:
    OuterInterpreter(Engine* pEngine);
    virtual         ~OuterInterpreter();
    void            Initialize();

    void            Reset( void );

    // add an op to the operator dispatch table. returns the assigned opcode (without type field)
    forthop         AddOp( const void *pOp );
    forthop         AddUserOp( const char *pSymbol, forthop** pEntryOut=NULL, bool smudgeIt=false );
    void            AddBuiltinClasses();
    forthop*        AddBuiltinOp( const char* name, uint32_t flags, void* value );
    void            AddBuiltinOps( baseDictionaryEntry *pEntries );

    ClassVocabulary* StartClassDefinition(const char* pClassName, eBuiltinClassIndex classIndex = kNumBuiltinClasses);
    void					EndClassDefinition();
    ClassVocabulary* StartInterfaceDefinition(const char* pInterfaceName, eBuiltinClassIndex classIndex = kNumBuiltinClasses);
    void					EndInterfaceDefinition();
    ClassVocabulary*   AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry *pEntries);

    // forget the specified op and all higher numbered ops, and free the memory where those ops were stored
    void            ForgetOp(forthop opNumber, bool quietMode=true );
    // forget the named symbol - return false if symbol not found
    bool            ForgetSymbol( const char *pSym, bool quietMode=true );

    // return true IFF the last compiled opcode was an integer literal
    bool            GetLastConstant( int32_t& constantValue );

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

    // returns pointer to new vocabulary entry
    forthop*        StartOpDefinition(const char *pName = NULL, bool smudgeIt = false, forthOpType opType = kOpUserDef, Vocabulary* pDefinitionVocab = nullptr);
    void            EndOpDefinition(bool unsmudgeIt = false);
    // return pointer to symbol entry, NULL if not found
    forthop*        FindSymbol( const char *pSymName );
    void            DescribeSymbol( const char *pSymName );
    void            DescribeOp( const char *pSymName, forthop op, int32_t auxData );
    forthop* NextOp(forthop* pOp);

    void            StartStructDefinition( void );
    void            EndStructDefinition( void );
    // returns size of local stack frame in bytes after adding local var
    int32_t            AddLocalVar( const char *pName, int32_t typeCode, int32_t varSize );
    int32_t            AddLocalArray( const char *pName, int32_t typeCode, int32_t varSize );
	bool			HasLocalVariables();

    OpResult    ProcessToken( ParseInfo *pInfo );
    char *          GetLastInputToken( void );

    inline cell* GetCompileStatePtr(void) { return &mCompileState; };
    inline void             SetCompileState(cell v) { mCompileState = v; };
    inline cell             IsCompiling(void) { return mCompileState; };

#if defined(DEBUG)
    void                    CompileInt(int32_t v);
    void                    CompileDouble(double v);
    void                    CompileCell(cell v);
#else
    inline void             CompileInt(int32_t v) { *mpDictionary->pCurrent++ = v; };
    inline void             CompileDouble(double v) { *((double *)mpDictionary->pCurrent) = v; mpDictionary->pCurrent += 2; };
    inline void             CompileCell(cell v) { *((cell*)mpDictionary->pCurrent) = v; mpDictionary->pCurrent += CELL_LONGS; };
#endif
	void					CompileOpcode( forthOpType opType, forthop opVal );
    bool                    CompileLocalVariableOpcode(forthop* pEntry, VarOperation varop);
    void			        PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode);
    void                    ClearPeephole();
    void					CompileOpcode(forthop op );
    void                    CompileBuiltinOpcode(forthop v );
    // use CompileDummyOpcode when building control structures to avoid peephole optimizer breaking things
    void                    CompileDummyOpcode();
    void                    UncompileLastOpcode( void );
    forthop*				GetLastCompiledOpcodePtr( void );
    forthop*				GetLastCompiledIntoPtr( void );
    void                    ProcessConstant( int64_t value, bool isOffset=false, bool isSingle=true );
    void                    ProcessDoubleCellConstant(const doubleCell& value);
	inline TokenStack*      GetTokenStack() { return &mTokenStack; };

    inline Vocabulary  *GetSearchVocabulary( void )   { return mpVocabStack->GetTop(); };
    inline void             SetSearchVocabulary( Vocabulary* pVocab )  { mpVocabStack->SetTop( pVocab ); };
    inline Vocabulary  *GetDefinitionVocabulary( void )   { return mpDefinitionVocab; };
    inline void             SetDefinitionVocabulary( Vocabulary* pVocab )  { mpDefinitionVocab = pVocab; };
    inline LocalVocabulary  *GetLocalVocabulary( void )   { return mpLocalVocab; };
	void					ShowSearchInfo();
    inline Vocabulary  *GetForthVocabulary(void) { return mpForthVocab; };
    inline Vocabulary  *GetLiteralsVocabulary(void) { return mpLiteralsVocab; };
    inline bool             InStructDefinition( void ) { return ((mCompileFlags & kEngineFlagInStructDefinition) != 0); };
    inline bool             HasLocalVars( void ) { return (mpLocalAllocOp != NULL); };
    inline int32_t             GetFlags( void ) { return mCompileFlags; };
    inline void             SetFlags( int32_t flags ) { mCompileFlags = flags; };
    inline void             SetFlag( int32_t flags ) { mCompileFlags |= flags; };
    inline void             ClearFlag( int32_t flags ) { mCompileFlags &= (~flags); };
    inline int32_t             CheckFlag( int32_t flags ) { return mCompileFlags & flags; };
    inline int32_t&            GetFeatures( void ) { return mFeatures; };
    inline void             SetFeatures( int32_t features ) { mFeatures = features; };
    inline void             SetFeature( int32_t features ) { mFeatures |= features; };
    inline void             ClearFeature( int32_t features ) { mFeatures &= (~features); };
    inline int32_t             CheckFeature( int32_t features ) { return mFeatures & features; };
    inline char *           GetTempBuffer(void) { return mpTempBuffer; };
	inline int				GetTempBufferSize( void ) { return MAX_STRING_SIZE; };
    char *                  GrabTempBuffer(void);
    void                    UngrabTempBuffer(void);
    inline void             SetArraySize(int32_t numElements)        { mNumElements = numElements; };
    inline int32_t             GetArraySize( void )                    { return mNumElements; };
    inline ForthEnumInfo*   GetNewestEnumInfo(void) { return mpNewestEnum; };
    void                    SetNewestEnumInfo(ForthEnumInfo *pInfo) { mpNewestEnum = pInfo; };
    void                    StartEnumDefinition( void );
    void                    EndEnumDefinition( void );

    VocabularyStack*   GetVocabularyStack( void )              { return mpVocabStack; };

    void AddUsingVocabulary(Vocabulary* vocab);
    void ClearUsingVocabularies();

	// squish float/double down to 24-bits, returns true IFF number can be represented exactly OR approximateOkay==true and number is within range of squished float
	bool					SquishFloat( float fvalue, bool approximateOkay, uint32_t& squishedFloat );
	float					UnsquishFloat( uint32_t squishedFloat );

	bool					SquishDouble( double dvalue, bool approximateOkay, uint32_t& squishedDouble );
	double					UnsquishDouble( uint32_t squishedDouble );
	// squish 64-bit int to 24 bits, returns true IFF number can be represented in 24 bits
	bool					SquishLong( int64_t lvalue, uint32_t& squishedLong );
	int64_t				UnsquishLong( uint32_t squishedLong );

	void					DefineLabel(const char* inLabelName, forthop* inLabelIP);
	void					AddGoto(const char* inName, int inBranchType, forthop* inBranchIP);

	// if inText is null, string is not copied, an uninitialized space of size inNumChars+1 is allocated
	// if inNumChars is null and inText is not null, strlen(inText) is used for temp string size
	// if both inText and inNumChars are null, an uninitialized space of 255 chars is allocated
	char*					AddTempString(const char* inText = nullptr, cell inNumChars = -1);
    inline cell             UnusedTempStringSpace() { return (mStringBufferASize - (mpStringBufferANext - mpStringBufferA)); }

    void                    AddGlobalObjectVariable(ForthObject* pObject, Vocabulary* pVocab, const char* pName);
    void                    CleanupGlobalObjectVariables(forthop* pNewDP);

    forthop*                FindUserDefinition(Vocabulary* pVocab, forthop*& pClosestIP, forthop* pIP, forthop*& pBase);

private:

    Engine* mpEngine;
    Shell* mpShell;
    CoreState* mpCore;             // core inner interpreter state of main thread first fiber
    MemorySection* mpDictionary;

    TokenStack mTokenStack;		// contains tokens which will be gotten by GetNextSimpleToken instead of from input stream

    Vocabulary * mpForthVocab;              // main forth vocabulary
    Vocabulary * mpLiteralsVocab;            // user-defined literals vocabulary
    LocalVocabulary * mpLocalVocab;         // local variable vocabulary
    UsingVocabulary* mpUsingVocab;          // vocab for 'using:' feature

    Vocabulary * mpDefinitionVocab;    // vocabulary which new definitions are added to
    VocabularyStack * mpVocabStack;

	OpcodeCompiler* mpOpcodeCompiler;
    std::vector<ForthObject*> mGlobalObjectVariables;

    char        *mpStringBufferA;       // string buffer A is used for quoted strings when in interpreted mode
    char        *mpStringBufferANext;   // one char past last used in A
    int         mStringBufferASize;

    char        *mpTempBuffer;
#if defined(WINDOWS_BUILD)
    CRITICAL_SECTION* mpTempBufferLock;
#else
    pthread_mutex_t* mpTempBufferLock;
#endif

    char *          mpLastToken;
    int32_t            mLocalFrameSize;
    forthop*        mpLocalAllocOp;

    TypesManager *mpTypesManager;

    interpreterExtensionRoutine *mpInterpreterExtension;

    cell        mCompileState;          // true iff compiling
    int32_t            mCompileFlags;
    int32_t            mFeatures;
    int32_t            mNumElements;       // number of elements in next array declared

    cell*           mpEnumStackBase;
    cell            mNextEnum;

	std::vector<Label> mLabels;

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
    int32_t            mContinuationIx;
    forthop*        mContinueDestination;
    cell            mContinueCount;
    NumberParser    mNumberParser;

    ForthEnumInfo*  mpNewestEnum;
};

