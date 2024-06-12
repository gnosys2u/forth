#pragma once
//////////////////////////////////////////////////////////////////////
//
// Thread.h: interface for the Thread and Fiber classes.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
class Thread;
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

class Fiber
{
public:
    Fiber(Engine *pEngine, Thread *pParentThread, int threadIndex, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
    virtual ~Fiber();
    void Destroy();

#ifdef CHECK_GAURD_AREAS
    bool CheckGaurdAreas();
#endif

	void				InitTables(Fiber* pSourceThread);

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
    void                Join(Fiber* pJoiningThread);

	inline void			SetIP( forthop* newIP ) { mCore.IP = newIP; };
	
	void				FreeObjects();

	ShowContext*	GetShowContext();

	inline FiberState GetRunState() { return mRunState; }
	void				SetRunState(FiberState newState);
	inline Thread* GetParent() { return mpParentThread; }
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
	Thread	*mpParentThread;
    CoreState      mCore;
    forthop             mOps[2];
    uint32_t				mWakeupTime;
	FiberState mRunState;
    Fiber*         mpJoinHead;
    Fiber*         mpNextJoiner;
    int                 mIndex;
    std::string         mName;
};

class Thread
{
public:
	Thread(Engine *pEngine, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	virtual ~Thread();

	void                Reset(void);
	cell                Start();
	void                Exit();
	Fiber*		    GetNextReadyFiber();
	Fiber*		    GetNextSleepingFiber();
	Fiber*		    GetFiber(int fiberIndex);
	Fiber*		    GetActiveFiber();
    void                SetActiveFiber(Fiber *pThread);

	void				FreeObjects();

	inline FiberState GetRunState() { return mRunState; }

    void                Join();

    void                InnerLoop();

	Fiber*		    CreateFiber(Engine *pEngine, forthop fiberOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void				DeleteFiber(Fiber* pFiber);

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
	std::vector<Fiber*> mFibers;
	Thread*   mpNext;
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
	void FixupThread(Thread* pThread);
	void CreateFiberObject(ForthObject& outThread, Thread *pParentThread, Engine *pEngine, forthop threadOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void FixupFiber(Fiber* pThread);
}

namespace OLock
{
	void AddClasses(OuterInterpreter* pOuter);

	void CreateAsyncLockObject(ForthObject& outAsyncLock, Engine *pEngine);
    void CreateAsyncSemaphoreObject(ForthObject& outSemaphore, Engine *pEngine);
}
