#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthEngine.h: interface for the ForthEngine class.
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
class OuterInterpreter;

#define DEFAULT_USER_STORAGE 16384

#define MAIN_THREAD_PSTACK_LONGS   8192
#define MAIN_THREAD_RSTACK_LONGS   8192

// this is the size of the buffer returned by GetTmpStringBuffer()
//  which is the buffer used by word and blword
#define TMP_STRING_BUFFER_LEN MAX_STRING_SIZE

class ForthEngine
{
public:
    ForthEngine();
    virtual         ~ForthEngine();

    void            Initialize(ForthShell* pShell,
        int storageLongs = DEFAULT_USER_STORAGE,
        bool bAddBaseOps = true,
        ForthExtension* pExtension = NULL);
    void            Reset(void);
    void            ErrorReset(void);

    void            SetFastMode(bool goFast);
    void            ToggleFastMode(void);
    bool            GetFastMode(void);

    //
    // FullyExecuteOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
    // execute forth ops, and is also how systems external to forth execute ops
    //
    OpResult        FullyExecuteOp(ForthCoreState* pCore, forthop opCode);
    // ExecuteOp will start execution of an op, but will not finish user defs or methods
    OpResult        ExecuteOp(ForthCoreState* pCore, forthop opCode);
    // ExecuteOps executes a sequence of forth ops
    // The sequence must be terminated with an OP_DONE
    OpResult        ExecuteOps(ForthCoreState* pCore, forthop* pOps);

    OpResult		FullyExecuteMethod(ForthCoreState* pCore, ForthObject& obj, int methodNum);
    OpResult        DeleteObject(ForthCoreState* pCore, ForthObject& obj);
    void                ReleaseObject(ForthCoreState* pCore, ForthObject& inObject);

    inline forthop* GetDP() { return mDictionary.pCurrent; };
    inline void             SetDP(forthop* pNewDP) { mDictionary.pCurrent = pNewDP; };
    inline void             AllotLongs(int n) { mDictionary.pCurrent += n; };
    inline void             AllotBytes(int n) { mDictionary.pCurrent = reinterpret_cast<forthop*>(reinterpret_cast<cell>(mDictionary.pCurrent) + n); };
    inline void             AlignDP(void) {
        mDictionary.pCurrent = (forthop*)((((cell)mDictionary.pCurrent) + (sizeof(forthop) - 1))
            & ~(sizeof(forthop) - 1));
    };
    inline ForthMemorySection* GetDictionaryMemorySection() { return &mDictionary; };

    void                    ShowMemoryInfo();
    inline ForthShell* GetShell(void) { return mpShell; };
    inline void				SetShell(ForthShell* pShell) { mpShell = pShell; };
    inline ForthFiber* GetMainFiber(void) { return mpMainThread->GetFiber(0); };

    cell* GetCompileStatePtr(void);
    void            SetCompileState(cell v);
    cell            IsCompiling(void);

    void                    GetErrorString(char* pBuffer, int bufferSize);
    OpResult            CheckStacks(void);
    void                    SetError(ForthError e, const char* pString = NULL);
    void                    AddErrorText(const char* pString);
    void                    SetFatalError(ForthError e, const char* pString = NULL);
    inline ForthError      GetError(void) { return (ForthError)(mpCore->error); };
    // for some reason, inlining this makes it return bogus data in some cases - WTF?
    //inline ForthCoreState*  GetCoreState( void ) { return mpCore; };
    ForthCoreState* GetCoreState(void);

    static ForthEngine* GetInstance(void);

    void					SetDefaultConsoleOut(ForthObject& newOutStream);
    void					SetConsoleOut(ForthCoreState* pCore, ForthObject& newOutStream);
    void					SetErrorOut(ForthCoreState* pCore, ForthObject& newOutStream);
    void* GetErrorOut(ForthCoreState* pCore);
    void					PushConsoleOut(ForthCoreState* pCore);
    void					PushErrorOut(ForthCoreState* pCore);
    void					PushDefaultConsoleOut(ForthCoreState* pCore);
    void					ResetConsoleOut(ForthCoreState& core);
    void					ResetConsoleOut();

    // return milliseconds since engine was created
    uint32_t           GetElapsedTime(void);

    void                    ConsoleOut(const char* pBuffer);
    void                    TraceOut(const char* pFormat, ...);

    int32_t					GetTraceFlags(void);
    void					SetTraceFlags(int32_t flags);

    void					SetTraceOutRoutine(traceOutRoutine traceRoutine, void* pTraceData);
    void					GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const;

    const char* GetOpTypeName(int32_t opType);
    void                    TraceOp(forthop* pOp, forthop op);
    void                    TraceStack(ForthCoreState* pCore);
    void                    DescribeOp(forthop* pOp, char* pBuffer, int buffSize, bool lookupUserDefs = false);
    void                    AddOpExecutionToProfile(forthop op);
    void                    DumpExecutionProfile();
    void                    ResetExecutionProfile();

    void					DumpCrashState();
    void					DisplayUserDefCrash(forthop* pRVal, char* buff, int buffSize);

    void                    RaiseException(ForthCoreState* pCore, cell exceptionNum);

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    ForthThread* CreateThread(forthop fiberOp = OP_DONE, int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE);
    void            DestroyThread(ForthThread* pThread);

    void InitCoreState(ForthCoreState& core);

    ForthBlockFileManager* GetBlockFileManager();

    // returns true IFF file opened successfully
    bool            PushInputFile(const char* pInFileName);
    void            PushInputBuffer(const char* pDataBuffer, int dataBufferLen);
    void            PushInputBlocks(ForthBlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock);
    void            PopInputStream(void);

    bool					IsServer() const;
    void					SetIsServer(bool isServer);

    inline ForthExtension* GetExtension() { return mpExtension; }

    ForthClassVocabulary* AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex,
        eBuiltinClassIndex parentClassIndex, baseMethodEntry* pEntries);

    inline OuterInterpreter* GetOuterInterpreter() { return mpOuter; }

    void AddOpNameForTracing(const char* pName);

private:
    OuterInterpreter* mpOuter;
    ForthBlockFileManager* mBlockFileManager;

    ForthCoreState* mpCore;             // core inner interpreter state

    ForthMemorySection mDictionary;

    ForthThread* mpThreads;
    ForthThread* mpMainThread;
    ForthShell* mpShell;
    int32_t* mpEngineScratch;
    char* mpErrorString;  // optional error information from shell
    traceOutRoutine	mTraceOutRoutine;
    void* mpTraceOutData;
    ForthObject		mDefaultConsoleOutStream;
    ForthObject     mErrorOutStream;


    struct opcodeProfileInfo {
        forthop op;
        ucell count;
        ForthVocabulary* pVocabulary;
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

    ForthExtension* mpExtension;

    static ForthEngine* mpInstance;
    bool            mFastMode;
    bool			mIsServer;

};

