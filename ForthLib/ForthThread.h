#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthThread.h: interface for the ForthThread and ForthFiber classes.
//
//////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>

#include "Forth.h"
#include "ForthInner.h"
#if defined(LINUX) || defined(MACOSX)
#include <pthread.h>
#include <semaphore.h>
#endif

class Engine;
class ShowContext;
class ForthThread;
class OuterInterpreter;

#define DEFAULT_PSTACK_SIZE 128
#define DEFAULT_RSTACK_SIZE 128


// define CHECK_GAURD_AREAS if there is a bug where memory immediately above or below the stacks is getting trashed
//#define CHECK_GAURD_AREAS

#ifdef CHECK_GAURD_AREAS
#define CHECK_STACKS(THREAD_PTR)    (THREAD_PTR)->CheckGaurdAreas()
#else
#define CHECK_STACKS(THREAD_PTR)
#endif

class ForthFiber
{
public:
    ForthFiber(Engine *pEngine, ForthThread *pParentThread, int threadIndex, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
    virtual ~ForthFiber();
    void Destroy();

#ifdef CHECK_GAURD_AREAS
    bool CheckGaurdAreas();
#endif

	void				InitTables(ForthFiber* pSourceThread);

    void                Reset( void );
    void                ResetIP( void );

    inline void         SetOp( forthop op ) { mOps[0] = op; };

    inline void         Push(cell value ) { *--mCore.SP = value; };
    inline cell         Pop() { return *mCore.SP++; };
    inline void         RPush(cell value ) { *--mCore.RP = value; };
    inline cell         RPop() { return *mCore.RP++; };

	void				Run();
	void				Block();

	inline uint32_t        GetWakeupTime() { return mWakeupTime; };
	void				Sleep(uint32_t milliSeconds);
	void				Wake();
	void				Stop();
	void				Exit();
    void                Join(ForthFiber* pJoiningThread);

	inline void			SetIP( forthop* newIP ) { mCore.IP = newIP; };
	
	void				FreeObjects();

	ShowContext*	GetShowContext();

	inline FiberState GetRunState() { return mRunState; }
	void				SetRunState(FiberState newState);
	inline ForthThread* GetParent() { return mpParentThread; }
	inline Engine* GetEngine() { return mpEngine; }

    friend class Engine;

	inline CoreState* GetCore() { return &mCore; };

	inline ForthObject& GetFiberObject() { return mObject; }
	inline void SetFiberObject(ForthObject& inObject) { mObject = inObject; }

    inline int GetIndex() { return mIndex; }

    const char* GetName() const;
    void SetName(const char* newName);

protected:
    void    WakeAllJoiningFibers();

	ForthObject			mObject;
    Engine         *mpEngine;
    void                *mpPrivate;
	ShowContext	*mpShowContext;
	ForthThread	*mpParentThread;
    CoreState      mCore;
    forthop             mOps[2];
    uint32_t				mWakeupTime;
	FiberState mRunState;
    ForthFiber*         mpJoinHead;
    ForthFiber*         mpNextJoiner;
    int                 mIndex;
    std::string         mName;
};

class ForthThread
{
public:
	ForthThread(Engine *pEngine, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	virtual ~ForthThread();

	void                Reset(void);
	cell                Start();
	void                Exit();
	ForthFiber*		    GetNextReadyFiber();
	ForthFiber*		    GetNextSleepingFiber();
	ForthFiber*		    GetFiber(int fiberIndex);
	ForthFiber*		    GetActiveFiber();
    void                SetActiveFiber(ForthFiber *pThread);

	void				FreeObjects();

	inline FiberState GetRunState() { return mRunState; }

    void                Join();

    void                InnerLoop();

	ForthFiber*		    CreateFiber(Engine *pEngine, forthop fiberOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void				DeleteFiber(ForthFiber* pFiber);

	inline ForthObject& GetThreadObject() { return mObject; }
	inline void SetThreadObject(ForthObject& inObject) { mObject = inObject; }

    const char* GetName() const;
    void SetName(const char* newName);

#if defined(LINUX) || defined(MACOSX)
	static void* RunLoop(void *pThis);
#else
	static unsigned __stdcall RunLoop(void *pThis);
#endif

	friend class Engine;

protected:
	ForthObject			mObject;
	std::vector<ForthFiber*> mFibers;
	ForthThread*   mpNext;
	int					mActiveFiberIndex;
	FiberState mRunState;
    std::string         mName;
#if defined(LINUX) || defined(MACOSX)
	int                 mHandle;
	pthread_t           mThread;
	int					mExitStatus;
    pthread_mutex_t		mExitMutex;
    pthread_cond_t		mExitSignal;
#else
    HANDLE              mHandle;
	uint32_t               mThreadId;
    HANDLE              mExitSignal;
#endif
};

namespace OThread
{
	void AddClasses(OuterInterpreter* pOuter);

	void CreateThreadObject(ForthObject& outThread, Engine *pEngine, forthop threadOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void FixupThread(ForthThread* pThread);
	void CreateFiberObject(ForthObject& outThread, ForthThread *pParentThread, Engine *pEngine, forthop threadOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void FixupFiber(ForthFiber* pThread);
}

namespace OLock
{
	void AddClasses(OuterInterpreter* pOuter);

	void CreateAsyncLockObject(ForthObject& outAsyncLock, Engine *pEngine);
    void CreateAsyncSemaphoreObject(ForthObject& outSemaphore, Engine *pEngine);
}
