//////////////////////////////////////////////////////////////////////
//
// Thread.cpp: implementation of the Thread and Fiber classes.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include <deque>
#include "ForthThread.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#include "ShowContext.h"
#include "BuiltinClasses.h"
#include "ClassVocabulary.h"

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#ifdef CHECK_GAURD_AREAS
#define GAURD_AREA 64
#else
#define GAURD_AREA 4
#endif

struct oThreadStruct
{
    forthop*            pMethods;
	REFCOUNTER          refCount;
	cell                id;
	Thread			*pThread;
};

struct oFiberStruct
{
    forthop*            pMethods;
	REFCOUNTER          refCount;
    cell                id;
	Fiber			*pFiber;
};

//////////////////////////////////////////////////////////////////////
////
///
//                     CoreState struct
// 

CoreState::CoreState(int paramStackSize, int returnStackSize)
    : SLen(paramStackSize)
    , RLen(returnStackSize)
	, SP(nullptr)
	, RP(nullptr)
{
    // leave a few extra words above top of stacks, so that underflows don't
    //   tromp on the memory allocator info
    SB = new cell[SLen + (GAURD_AREA * 2)];
    SB += GAURD_AREA;
    ST = SB + SLen;

    RB = new cell[RLen + (GAURD_AREA * 2)];
    RB += GAURD_AREA;
    RT = RB + RLen;

#ifdef CHECK_GAURD_AREAS
    int32_t checkVal = 0x03020100;
    for (int i = 0; i < GAURD_AREA; i++)
    {
        SB[i - GAURD_AREA] = checkVal;
        RB[i - GAURD_AREA] = checkVal;
        ST[i] = checkVal;
        RT[i] = checkVal;
        checkVal += 0x04040404;
    }
#endif
    IP = nullptr;
    FP = nullptr;
    TP = nullptr;

    optypeAction = nullptr;
    numBuiltinOps = 0;
    ops = nullptr;
    numOps = 0;
    maxOps = 0;

    pEngine = nullptr;
    pFiber = nullptr;
    pDictionary = nullptr;
    pFileFuncs = nullptr;
    consoleOutStream = nullptr;
    pExceptionFrame = nullptr;

    innerLoop = nullptr;
    innerExecute = nullptr;

    varMode = VarOperation::kVarDefaultOp;
    state = OpResult::kDone;
    error = ForthError::kNone;

    base = DEFAULT_BASE;
    signedPrintMode = kPrintSignedDecimal;
    traceFlags = 0;

    for (int i = 0; i < NUM_CORE_SCRATCH_CELLS; i++)
    {
        scratch[i] = 0;
    }
}


void CoreState::InitializeFromEngine(void* engineIn)
{
    Engine* engine = (Engine *)engineIn;
    pEngine = engineIn;
    pDictionary = engine->GetDictionaryMemorySection();
    //    core.pFileFuncs = mpShell->GetFileInterface();

    CoreState* pEngineCore = engine->GetCoreState();
    if (pEngineCore != NULL)
    {
        // fill in optype & opcode action tables from engine thread
        optypeAction = pEngineCore->optypeAction;
        numBuiltinOps = pEngineCore->numBuiltinOps;
        numOps = pEngineCore->numOps;
        maxOps = pEngineCore->maxOps;
        ops = pEngineCore->ops;
        innerLoop = pEngineCore->innerLoop;
        innerExecute = pEngineCore->innerExecute;
        innerExecute = pEngineCore->innerExecute;
    }

    engine->ResetConsoleOut(*this);
}


//////////////////////////////////////////////////////////////////////
////
///
//                     Fiber
// 

Fiber::Fiber(Engine *pEngine, Thread *pParentThread, int fiberIndex, int paramStackLongs, int returnStackLongs)
: mpEngine( pEngine )
, mpParentThread(pParentThread)
, mIndex(fiberIndex)
, mWakeupTime(0)
, mpPrivate( NULL )
, mpShowContext(NULL)
, mpJoinHead(nullptr)
, mpNextJoiner(nullptr)
, mObject(nullptr)
, mCore(paramStackLongs, returnStackLongs)
{
    mCore.pFiber = this;

    mCore.InitializeFromEngine(pEngine);

    mOps[1] = gCompiledOps[OP_DONE];

    Reset();
}

Fiber::~Fiber()
{
    if (mpJoinHead != nullptr)
    {
        WakeAllJoiningFibers();
    }
    // TODO: warn if mpNextJoiner is not null

    mCore.SB -= GAURD_AREA;
    delete [] mCore.SB;
    mCore.RB -= GAURD_AREA;
    delete [] mCore.RB;

	if (mpShowContext != NULL)
	{
		delete mpShowContext;
	}

    if (mObject)
    {
        oFiberStruct* pFiberStruct = (oFiberStruct *)mObject;
		// WTF?
        if (pFiberStruct != nullptr)
        {
            mpEngine->DeleteObject(&mCore, mObject);
        }
    }
}

void Fiber::Destroy()
{
    if (mpParentThread != nullptr)
    {
        mpParentThread->DeleteFiber(this);
    }
    else
    {
        delete this;
    }
}

#ifdef CHECK_GAURD_AREAS
bool
Fiber::CheckGaurdAreas( void )
{
    int32_t checkVal = 0x03020100;
    bool retVal = false;
    for ( int i = 0; i < 64; i++ )
    {
        if ( mCore.SB[i - GAURD_AREA] != checkVal )
        {
            return true;
        }
        if ( mCore.RB[i - GAURD_AREA] != checkVal )
        {
            return true;
        }
        if ( mCore.ST[i] != checkVal )
        {
            return true;
        }
        if ( mCore.RT[i] != checkVal )
        {
            return true;
        }
        checkVal += 0x04040404;
    }
    return false;
}
#endif

void Fiber::InitTables(Fiber* pSourceThread)
{
	CoreState& sourceCore = pSourceThread->mCore;
	mCore.optypeAction = sourceCore.optypeAction;
	mCore.numBuiltinOps = sourceCore.numBuiltinOps;
	mCore.numOps = sourceCore.numOps;
	mCore.maxOps = sourceCore.maxOps;
	mCore.ops = sourceCore.ops;
	mCore.innerLoop = sourceCore.innerLoop;
    mCore.innerExecute = sourceCore.innerExecute;
}

void
Fiber::Reset( void )
{
    mCore.SP = mCore.ST;
    mCore.RP = mCore.RT;
    mCore.FP = nullptr;
    mCore.TP = nullptr;

    mCore.error = ForthError::kNone;
    mCore.state = OpResult::kDone;
    mCore.varMode = VarOperation::kVarDefaultOp;
    mCore.base = 10;
    mCore.signedPrintMode = kPrintSignedDecimal;
	mCore.IP = &(mOps[0]);
    mCore.traceFlags = 0;
    mCore.pExceptionFrame = nullptr;
	//mCore.IP = nullptr;

	if (mpShowContext != nullptr)
	{
		mpShowContext->Reset();
	}

}

void
Fiber::ResetIP( void )
{
	mCore.IP = &(mOps[0]);
	//mCore.IP = nullptr;
}

void Fiber::Run()
{
    OpResult exitStatus;

	Wake();
	mCore.IP = &(mOps[0]);
	//mCore.IP = nullptr;
	// the user defined ops could have changed since this thread was created, update it to match the engine
	CoreState* pEngineState = mpEngine->GetCoreState();
	mCore.ops = pEngineState->ops;
	mCore.numOps = pEngineState->numOps;
#ifdef ASM_INNER_INTERPRETER
    if ( mpEngine->GetFastMode() )
    {
		do
		{
			exitStatus = InnerInterpreterFast(&mCore);
		} while (exitStatus == OpResult::kYield);
    }
    else
#endif
    {
		do
		{
			exitStatus = InnerInterpreter(&mCore);
		} while (exitStatus == OpResult::kYield);
    }
}

void Fiber::Join(Fiber* pJoiningFiber)
{
    //printf("Join: thread %x is waiting for %x to exit\n", pJoiningThread, this);
    if (mRunState != FiberState::kExited)
    {
        ForthObject& joiner = pJoiningFiber->GetFiberObject();
        SAFE_KEEP(joiner);
        pJoiningFiber->Block();
        pJoiningFiber->mpNextJoiner = mpJoinHead;
        mpJoinHead = pJoiningFiber;
    }
}

void Fiber::WakeAllJoiningFibers()
{
    Fiber* pFiber = mpJoinHead;
    //printf("WakeAllJoiningFibers: thread %x is exiting\n", this);
    while (pFiber != nullptr)
    {
        Fiber* pNextFiber = pFiber->mpNextJoiner;
        //printf("WakeAllJoiningFibers: waking thread %x\n", pFiber);
        pFiber->mpNextJoiner = nullptr;
        pFiber->Wake();
        ForthObject& joiner = pFiber->GetFiberObject();
        //SAFE_RELEASE(&mCore, joiner);
		if (joiner != nullptr)
		{
#if defined(ATOMIC_REFCOUNTS)
			if (joiner->refCount.fetch_sub(1) == 1)
			{
				((Engine*)(mCore.pEngine))->DeleteObject(&mCore, joiner);
			}
#else
			joiner->refCount -= 1;
			if (joiner->refCount == 0)
			{
				((Engine*)(mCore.pEngine))->DeleteObject(&mCore, joiner);
			}
#endif
		}
		TRACK_RELEASE;
        pFiber = pNextFiber;
    }
    mpJoinHead = nullptr;
}

ShowContext* Fiber::GetShowContext()
{
	if (mpShowContext == NULL)
	{
		mpShowContext = new ShowContext;
	}
	return mpShowContext;
}

void Fiber::SetRunState(FiberState newState)
{
	// TODO!
	mRunState = newState;
}

void Fiber::Sleep(uint32_t sleepMilliSeconds)
{
	uint32_t now = mpEngine->GetElapsedTime();
#ifdef WIN32
	mWakeupTime = (sleepMilliSeconds == MAXINT32) ? MAXINT32 : now + sleepMilliSeconds;
#else
	mWakeupTime = (sleepMilliSeconds == INT32_MAX) ? INT32_MAX : now + sleepMilliSeconds;
#endif
	mRunState = FiberState::kSleeping;
}

void Fiber::Block()
{
	mRunState = FiberState::kBlocked;
}

void Fiber::Wake()
{
	mRunState = FiberState::kReady;
	mWakeupTime = 0;
}

void Fiber::Stop()
{
	mRunState = FiberState::kStopped;
}

void Fiber::Exit()
{
	mRunState = FiberState::kExited;
    WakeAllJoiningFibers();
}

const char* Fiber::GetName() const
{
    return mName.c_str();
}

void Fiber::SetName(const char* newName)
{
    mName.assign(newName);
}

void Fiber::FreeObjects()
{
	if (mObject != nullptr)
	{
		FREE_OBJECT(mObject);
	}
	mObject = nullptr;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     Thread
// 

Thread::Thread(Engine *pEngine, int paramStackLongs, int returnStackLongs)
	: mHandle(0)
#ifdef WIN32
	, mThreadId(0)
#endif
	, mpNext(NULL)
	, mActiveFiberIndex(0)
	, mRunState(FiberState::kStopped)
    , mObject(nullptr)
{
	Fiber* pPrimaryFiber = new Fiber(pEngine, this, 0, paramStackLongs, returnStackLongs);
    pPrimaryFiber->SetRunState(FiberState::kReady);
	mFibers.push_back(pPrimaryFiber);
#ifdef WIN32
    // default security attributes, manual reset event, initially nonsignaled, unnamed event
    mExitSignal = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (mExitSignal == NULL)
    {
        printf("Thread constructor - CreateEvent error: %d\n", GetLastError());
    }
#else
    pthread_mutex_init(&mExitMutex, nullptr);
    pthread_cond_init(&mExitSignal, nullptr);
#endif
}

Thread::~Thread()
{
#ifdef WIN32
	if (mHandle != 0)
	{
		CloseHandle(mHandle);
	}
    CloseHandle(mExitSignal);
#else
    pthread_mutex_destroy(&mExitMutex);
    pthread_cond_destroy(&mExitSignal);
#endif

	FreeObjects();

    for (Fiber* pFiber : mFibers)
	{
		if (pFiber != nullptr)
		{
			delete pFiber;
		}
	}
}

void Thread::FreeObjects()
{
    for (Fiber* pFiber : mFibers)
	{
		if (pFiber != nullptr)
		{
			pFiber->FreeObjects();
		}
	}
	mFibers.clear();

	oThreadStruct* pThreadStruct = (oThreadStruct*)mObject;
	if (pThreadStruct != nullptr && pThreadStruct->pThread != nullptr)
	{
		FREE_OBJECT(pThreadStruct);
	}
	mObject = nullptr;
}

#ifdef WIN32
unsigned __stdcall Thread::RunLoop(void *pUserData)
#else
void* Thread::RunLoop(void *pUserData)
#endif
{
    Thread* pThisThread = (Thread*)pUserData;
	Fiber* pActiveFiber = pThisThread->GetActiveFiber();
	Engine* pEngine = pActiveFiber->GetEngine();
    //printf("Starting thread %x\n", pThisThread);

    pThisThread->mRunState = FiberState::kReady;
	OpResult exitStatus = OpResult::kOk;
	bool keepRunning = true;
	while (keepRunning)
	{
		bool checkForAllDone = false;
		CoreState* pCore = pActiveFiber->GetCore();
#ifdef ASM_INNER_INTERPRETER
		if (pEngine->GetFastMode())
		{
			exitStatus = InnerInterpreterFast(pCore);
		}
		else
#endif
		{
			exitStatus = InnerInterpreter(pCore);
		}

		bool switchActiveFiber = false;
		switch (exitStatus)
		{
		case OpResult::kYield:
			switchActiveFiber = true;
			SET_STATE(OpResult::kOk);
			break;

		case OpResult::kDone:
			switchActiveFiber = true;
			checkForAllDone = true;
			break;

		default:
			break;
		}

		if (switchActiveFiber)
		{
			// TODO!
			// - switch to next runnable fiber
			// - sleep if all fibers are sleeping
			// - deal with all fibers stopped
			uint32_t now = pEngine->GetElapsedTime();

			Fiber* pNextFiber = pThisThread->GetNextReadyFiber();
			if (pNextFiber != nullptr)
			{
				pActiveFiber = pNextFiber;
			}
			else
			{
				pNextFiber = pThisThread->GetNextSleepingFiber();
				// pNextFiber is fiber in this thread which has earliest wakeup time
				if (pNextFiber != nullptr)
				{
					uint32_t wakeupTime = pNextFiber->GetWakeupTime();
					if (now >= wakeupTime)
					{
						pNextFiber->Wake();
						pActiveFiber = pNextFiber;
					}
					else
					{
						// TODO: don't always sleep until wakeupTime, since fibers which are blocked or stopped
						//  could be unblocked or started by other threads
						int sleepMilliseconds = wakeupTime - now;
#ifdef WIN32
						::Sleep((DWORD)sleepMilliseconds);
#else
						usleep(sleepMilliseconds * 1000);
#endif
						pActiveFiber = pNextFiber;
					}
				}
				else
				{
					// there are no ready or sleeping fibers, should we exit this thread?
					checkForAllDone = true;
				}
			}
		}
		if (checkForAllDone)
		{
			keepRunning = false;
			for (Fiber* pFiber : pThisThread->mFibers)
			{
				if (pFiber->GetRunState() != FiberState::kExited)
				{
					keepRunning = true;
					break;
				}
			}
		}
	}

	// NOTE: this seems redundant with Thread::Exit, but if you include the _endthreadex
	//  here you will get a crash.  So for now, threads must explicitly do thisThread.exit, not
	// just return from their thread op, which causes a different crash when another thread tries
	// to do a join on this thread

	// so I guess this code should never be executed - there is no returning from RunLoop.

	//printf("Exiting thread %x signaling exitEvent %d\n", pThisThread, pThisThread->mExitSignal);
#ifdef WIN32
	pThisThread->mRunState = FiberState::kExited;
	if (!SetEvent(pThisThread->mExitSignal))
	{
		printf("Thread::RunLoop SetEvent failed (%d)\n", GetLastError());
	}

	return 0;
#else
	pthread_mutex_lock(&pThisThread->mExitMutex);
	pThisThread->mRunState = FiberState::kExited;
	pthread_cond_broadcast(&pThisThread->mExitSignal);
	pthread_mutex_unlock(&pThisThread->mExitMutex);

	return nullptr;
#endif
}

void Thread::InnerLoop()
{
    Fiber* pMainFiber = mFibers[0];
    Fiber* pActiveFiber = pMainFiber;
    Engine* pEngine = pActiveFiber->GetEngine();
    pMainFiber->SetRunState(FiberState::kReady);

    OpResult exitStatus = OpResult::kOk;
    bool keepRunning = true;
    while (keepRunning)
    {
        CoreState* pCore = pActiveFiber->GetCore();
#ifdef ASM_INNER_INTERPRETER
        if (pEngine->GetFastMode())
        {
            exitStatus = InnerInterpreterFast(pCore);
        }
        else
#endif
        {
            exitStatus = InnerInterpreter(pCore);
        }

        bool switchActiveFiber = false;
        if ((exitStatus == OpResult::kYield) || (pActiveFiber->GetRunState() == FiberState::kExited))
        {
            switchActiveFiber = true;
            pCore->state = OpResult::kOk;
            /*
            static char* runStateNames[] = {
                "Stopped",		// initial state, or after executing stop, needs another thread to Start it
                "Ready",			// ready to continue running
                "Sleeping",		// sleeping until wakeup time is reached
                "Blocked",		// blocked on a soft lock
                "Exited"		// done running - executed exitFiber
            };
            for (int i = 0; i < mFibers.size(); ++i)
            {
                Fiber* pFiber = mFibers[i];
                printf("Fiber %d 0x%x   runState %s   coreState %d\n", i, (int)pFiber,
                    runStateNames[pFiber->GetRunState()], pFiber->GetCore()->state);
            }
            */
        }

        if (switchActiveFiber)
        {
            // TODO!
            // - switch to next runnable fiber
            // - sleep if all fibers are sleeping
            // - deal with all fibers stopped
            uint32_t now = pEngine->GetElapsedTime();

            Fiber* pNextFiber = GetNextReadyFiber();
            if (pNextFiber != nullptr)
            {
                //printf("Switching from fiber 0x%x to ready fiber 0x%x\n", pActiveFiber, pNextFiber);
                pActiveFiber = pNextFiber;
                SetActiveFiber(pActiveFiber);
            }
            else
            {
                pNextFiber = GetNextSleepingFiber();
				// pNextFiber is fiber in this thread which has earliest wakeup time
				if (pNextFiber != nullptr)
                {
                    uint32_t wakeupTime = pNextFiber->GetWakeupTime();
                    if (now >= wakeupTime)
                    {
                        //printf("Switching from fiber 0x%x to fiber 0x%x\n", pActiveFiber, pNextFiber);
                        pNextFiber->Wake();
                        pActiveFiber = pNextFiber;
                        SetActiveFiber(pActiveFiber);
                    }
                    else
                    {
                        // TODO: don't always sleep until wakeupTime, since fibers which are blocked or stopped
                        //  could be unblocked or started by other threads
                        int sleepMilliseconds = wakeupTime - now;
#ifdef WIN32
                        ::Sleep((DWORD)sleepMilliseconds);
#else
                        usleep(sleepMilliseconds * 1000);
#endif
                        pActiveFiber = pNextFiber;
                        SetActiveFiber(pActiveFiber);
                    }
                }
            }
        }  // end if switchActiveFiber

        OpResult mainFiberState = (OpResult) pMainFiber->GetCore()->state;
        keepRunning = false;
        if ((exitStatus != OpResult::kDone) && (pMainFiber->GetRunState() != FiberState::kExited))
        {
            switch ((OpResult)pMainFiber->GetCore()->state)
            {
            case OpResult::kOk:
            case OpResult::kYield:
                keepRunning = true;
                break;
            default:
                break;
            }
        }
    }
}

Fiber* Thread::GetFiber(int threadIndex)
{
	if (threadIndex < (int)mFibers.size())
	{
		return mFibers[threadIndex];
	}
	return nullptr;
}

Fiber* Thread::GetActiveFiber()
{
	if (mActiveFiberIndex < (int)mFibers.size())
	{
		return mFibers[mActiveFiberIndex];
	}
	return nullptr;
}


void Thread::SetActiveFiber(Fiber *pFiber)
{
    // TODO: verify that active fiber is actually a child of this thread
    int fiberIndex = pFiber->GetIndex();
    // ASSERT(fiberIndex < (int)mFibers.size() && mFibers[fiberIndex] == pFiber);
    mActiveFiberIndex = fiberIndex;
}

void Thread::Reset(void)
{ 
	for (Fiber* pFiber : mFibers)
	{
		if (pFiber != nullptr)
		{
			pFiber->Reset();
		}
	}
}

Fiber* Thread::GetNextReadyFiber()
{
	int originalFiberIndex = mActiveFiberIndex;

	do	
	{
		mActiveFiberIndex++;
		if (mActiveFiberIndex >= (int)mFibers.size())
		{
			mActiveFiberIndex = 0;
		}
		Fiber* pNextFiber = mFibers[mActiveFiberIndex];
		if (pNextFiber->GetRunState() == FiberState::kReady)
		{
			return pNextFiber;
		}
	} while (originalFiberIndex != mActiveFiberIndex);

	return nullptr;
}

Fiber* Thread::GetNextSleepingFiber()
{
	Fiber* pFiberToWake = nullptr;
	int originalFiberIndex = mActiveFiberIndex;
	uint32_t minWakeupTime = (uint32_t)(~0);
	do
	{
        mActiveFiberIndex++;
        if (mActiveFiberIndex >= (int)mFibers.size())
		{
			mActiveFiberIndex = 0;
		}
		Fiber* pNextFiber = mFibers[mActiveFiberIndex];
		if (pNextFiber->GetRunState() == FiberState::kSleeping)
		{
			uint32_t wakeupTime = pNextFiber->GetWakeupTime();
			if (wakeupTime < minWakeupTime)
			{
				minWakeupTime = wakeupTime;
				pFiberToWake = pNextFiber;
			}
		}
	} while (originalFiberIndex != mActiveFiberIndex);
	return pFiberToWake;
}

cell Thread::Start()
{
#ifdef WIN32
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		::CloseHandle(mHandle);
	}
	mHandle = (HANDLE)_beginthreadex(NULL, 0, Thread::RunLoop, this, 0, (unsigned *)&mThreadId);
#else
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		// TODO
		//::CloseHandle( mHandle );
	}
	mHandle = pthread_create(&mThread, NULL, Thread::RunLoop, this);

#endif
	return (cell)mHandle;
}

void Thread::Exit()
{
	// TBD: make sure this isn't the main thread
	if (mpNext != NULL)
	{
#ifdef WIN32
		mRunState = FiberState::kExited;
        // signal all threads waiting for us to exit
        if (!SetEvent(mExitSignal))
        {
            printf("Thread::Exit SetEvent error: %d\n", GetLastError());
        }
        _endthreadex(0);
#else
        pthread_mutex_lock(&mExitMutex);
		mRunState = FiberState::kExited;
    	pthread_cond_broadcast(&mExitSignal);
        pthread_mutex_unlock(&mExitMutex);
        pthread_exit(&mExitStatus);
#endif
    }
}

void Thread::Join()
{
#ifdef WIN32
    DWORD waitResult = WaitForSingleObject(mExitSignal, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        printf("Thread::Join WaitForSingleObject failed (%d)\n", GetLastError());
    }
#else
    pthread_mutex_lock(&mExitMutex);
	while (mRunState != FiberState::kExited)
	{
		pthread_cond_wait(&mExitSignal, &mExitMutex);
	}
    pthread_mutex_unlock(&mExitMutex);
#endif
}

Fiber* Thread::CreateFiber(Engine *pEngine, forthop threadOp, int paramStackLongs, int returnStackLongs)
{
	Fiber* pFiber = new Fiber(pEngine, this, (int)mFibers.size(), paramStackLongs, returnStackLongs);
	pFiber->SetOp(threadOp);
	pFiber->SetRunState(FiberState::kStopped);
	Fiber* pPrimaryFiber = GetFiber(0);
	pFiber->InitTables(pPrimaryFiber);
	mFibers.push_back(pFiber);

	return pFiber;
}

void Thread::DeleteFiber(Fiber* pInFiber)
{
	// TODO: do something when erasing last thread?  what about thread 0?
	size_t lastIndex = mFibers.size() - 1;
	for (size_t i = 0; i <= lastIndex; ++i)
	{
		Fiber* pFiber = mFibers[i];
		if (pFiber == pInFiber)
		{
			delete pFiber;
			mFibers.erase(mFibers.begin() + i);
            break;
        }
	}

	if (mActiveFiberIndex == lastIndex)
	{
		mActiveFiberIndex = 0;
	}
}


const char* Thread::GetName() const
{
    return mName.c_str();
}

void Thread::SetName(const char* newName)
{
    mName.assign(newName);
}


namespace OThread
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 oThread
	//

	static ClassVocabulary* gpThreadVocabulary;
	static ClassVocabulary* gpFiberVocabulary;

	void CreateThreadObject(ForthObject& outThread, Engine *pEngine, forthop threadOp, int paramStackLongs, int returnStackLongs)
	{
		ALLOCATE_OBJECT(oThreadStruct, pThreadStruct, gpThreadVocabulary);
        pThreadStruct->pMethods = gpThreadVocabulary->GetMethods();
        pThreadStruct->refCount = 1;
		Thread* pThread = pEngine->CreateThread(threadOp, paramStackLongs, returnStackLongs);
		pThreadStruct->pThread = pThread;
		pThread->Reset();
		outThread = (ForthObject) pThreadStruct;
		pThread->SetThreadObject(outThread);
		OThread::FixupFiber(pThread->GetFiber(0));
	}

	FORTHOP(oThreadNew)
	{
		GET_ENGINE->SetError(ForthError::kIllegalOperation, " cannot explicitly create a Thread object");
	}

	FORTHOP(oThreadDeleteMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		if (pThreadStruct->pThread != NULL)
		{
			GET_ENGINE->DestroyThread(pThreadStruct->pThread);
		}
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		Thread* pThread = pThreadStruct->pThread;
		Fiber* pFiber = pThread->GetFiber(0);
		pFiber->Reset();
		int32_t result = pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartWithArgsMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		Thread* pThread = pThreadStruct->pThread;
		Fiber* pFiber = pThread->GetFiber(0);
		pFiber->Reset();
		CoreState* pDstCore = pFiber->GetCore();
		cell numArgs = SPOP;
		if (numArgs > 0)
		{
			pDstCore->SP -= numArgs;
#if defined(FORTH64)
            memcpy(pDstCore->SP, pCore->SP, numArgs << 3);
#else
            memcpy(pDstCore->SP, pCore->SP, numArgs << 2);
#endif
			pCore->SP += numArgs;
		}
		int32_t result = pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadCreateFiberMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		Engine* pEngine = GET_ENGINE;
		ForthObject fiber;

		Thread* pThread = pThreadStruct->pThread;
		int returnStackLongs = (int)(SPOP);
		int paramStackLongs = (int)(SPOP);
		int32_t threadOp = SPOP;
		OThread::CreateFiberObject(fiber, pThread, pEngine, threadOp, paramStackLongs, returnStackLongs);

		PUSH_OBJECT(fiber);
		METHOD_RETURN;
	}

	FORTHOP(oThreadExitMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		Thread* pThread = pThreadStruct->pThread;
		pThread->Exit();
		METHOD_RETURN;
	}

    FORTHOP(oThreadJoinMethod)
    {
        GET_THIS(oThreadStruct, pThreadStruct);
        Thread* pThread = pThreadStruct->pThread;
        pThread->Join();
        METHOD_RETURN;
    }

    FORTHOP(oThreadResetMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oThreadGetRunStateMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		SPUSH((int32_t)(pThreadStruct->pThread->GetRunState()));
		METHOD_RETURN;
	}

    FORTHOP(oThreadGetNameMethod)
    {
        GET_THIS(oThreadStruct, pThreadStruct);
        SPUSH((cell)(pThreadStruct->pThread->GetName()));
        METHOD_RETURN;
    }

    FORTHOP(oThreadSetNameMethod)
    {
        GET_THIS(oThreadStruct, pThreadStruct);
        const char* name = (const char*)(SPOP);
        pThreadStruct->pThread->SetName(name);
        METHOD_RETURN;
    }

	baseMethodEntry oThreadMembers[] =
	{
		METHOD("__newOp", oThreadNew),
		METHOD("delete", oThreadDeleteMethod),
		METHOD_RET("start", oThreadStartMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD_RET("startWithArgs", oThreadStartWithArgsMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD_RET("createFiber", oThreadCreateFiberMethod, RETURNS_OBJECT(kBCIFiber)),
		METHOD("exit", oThreadExitMethod),
        METHOD("join", oThreadJoinMethod),
        METHOD("reset", oThreadResetMethod),
		METHOD_RET("getRunState", oThreadGetRunStateMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("getName", oThreadGetNameMethod, RETURNS_NATIVE_PTR(BaseType::kByte)),
        METHOD("setName", oThreadSetNameMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),

		// following must be last in table
		END_MEMBERS
	};
	

	//////////////////////////////////////////////////////////////////////
	///
	//                 oFiber
	//

	void CreateFiberObject(ForthObject& outFiber, Thread *pParentThread, Engine *pEngine, forthop threadOp, int paramStackLongs, int returnStackLongs)
	{
		ALLOCATE_OBJECT(oFiberStruct, pFiberStruct, gpFiberVocabulary);
        pFiberStruct->pMethods = gpFiberVocabulary->GetMethods();
        pFiberStruct->refCount = 1;
		pFiberStruct->pFiber = pParentThread->CreateFiber(pEngine, threadOp, paramStackLongs, returnStackLongs);
		pFiberStruct->pFiber->Reset();

        outFiber = (ForthObject)pFiberStruct;
        pFiberStruct->pFiber->SetFiberObject(outFiber);
	}

	void FixupFiber(Fiber* pFiber)
	{
		ALLOCATE_OBJECT(oFiberStruct, pFiberStruct, gpFiberVocabulary);
		ForthObject& primaryFiber = pFiber->GetFiberObject();
        pFiberStruct->pMethods = gpFiberVocabulary->GetMethods();
        pFiberStruct->refCount = 1;
		pFiberStruct->pFiber = pFiber;
		primaryFiber = (ForthObject)pFiberStruct;
	}

	void FixupThread(Thread* pThread)
	{
		ALLOCATE_OBJECT(oThreadStruct, pThreadStruct, gpThreadVocabulary);
        pThreadStruct->pMethods = gpThreadVocabulary->GetMethods();
        pThreadStruct->refCount = 1;
		pThreadStruct->pThread = pThread;
		ForthObject& primaryThread = pThread->GetThreadObject();
        primaryThread = (ForthObject)pThreadStruct;

        OThread::FixupFiber(pThread->GetFiber(0));
	}

	FORTHOP(oFiberNew)
	{
		GET_ENGINE->SetError(ForthError::kIllegalOperation, " cannot explicitly create a Fiber object");
	}

	FORTHOP(oFiberDeleteMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		if (pFiberStruct->pFiber != NULL)
		{
            pFiberStruct->pFiber->Destroy();
        }
		METHOD_RETURN;
	}

	FORTHOP(oFiberStartMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		pFiberStruct->pFiber->SetRunState(FiberState::kReady);
		METHOD_RETURN;
	}

	FORTHOP(oFiberStartWithArgsMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		Fiber* pFiber = pFiberStruct->pFiber;
		CoreState* pDstCore = pFiber->GetCore();
		int numArgs = SPOP;
		if (numArgs > 0)
		{
			pDstCore->SP -= numArgs;
#if defined(FORTH64)
            memcpy(pDstCore->SP, pCore->SP, numArgs << 3);
#else
            memcpy(pDstCore->SP, pCore->SP, numArgs << 2);
#endif
			pCore->SP += numArgs;
		}
		pFiber->SetRunState(FiberState::kReady);
        SPUSH(-1);
		METHOD_RETURN;
	}

	FORTHOP(oFiberStopMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		SET_STATE(OpResult::kYield);
		pFiberStruct->pFiber->Stop();
		METHOD_RETURN;
	}

	FORTHOP(oFiberExitMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		SET_STATE(OpResult::kYield);
		pFiberStruct->pFiber->Exit();
		METHOD_RETURN;
	}

	FORTHOP(oFiberJoinMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        Fiber* pJoiner = pFiberStruct->pFiber->GetParent()->GetActiveFiber();
        pFiberStruct->pFiber->Join(pJoiner);
        SET_STATE(OpResult::kYield);
        METHOD_RETURN;
    }

    FORTHOP(oFiberSleepMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
        SET_STATE(OpResult::kYield);
        uint32_t sleepMilliseconds = SPOP;
		pFiberStruct->pFiber->Sleep(sleepMilliseconds);
		METHOD_RETURN;
	}

    FORTHOP(oFiberWakeMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        SET_STATE(OpResult::kYield);
        pFiberStruct->pFiber->Wake();
        METHOD_RETURN;
    }

    FORTHOP(oFiberPushMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		pFiberStruct->pFiber->Push(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oFiberPopMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		int32_t val = pFiberStruct->pFiber->Pop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oFiberRPushMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		pFiberStruct->pFiber->RPush(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oFiberRPopMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		int32_t val = pFiberStruct->pFiber->RPop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oFiberGetRunStateMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		SPUSH((int32_t)(pFiberStruct->pFiber->GetRunState()));
		METHOD_RETURN;
	}

	FORTHOP(oFiberStepMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		Fiber* pFiber = pFiberStruct->pFiber;
		CoreState* pFiberCore = pFiber->GetCore();
		forthop op = *(pFiberCore->IP)++;
		OpResult result;
#ifdef ASM_INNER_INTERPRETER
        Engine *pEngine = GET_ENGINE;
		if (pEngine->GetFastMode())
		{
			result = InterpretOneOpFast(pFiberCore, op);
		}
		else
#endif
		{
			result = InterpretOneOp(pFiberCore, op);
		}
		if (result == OpResult::kDone)
		{
			pFiber->ResetIP();
		}
		SPUSH((cell)result);
		METHOD_RETURN;
	}

	FORTHOP(oFiberResetMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		pFiberStruct->pFiber->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oFiberResetIPMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		pFiberStruct->pFiber->ResetIP();
		METHOD_RETURN;
	}

    FORTHOP(oFiberGetCoreMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        SPUSH((cell)(pFiberStruct->pFiber->GetCore()));
        METHOD_RETURN;
    }

    FORTHOP(oFiberGetNameMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        SPUSH((cell)(pFiberStruct->pFiber->GetName()));
        METHOD_RETURN;
    }

    FORTHOP(oFiberSetNameMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        const char* name = (const char*)(SPOP);
        pFiberStruct->pFiber->SetName(name);
        METHOD_RETURN;
    }

    baseMethodEntry oFiberMembers[] =
	{
		METHOD("__newOp", oFiberNew),
		METHOD("delete", oFiberDeleteMethod),
		METHOD("start", oFiberStartMethod),
        METHOD_RET("startWithArgs", oFiberStartWithArgsMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD("stop", oFiberStopMethod),
		METHOD("exit", oFiberExitMethod),
		METHOD("join", oFiberJoinMethod),
        METHOD("sleep", oFiberSleepMethod),
        METHOD("wake", oFiberWakeMethod),
        METHOD("push", oFiberPushMethod),
		METHOD_RET("pop", oFiberPopMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD("rpush", oFiberRPushMethod),
		METHOD_RET("rpop", oFiberRPopMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD_RET("getRunState", oFiberGetRunStateMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD_RET("step", oFiberStepMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD("reset", oFiberResetMethod),
		METHOD("resetIP", oFiberResetIPMethod),
        METHOD("getCore", oFiberGetCoreMethod),
        METHOD_RET("getName", oFiberGetNameMethod, RETURNS_NATIVE_PTR(BaseType::kByte)),
        METHOD("setName", oFiberSetNameMethod),
        //METHOD_RET("getParent", oFiberGetParentMethod, RETURNS_NATIVE(BaseType::k)),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
		MEMBER_VAR("__fiber", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(OuterInterpreter* pOuter)
	{
		gpFiberVocabulary = pOuter->AddBuiltinClass("Fiber", kBCIFiber, kBCIObject, oFiberMembers);
		gpThreadVocabulary = pOuter->AddBuiltinClass("Thread", kBCIThread, kBCIObject, oThreadMembers);
	}

} // namespace OThread

namespace OLock
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 OAsyncLock
	//

	struct oAsyncLockStruct
	{
        forthop*        pMethods;
		REFCOUNTER      refCount;
		cell            id;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
        pthread_mutex_t* pLock;
#endif
	};

	static ClassVocabulary* gpAsyncLockVocabulary;

	void CreateAsyncLockObject(ForthObject& outAsyncLock, Engine *pEngine)
	{
		ALLOCATE_OBJECT(oAsyncLockStruct, pLockStruct, gpAsyncLockVocabulary);
        pLockStruct->pMethods = gpAsyncLockVocabulary->GetMethods();
		pLockStruct->refCount = 0;
#ifdef WIN32
        pLockStruct->pLock = new CRITICAL_SECTION();
        InitializeCriticalSection(pLockStruct->pLock);
#else
		pLockStruct->pLock = new pthread_mutex_t;
		pthread_mutexattr_t mutexAttr;
		pthread_mutexattr_init(&mutexAttr);
		pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

		pthread_mutex_init(pLockStruct->pLock, &mutexAttr);

		pthread_mutexattr_destroy(&mutexAttr);
#endif
		outAsyncLock = (ForthObject)pLockStruct;
	}

	FORTHOP(oAsyncLockNew)
	{
		GET_ENGINE->SetError(ForthError::kIllegalOperation, " cannot explicitly create an AsyncLock object");
	}

	FORTHOP(oAsyncLockDeleteMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
		if (pLockStruct->pLock != NULL)
		{
#ifdef WIN32
			DeleteCriticalSection(pLockStruct->pLock);
#else
			pthread_mutex_destroy(pLockStruct->pLock);
			delete pLockStruct->pLock;
#endif
			pLockStruct->pLock = NULL;
		}
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockGrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockTryGrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
#ifdef WIN32
        BOOL result = TryEnterCriticalSection(pLockStruct->pLock);
#else
		int lockResult = pthread_mutex_trylock(pLockStruct->pLock);
		bool result = (lockResult == 0);
#endif
		SPUSH((cell)result);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockUngrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	baseMethodEntry oAsyncLockMembers[] =
	{
		METHOD("__newOp", oAsyncLockNew),
		METHOD("delete", oAsyncLockDeleteMethod),
		METHOD("grab", oAsyncLockGrabMethod),
		METHOD_RET("tryGrab", oAsyncLockTryGrabMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD("ungrab", oAsyncLockUngrabMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
		MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 OLock
	//

	struct oLockStruct
	{
        forthop*        pMethods;
		REFCOUNTER      refCount;
		cell            id;
		cell            lockDepth;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
		pthread_mutex_t* pLock;
#endif
		Fiber* pLockHolder;
		std::deque<Fiber*> *pBlockedFibers;
	};

	FORTHOP(oLockNew)
	{
		ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
		ALLOCATE_OBJECT(oLockStruct, pLockStruct, pClassVocab);

        pLockStruct->pMethods = pClassVocab->GetMethods();
        pLockStruct->refCount = 0;
#ifdef WIN32
        pLockStruct->pLock = new CRITICAL_SECTION();
        InitializeCriticalSection(pLockStruct->pLock);
#else
		pLockStruct->pLock = new pthread_mutex_t;
		pthread_mutexattr_t mutexAttr;
		pthread_mutexattr_init(&mutexAttr);
		pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

		pthread_mutex_init(pLockStruct->pLock, &mutexAttr);

		pthread_mutexattr_destroy(&mutexAttr);
#endif
		pLockStruct->pLockHolder = nullptr;
		pLockStruct->pBlockedFibers = new std::deque<Fiber*>;
		pLockStruct->lockDepth = 0;

		PUSH_OBJECT(pLockStruct);
	}

	FORTHOP(oLockDeleteMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
		GET_ENGINE->SetError(ForthError::kIllegalOperation, " OLock.delete called with threads blocked on lock");

#ifdef WIN32
        DeleteCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_destroy(pLockStruct->pLock);
#endif
        delete pLockStruct->pLock;
        delete pLockStruct->pBlockedFibers;
		METHOD_RETURN;
	}

	FORTHOP(oLockGrabMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif

		Fiber* pFiber = (Fiber*)(pCore->pFiber);
		if (pLockStruct->pLockHolder == nullptr)
		{
			if (pLockStruct->lockDepth != 0)
			{
				GET_ENGINE->SetError(ForthError::kIllegalOperation, " OLock.grab called with no lock holder and lock depth not 0");
			}
			else
			{
				pLockStruct->pLockHolder = pFiber;
				pLockStruct->lockDepth++;
			}
		}
		else
		{
			if (pLockStruct->pLockHolder == pFiber)
			{
				pLockStruct->lockDepth++;
			}
			else
			{
				pFiber->Block();
				SET_STATE(OpResult::kYield);
				pLockStruct->pBlockedFibers->push_back(pFiber);
			}
		}

#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	FORTHOP(oLockTryGrabMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif

		int result = (int)false;
		Fiber* pFiber = (Fiber*)(pCore->pFiber);
		if (pLockStruct->pLockHolder == nullptr)
		{
			if (pLockStruct->lockDepth != 0)
			{
				GET_ENGINE->SetError(ForthError::kIllegalOperation, " OLock.tryGrab called with no lock holder and lock depth not 0");
			}
			else
			{
				pLockStruct->pLockHolder = pFiber;
				pLockStruct->lockDepth++;
				result = true;
			}
		}
		else
		{
			if (pLockStruct->pLockHolder == pFiber)
			{
				pLockStruct->lockDepth++;
				result = true;
			}
		}
		SPUSH(result);

#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	FORTHOP(oLockUngrabMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif

		if (pLockStruct->pLockHolder == nullptr)
		{
			GET_ENGINE->SetError(ForthError::kIllegalOperation, " OLock.ungrab called on ungrabbed lock");
		}
		else
		{
			if (pLockStruct->pLockHolder != (Fiber*)(pCore->pFiber))
			{
				GET_ENGINE->SetError(ForthError::kIllegalOperation, " OLock.ungrab called by thread which does not have lock");
			}
			else
			{
				if (pLockStruct->lockDepth <= 0)
				{
					GET_ENGINE->SetError(ForthError::kIllegalOperation, " OLock.ungrab called with lock depth <= 0");
				}
				else
				{
					pLockStruct->lockDepth--;
					if (pLockStruct->lockDepth == 0)
					{
						if (pLockStruct->pBlockedFibers->size() == 0)
						{
							pLockStruct->pLockHolder = nullptr;
						}
						else
						{
							Fiber* pFiber = pLockStruct->pBlockedFibers->front();
							pLockStruct->pBlockedFibers->pop_front();
							pLockStruct->pLockHolder = pFiber;
							pLockStruct->lockDepth++;
							pFiber->Wake();
							SET_STATE(OpResult::kYield);
						}
					}
				}
			}
		}

#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	baseMethodEntry oLockMembers[] =
	{
		METHOD("__newOp", oLockNew),
		METHOD("delete", oLockDeleteMethod),
		METHOD("grab", oLockGrabMethod),
		METHOD_RET("tryGrab", oLockTryGrabMethod, RETURNS_NATIVE(BaseType::kInt)),
		METHOD("ungrab", oLockUngrabMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
		MEMBER_VAR("lockDepth", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
		MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),
		MEMBER_VAR("__lockHolder", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),
		MEMBER_VAR("__blockedThreads", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),

		// following must be last in table
		END_MEMBERS
	};


    //////////////////////////////////////////////////////////////////////
    ///
    //                 OSemaphore
    //

    struct oSemaphoreStruct
    {
        forthop*        pMethods;
		REFCOUNTER      refCount;
        cell            id;
        cell            count;
#ifdef WIN32
        CRITICAL_SECTION* pLock;
#else
        pthread_mutex_t* pLock;
#endif
        std::deque<Fiber*> *pBlockedThreads;
    };

    FORTHOP(oSemaphoreNew)
    {
        ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
        ALLOCATE_OBJECT(oSemaphoreStruct, pSemaphoreStruct, pClassVocab);

        pSemaphoreStruct->pMethods = pClassVocab->GetMethods();
        pSemaphoreStruct->refCount = 0;
#ifdef WIN32
        pSemaphoreStruct->pLock = new CRITICAL_SECTION();
        InitializeCriticalSection(pSemaphoreStruct->pLock);
#else
        pSemaphoreStruct->pLock = new pthread_mutex_t;
        pthread_mutexattr_t mutexAttr;
        pthread_mutexattr_init(&mutexAttr);
        pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(pSemaphoreStruct->pLock, &mutexAttr);

        pthread_mutexattr_destroy(&mutexAttr);
#endif
        pSemaphoreStruct->pBlockedThreads = new std::deque<Fiber*>;
        pSemaphoreStruct->count = 0;

        PUSH_OBJECT(pSemaphoreStruct);
    }

    FORTHOP(oSemaphoreDeleteMethod)
    {
        GET_THIS(oSemaphoreStruct, pSemaphoreStruct);
        //GET_ENGINE->SetError(ForthError::kIllegalOperation, " OSemaphore.delete called with threads blocked on lock");

#ifdef WIN32
        DeleteCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_destroy(pSemaphoreStruct->pLock);
#endif
        delete pSemaphoreStruct->pLock;
        delete pSemaphoreStruct->pBlockedThreads;
		METHOD_RETURN;
	}

    FORTHOP(oSemaphoreInitMethod)
    {
        GET_THIS(oSemaphoreStruct, pSemaphoreStruct);
#ifdef WIN32
        EnterCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_lock(pSemaphoreStruct->pLock);
#endif

        pSemaphoreStruct->count = SPOP;

#ifdef WIN32
        LeaveCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_unlock(pSemaphoreStruct->pLock);
#endif
        METHOD_RETURN;
    }

    FORTHOP(oSemaphoreWaitMethod)
    {
        GET_THIS(oSemaphoreStruct, pSemaphoreStruct);
#ifdef WIN32
        EnterCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_lock(pSemaphoreStruct->pLock);
#endif

        if (pSemaphoreStruct->count > 0)
        {
            --(pSemaphoreStruct->count);
        }
        else
        {
            Fiber* pFiber = (Fiber*)(pCore->pFiber);
            pFiber->Block();
            SET_STATE(OpResult::kYield);
            pSemaphoreStruct->pBlockedThreads->push_back(pFiber);
        }

#ifdef WIN32
        LeaveCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_unlock(pSemaphoreStruct->pLock);
#endif
        METHOD_RETURN;
    }

    FORTHOP(oSemaphorePostMethod)
    {
        GET_THIS(oSemaphoreStruct, pSemaphoreStruct);
#ifdef WIN32
        EnterCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_lock(pSemaphoreStruct->pLock);
#endif

        ++(pSemaphoreStruct->count);
        if (pSemaphoreStruct->count > 0)
        {
            if (pSemaphoreStruct->pBlockedThreads->size() > 0)
            {
                Fiber* pFiber = pSemaphoreStruct->pBlockedThreads->front();
                pSemaphoreStruct->pBlockedThreads->pop_front();
                pFiber->Wake();
                SET_STATE(OpResult::kYield);
                --(pSemaphoreStruct->count);
            }
        }

#ifdef WIN32
        LeaveCriticalSection(pSemaphoreStruct->pLock);
#else
        pthread_mutex_unlock(pSemaphoreStruct->pLock);
#endif
        METHOD_RETURN;
    }

    baseMethodEntry oSemaphoreMembers[] =
    {
        METHOD("__newOp", oSemaphoreNew),
        METHOD("delete", oSemaphoreDeleteMethod),
        METHOD("init", oSemaphoreInitMethod),
        METHOD("wait", oSemaphoreWaitMethod),
        METHOD("post", oSemaphorePostMethod),

        MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
        MEMBER_VAR("count", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
        MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),
        MEMBER_VAR("__blockedThreads", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),

        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 OAsyncSemaphore
    //

    struct oAsyncSemaphoreStruct
    {
        forthop*        pMethods;
		REFCOUNTER      refCount;
        cell            id;
#ifdef WIN32
        HANDLE pSemaphore;
#else
        sem_t* pSemaphore;
#endif
    };

    static ClassVocabulary* gpSemaphoreVocabulary;

    void CreateAsyncSemaphoreObject(ForthObject& outSemaphore, Engine *pEngine)
    {
        ALLOCATE_OBJECT(oAsyncSemaphoreStruct, pSemaphoreStruct, gpSemaphoreVocabulary);
        pSemaphoreStruct->pMethods = gpSemaphoreVocabulary->GetMethods();
        pSemaphoreStruct->refCount = 0;
#ifdef WIN32
        pSemaphoreStruct->pSemaphore = 0;
#else
        pSemaphoreStruct->pSemaphore = nullptr;
#endif

        outSemaphore = (ForthObject)pSemaphoreStruct;
    }

    FORTHOP(oAsyncSemaphoreNew)
    {
        GET_ENGINE->SetError(ForthError::kIllegalOperation, " cannot explicitly create a Semaphore object");
    }

    FORTHOP(oAsyncSemaphoreDeleteMethod)
    {
        GET_THIS(oAsyncSemaphoreStruct, pSemaphoreStruct);

#ifdef WIN32
        if (pSemaphoreStruct->pSemaphore)
        {
            CloseHandle(pSemaphoreStruct->pSemaphore);
        }
#else
        if (pSemaphoreStruct->pSemaphore)
        {
            sem_close(pSemaphoreStruct->pSemaphore);
        }
#endif
		METHOD_RETURN;
	}

    FORTHOP(oAsyncSemaphoreInitMethod)
    {
        GET_THIS(oAsyncSemaphoreStruct, pSemaphoreStruct);
        int initialCount = SPOP;
#ifdef WIN32
        // default security attributes, initial count, max count, unnamed semaphore
        pSemaphoreStruct->pSemaphore = CreateSemaphore(nullptr, initialCount, 0x7FFFFFFF, nullptr);

        if (pSemaphoreStruct->pSemaphore == NULL)
        {
            printf("Semaphore:init - CreateSemaphore error: %d\n", GetLastError());
        }
#else
        char semaphoreName[32];
        snprintf(semaphoreName, sizeof(semaphoreName), "forth_%x", pSemaphoreStruct);
        pSemaphoreStruct->pSemaphore = sem_open(semaphoreName, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, initialCount);
    	sem_unlink(semaphoreName);
        if (pSemaphoreStruct->pSemaphore == nullptr)
        {
        	perror("AsyncSemaphore:init");
        }
#endif
        METHOD_RETURN;
    }

    FORTHOP(oAsyncSemaphoreWaitMethod)
    {
        GET_THIS(oAsyncSemaphoreStruct, pSemaphoreStruct);
#ifdef WIN32
        DWORD waitResult = WaitForSingleObject(pSemaphoreStruct->pSemaphore, INFINITE);
        if (waitResult != WAIT_OBJECT_0)
        {
            printf("Semaphore:wait WaitForSingleObject failed (%d)\n", GetLastError());
        }
#else
        sem_wait(pSemaphoreStruct->pSemaphore);
#endif
        METHOD_RETURN;
    }

    FORTHOP(oAsyncSemaphorePostMethod)
    {
        GET_THIS(oAsyncSemaphoreStruct, pSemaphoreStruct);
#ifdef WIN32
        // increment the semaphore to signal all threads waiting for us to exit
        if (!ReleaseSemaphore(pSemaphoreStruct->pSemaphore, 1, NULL))
        {
            printf("Semaphore:post - ReleaseSemaphore error: %d\n", GetLastError());
        }
#else
        sem_post(pSemaphoreStruct->pSemaphore);
#endif
        METHOD_RETURN;
    }

    baseMethodEntry oAsyncSemaphoreMembers[] =
    {
        METHOD("__newOp", oAsyncSemaphoreNew),
        METHOD("delete", oAsyncSemaphoreDeleteMethod),
        METHOD("init", oAsyncSemaphoreInitMethod),
        METHOD("wait", oAsyncSemaphoreWaitMethod),
        METHOD("post", oAsyncSemaphorePostMethod),

        MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, BaseType::kCell)),
        MEMBER_VAR("__semaphore", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),

        // following must be last in table
        END_MEMBERS
    };
   
    
    void AddClasses(OuterInterpreter* pOuter)
	{
		gpAsyncLockVocabulary = pOuter->AddBuiltinClass("AsyncLock", kBCIAsyncLock, kBCIObject, oAsyncLockMembers);
        pOuter->AddBuiltinClass("Lock", kBCILock, kBCIObject, oLockMembers);
        gpSemaphoreVocabulary = pOuter->AddBuiltinClass("AsyncSemaphore", kBCIAsyncSemaphore, kBCIObject, oAsyncSemaphoreMembers);
        pOuter->AddBuiltinClass("Semaphore", kBCISemaphore, kBCIObject, oSemaphoreMembers);
    }

} // namespace OLock

