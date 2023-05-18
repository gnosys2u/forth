#pragma once
//////////////////////////////////////////////////////////////////////
//
// Engine.h: interface for the Engine class.
//
//////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#if defined(LINUX)
#include <time.h>
#else
#include <sys/timeb.h>
#endif
#include <map>
#include <string>

#include "Forth.h"
#include "Thread.h"
#include "Shell.h"
#include "ForthInner.h"
#include "Vocabulary.h"
#include "StructVocabulary.h"

class Thread;
class Fiber;
class Shell;
class Extension;
class OpcodeCompiler;
class BlockFileManager;
class OuterInterpreter;

#define DEFAULT_USER_STORAGE 16384

#define MAIN_THREAD_PSTACK_LONGS   8192
#define MAIN_THREAD_RSTACK_LONGS   8192

// this is the size of the buffer returned by GetTmpStringBuffer()
//  which is the buffer used by word and blword
#define TMP_STRING_BUFFER_LEN MAX_STRING_SIZE

class Engine
{
public:
    Engine();
    virtual         ~Engine();

    void            Initialize(Shell* pShell,
        int storageLongs = DEFAULT_USER_STORAGE,
        bool bAddBaseOps = true,
        Extension* pExtension = NULL);
    void            Reset(void);
    void            ErrorReset(void);

    void            SetFastMode(bool goFast);
    void            ToggleFastMode(void);
    bool            GetFastMode(void);

    //
    // FullyExecuteOp is used by the Outer Interpreter (Engine::ProcessToken) to
    // execute forth ops, and is also how systems external to forth execute ops
    //
    OpResult        FullyExecuteOp(CoreState* pCore, forthop opCode);
    // ExecuteOp will start execution of an op, but will not finish user defs or methods
    OpResult        ExecuteOp(CoreState* pCore, forthop opCode);
    // ExecuteOps executes a sequence of forth ops
    // The sequence must be terminated with an OP_DONE
    OpResult        ExecuteOps(CoreState* pCore, forthop* pOps);

    OpResult		FullyExecuteMethod(CoreState* pCore, ForthObject& obj, int methodNum);
    OpResult        DeleteObject(CoreState* pCore, ForthObject& obj);
    void                ReleaseObject(CoreState* pCore, ForthObject& inObject);

    inline forthop* GetDP() { return mDictionary.pCurrent; };
    inline void             SetDP(forthop* pNewDP) { mDictionary.pCurrent = pNewDP; };
    inline void             AllotLongs(int n) { mDictionary.pCurrent += n; };
    inline void             AllotBytes(int n) { mDictionary.pCurrent = reinterpret_cast<forthop*>(reinterpret_cast<cell>(mDictionary.pCurrent) + n); };
    inline void             AlignDP(void) {
        mDictionary.pCurrent = (forthop*)((((cell)mDictionary.pCurrent) + (sizeof(forthop) - 1))
            & ~(sizeof(forthop) - 1));
    };
    inline MemorySection* GetDictionaryMemorySection() { return &mDictionary; };

    void                    ShowMemoryInfo();
    inline Shell* GetShell(void) { return mpShell; };
    inline void				SetShell(Shell* pShell) { mpShell = pShell; };
    inline Fiber* GetMainFiber(void) { return mpMainThread->GetFiber(0); };

    cell* GetCompileStatePtr(void);
    void            SetCompileState(cell v);
    cell            IsCompiling(void);

    void                    GetErrorString(ForthError err, char* pBuffer, int bufferSize);
    OpResult            CheckStacks(void);
    void                    SetError(ForthError e, const char* pString = NULL);
    void                    AddErrorText(const char* pString);
    void                    SetFatalError(ForthError e, const char* pString = NULL);
    inline ForthError      GetError(void) { return (ForthError)(mpCore->error); };
    // for some reason, inlining this makes it return bogus data in some cases - WTF?
    //inline CoreState*  GetCoreState( void ) { return mpCore; };
    CoreState* GetCoreState(void);

    static Engine* GetInstance(void);

    void					SetDefaultConsoleOut(ForthObject& newOutStream);
    void					SetConsoleOut(CoreState* pCore, ForthObject& newOutStream);
    void					SetErrorOut(CoreState* pCore, ForthObject& newOutStream);
    void* GetErrorOut(CoreState* pCore);
    void					PushConsoleOut(CoreState* pCore);
    void					PushErrorOut(CoreState* pCore);
    void					PushDefaultConsoleOut(CoreState* pCore);
    void					ResetConsoleOut(CoreState& core);
    void					ResetConsoleOut();

    // return milliseconds since engine was created
    uint32_t           GetElapsedTime(void);

    void                    ConsoleOut(const char* pBuffer);
    void                    TraceOut(const char* pFormat, ...);

    int32_t					GetTraceFlags(void);
    void					SetTraceFlags(int32_t flags);

    void					SetTraceOutRoutine(traceOutRoutine traceRoutine, void* pTraceData);
    void					GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const;

    const char*             GetOpTypeName(int32_t opType) const;
    void                    TraceOp(forthop* pOp, forthop op);
    void                    TraceStack(CoreState* pCore);
    void                    DescribeOp(forthop* pOp, char* pBuffer, int buffSize, bool lookupUserDefs, int& numFollowing);
    void                    AddOpExecutionToProfile(forthop op);
    void                    DumpExecutionProfile();
    void                    ResetExecutionProfile();

    void					DumpCrashState();
    void					DisplayUserDefCrash(forthop* pRVal, char* buff, int buffSize);

    void                    RaiseException(CoreState* pCore, ForthError exceptionNum);
    void                    ContinueException(CoreState* pCore);

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    Thread* CreateThread(forthop fiberOp = OP_DONE, int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE);
    void            DestroyThread(Thread* pThread);

    void InitCoreState(CoreState& core);

    BlockFileManager* GetBlockFileManager();

    // returns true IFF file opened successfully
    bool            PushInputFile(const char* pInFileName);
    void            PushInputBuffer(const char* pDataBuffer, int dataBufferLen);
    void            PushInputBlocks(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock);
    void            PopInputStream(void);

    bool					IsServer() const;
    void					SetIsServer(bool isServer);

    inline Extension* GetExtension() { return mpExtension; }

    ClassVocabulary* AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex,
        eBuiltinClassIndex parentClassIndex, baseMethodEntry* pEntries);

    inline OuterInterpreter* GetOuterInterpreter() { return mpOuter; }

    void AddOpNameForTracing(const char* pName);

    void Interrupt();

    // can return null if vocabulary not found
    Vocabulary* GetVocabulary(ucell wordlistId);
    void AddVocabulary(Vocabulary* pVocab);
    void RemoveVocabulary(Vocabulary* pVocab);

private:
    OuterInterpreter* mpOuter;
    BlockFileManager* mBlockFileManager;

    CoreState* mpCore;             // core inner interpreter state

    MemorySection mDictionary;

    Thread* mpThreads;
    Thread* mpMainThread;
    Shell* mpShell;
    int32_t* mpEngineScratch;
    char* mpErrorString;  // optional error information from shell
    traceOutRoutine	mTraceOutRoutine;
    void* mpTraceOutData;
    ForthObject		mDefaultConsoleOutStream;
    ForthObject     mErrorOutStream;


    struct opcodeProfileInfo {
        forthop op;
        ucell count;
        Vocabulary* pVocabulary;
        forthop* pEntry;
    };
    std::vector<opcodeProfileInfo> mProfileOpcodeCounts;
    cell mProfileOpcodeTypeCounts[256];

#if defined(WINDOWS_BUILD)
#ifdef MSDEV
    //struct _timeb   mStartTime;
    struct __timeb32	mStartTime;
#else
    struct _timeb    mStartTime;
#endif
#elif defined(LINUX)
    struct timespec mStartTime;
#else
    struct timeb    mStartTime;
#endif

    Extension* mpExtension;

    static Engine* mpInstance;
    bool            mFastMode;
    bool			mIsServer;

    std::map<ucell, Vocabulary*> mWordlistMap;

};

