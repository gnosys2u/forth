//////////////////////////////////////////////////////////////////////
//
// ForthThread.cpp: implementation of the ForthThread and ForthFiber classes.
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
#include "ForthEngine.h"
#include "ForthShowContext.h"
#include "ForthBuiltinClasses.h"

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
    ucell               refCount;
	cell                id;
	ForthThread	*pThread;
};

struct oFiberStruct
{
    forthop*            pMethods;
    ucell               refCount;
    cell                id;
	ForthFiber			*pFiber;
};

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthCoreState struct
// 

ForthCoreState::ForthCoreState(int paramStackSize, int returnStackSize)
    : SLen(paramStackSize)
    , RLen(returnStackSize)
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

    varMode = kVarDefaultOp;
    state = kResultDone;
    error = kForthErrorNone;

    base = DEFAULT_BASE;
    signedPrintMode = kPrintSignedDecimal;
    traceFlags = 0;

    for (int i = 0; i < NUM_CORE_SCRATCH_CELLS; i++)
    {
        scratch[i] = 0;
    }
}


void ForthCoreState::InitializeFromEngine(void* engineIn)
{
    ForthEngine* engine = (ForthEngine *)engineIn;
    pEngine = engineIn;
    pDictionary = engine->GetDictionaryMemorySection();
    //    core.pFileFuncs = mpShell->GetFileInterface();

    ForthCoreState* pEngineCore = engine->GetCoreState();
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
//                     ForthFiber
// 

ForthFiber::ForthFiber(ForthEngine *pEngine, ForthThread *pParentThread, int fiberIndex, int paramStackLongs, int returnStackLongs)
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

ForthFiber::~ForthFiber()
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

void ForthFiber::Destroy()
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
ForthFiber::CheckGaurdAreas( void )
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

void ForthFiber::InitTables(ForthFiber* pSourceThread)
{
	ForthCoreState& sourceCore = pSourceThread->mCore;
	mCore.optypeAction = sourceCore.optypeAction;
	mCore.numBuiltinOps = sourceCore.numBuiltinOps;
	mCore.numOps = sourceCore.numOps;
	mCore.maxOps = sourceCore.maxOps;
	mCore.ops = sourceCore.ops;
	mCore.innerLoop = sourceCore.innerLoop;
    mCore.innerExecute = sourceCore.innerExecute;
}

void
ForthFiber::Reset( void )
{
    mCore.SP = mCore.ST;
    mCore.RP = mCore.RT;
    mCore.FP = nullptr;
    mCore.TP = nullptr;

    mCore.error = kForthErrorNone;
    mCore.state = kResultDone;
    mCore.varMode = kVarDefaultOp;
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
ForthFiber::ResetIP( void )
{
	mCore.IP = &(mOps[0]);
	//mCore.IP = nullptr;
}

void ForthFiber::Run()
{
    eForthResult exitStatus;

	Wake();
	mCore.IP = &(mOps[0]);
	//mCore.IP = nullptr;
	// the user defined ops could have changed since this thread was created, update it to match the engine
	ForthCoreState* pEngineState = mpEngine->GetCoreState();
	mCore.ops = pEngineState->ops;
	mCore.numOps = pEngineState->numOps;
#ifdef ASM_INNER_INTERPRETER
    if ( mpEngine->GetFastMode() )
    {
		do
		{
			exitStatus = InnerInterpreterFast(&mCore);
		} while (exitStatus == kResultYield);
    }
    else
#endif
    {
		do
		{
			exitStatus = InnerInterpreter(&mCore);
		} while (exitStatus == kResultYield);
    }
}

void ForthFiber::Join(ForthFiber* pJoiningFiber)
{
    //printf("Join: thread %x is waiting for %x to exit\n", pJoiningThread, this);
    if (mRunState != kFTRSExited)
    {
        ForthObject& joiner = pJoiningFiber->GetFiberObject();
        SAFE_KEEP(joiner);
        pJoiningFiber->Block();
        pJoiningFiber->mpNextJoiner = mpJoinHead;
        mpJoinHead = pJoiningFiber;
    }
}

void ForthFiber::WakeAllJoiningFibers()
{
    ForthFiber* pFiber = mpJoinHead;
    //printf("WakeAllJoiningFibers: thread %x is exiting\n", this);
    while (pFiber != nullptr)
    {
        ForthFiber* pNextFiber = pFiber->mpNextJoiner;
        //printf("WakeAllJoiningFibers: waking thread %x\n", pFiber);
        pFiber->mpNextJoiner = nullptr;
        pFiber->Wake();
        ForthObject& joiner = pFiber->GetFiberObject();
        //SAFE_RELEASE(&mCore, joiner);
		if (joiner != nullptr)
		{
			joiner->refCount -= 1;
			if (joiner->refCount == 0)
			{
				((ForthEngine*)(mCore.pEngine))->DeleteObject(&mCore, joiner);
			}
		}
		TRACK_RELEASE;
        pFiber = pNextFiber;
    }
    mpJoinHead = nullptr;
}

ForthShowContext* ForthFiber::GetShowContext()
{
	if (mpShowContext == NULL)
	{
		mpShowContext = new ForthShowContext;
	}
	return mpShowContext;
}

void ForthFiber::SetRunState(eForthFiberRunState newState)
{
	// TODO!
	mRunState = newState;
}

void ForthFiber::Sleep(uint32_t sleepMilliSeconds)
{
	uint32_t now = mpEngine->GetElapsedTime();
#ifdef WIN32
	mWakeupTime = (sleepMilliSeconds == MAXINT32) ? MAXINT32 : now + sleepMilliSeconds;
#else
	mWakeupTime = (sleepMilliSeconds == INT32_MAX) ? INT32_MAX : now + sleepMilliSeconds;
#endif
	mRunState = kFTRSSleeping;
}

void ForthFiber::Block()
{
	mRunState = kFTRSBlocked;
}

void ForthFiber::Wake()
{
	mRunState = kFTRSReady;
	mWakeupTime = 0;
}

void ForthFiber::Stop()
{
	mRunState = kFTRSStopped;
}

void ForthFiber::Exit()
{
	mRunState = kFTRSExited;
    WakeAllJoiningFibers();
}

const char* ForthFiber::GetName() const
{
    return mName.c_str();
}

void ForthFiber::SetName(const char* newName)
{
    mName.assign(newName);
}

void ForthFiber::FreeObjects()
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
//                     ForthThread
// 

ForthThread::ForthThread(ForthEngine *pEngine, int paramStackLongs, int returnStackLongs)
	: mHandle(0)
#ifdef WIN32
	, mThreadId(0)
#endif
	, mpNext(NULL)
	, mActiveFiberIndex(0)
	, mRunState(kFTRSStopped)
    , mObject(nullptr)
{
	ForthFiber* pPrimaryFiber = new ForthFiber(pEngine, this, 0, paramStackLongs, returnStackLongs);
    pPrimaryFiber->SetRunState(kFTRSReady);
	mFibers.push_back(pPrimaryFiber);
#ifdef WIN32
    // default security attributes, manual reset event, initially nonsignaled, unnamed event
    mExitSignal = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (mExitSignal == NULL)
    {
        printf("ForthThread constructor - CreateEvent error: %d\n", GetLastError());
    }
#else
    pthread_mutex_init(&mExitMutex, nullptr);
    pthread_cond_init(&mExitSignal, nullptr);
#endif
}

ForthThread::~ForthThread()
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

    for (ForthFiber* pFiber : mFibers)
	{
		if (pFiber != nullptr)
		{
			delete pFiber;
		}
	}
}

void ForthThread::FreeObjects()
{
    for (ForthFiber* pFiber : mFibers)
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
unsigned __stdcall ForthThread::RunLoop(void *pUserData)
#else
void* ForthThread::RunLoop(void *pUserData)
#endif
{
    ForthThread* pParentThread = (ForthThread*)pUserData;
	ForthFiber* pActiveFiber = pParentThread->GetActiveFiber();
	ForthEngine* pEngine = pActiveFiber->GetEngine();
    //printf("Starting thread %x\n", pParentThread);

    pParentThread->mRunState = kFTRSReady;
	eForthResult exitStatus = kResultOk;
	bool keepRunning = true;
	while (keepRunning)
	{
		bool checkForAllDone = false;
		ForthCoreState* pCore = pActiveFiber->GetCore();
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

		bool switchActiveThread = false;
		switch (exitStatus)
		{
		case kResultYield:
			switchActiveThread = true;
			SET_STATE(kResultOk);
			break;

		case kResultDone:
			switchActiveThread = true;
			checkForAllDone = true;
			break;

		default:
			break;
		}

		if (switchActiveThread)
		{
			// TODO!
			// - switch to next runnable thread
			// - sleep if all threads are sleeping
			// - deal with all threads stopped
			uint32_t now = pEngine->GetElapsedTime();

			ForthFiber* pNextFiber = pParentThread->GetNextReadyFiber();
			if (pNextFiber != nullptr)
			{
				pActiveFiber = pNextFiber;
			}
			else
			{
				pNextFiber = pParentThread->GetNextSleepingFiber();
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
						// TODO: don't always sleep until wakeupTime, since threads which are blocked or stopped
						//  could be unblocked or started by other async threads
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
			for (ForthFiber* pFiber : pParentThread->mFibers)
			{
				if (pFiber->GetRunState() != kFTRSExited)
				{
					keepRunning = true;
					break;
				}
			}
		}
	}

    //printf("Exiting thread %x signaling exitEvent %d\n", pParentThread, pParentThread->mExitSignal);
#ifdef WIN32
	pParentThread->mRunState = kFTRSExited;
    if (!SetEvent(pParentThread->mExitSignal))
    {
        printf("ForthThread::RunLoop SetEvent failed (%d)\n", GetLastError());
    }

    return 0;
#else
    pthread_mutex_lock(&pParentThread->mExitMutex);
	pParentThread->mRunState = kFTRSExited;
	pthread_cond_broadcast(&pParentThread->mExitSignal);
    pthread_mutex_unlock(&pParentThread->mExitMutex);

    return nullptr;
#endif
}

void ForthThread::InnerLoop()
{
    ForthFiber* pMainFiber = mFibers[0];
    ForthFiber* pActiveFiber = pMainFiber;
    ForthEngine* pEngine = pActiveFiber->GetEngine();
    pMainFiber->SetRunState(kFTRSReady);

    eForthResult exitStatus = kResultOk;
    bool keepRunning = true;
    while (keepRunning)
    {
        ForthCoreState* pCore = pActiveFiber->GetCore();
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
        if ((exitStatus == kResultYield) || (pActiveFiber->GetRunState() == kFTRSExited))
        {
            switchActiveFiber = true;
            pCore->state = kResultOk;
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
                ForthFiber* pFiber = mFibers[i];
                printf("Fiber %d 0x%x   runState %s   coreState %d\n", i, (int)pFiber,
                    runStateNames[pFiber->GetRunState()], pFiber->GetCore()->state);
            }
            */
        }

        if (switchActiveFiber)
        {
            // TODO!
            // - switch to next runnable thread
            // - sleep if all threads are sleeping
            // - deal with all threads stopped
            uint32_t now = pEngine->GetElapsedTime();

            ForthFiber* pNextFiber = GetNextReadyFiber();
            if (pNextFiber != nullptr)
            {
                //printf("Switching from thread 0x%x to ready thread 0x%x\n", pActiveFiber, pNextFiber);
                pActiveFiber = pNextFiber;
                SetActiveFiber(pActiveFiber);
            }
            else
            {
                pNextFiber = GetNextSleepingFiber();
                if (pNextFiber != nullptr)
                {
                    uint32_t wakeupTime = pNextFiber->GetWakeupTime();
                    if (now >= wakeupTime)
                    {
                        //printf("Switching from thread 0x%x to waking thread 0x%x\n", pActiveFiber, pNextFiber);
                        pNextFiber->Wake();
                        pActiveFiber = pNextFiber;
                        SetActiveFiber(pActiveFiber);
                    }
                    else
                    {
                        // TODO: don't always sleep until wakeupTime, since threads which are blocked or stopped
                        //  could be unblocked or started by other async threads
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

        eForthResult mainFiberState = (eForthResult) pMainFiber->GetCore()->state;
        keepRunning = false;
        if ((exitStatus != kResultDone) && (pMainFiber->GetRunState() != kFTRSExited))
        {
            switch (pMainFiber->GetCore()->state)
            {
            case kResultOk:
            case kResultYield:
                keepRunning = true;
                break;
            default:
                break;
            }
        }
    }
}

ForthFiber* ForthThread::GetFiber(int threadIndex)
{
	if (threadIndex < (int)mFibers.size())
	{
		return mFibers[threadIndex];
	}
	return nullptr;
}

ForthFiber* ForthThread::GetActiveFiber()
{
	if (mActiveFiberIndex < (int)mFibers.size())
	{
		return mFibers[mActiveFiberIndex];
	}
	return nullptr;
}


void ForthThread::SetActiveFiber(ForthFiber *pFiber)
{
    // TODO: verify that active fiber is actually a child of this thread
    int fiberIndex = pFiber->GetIndex();
    // ASSERT(fiberIndex < (int)mFibers.size() && mFibers[fiberIndex] == pFiber);
    mActiveFiberIndex = fiberIndex;
}

void ForthThread::Reset(void)
{ 
	for (ForthFiber* pFiber : mFibers)
	{
		if (pFiber != nullptr)
		{
			pFiber->Reset();
		}
	}
}

ForthFiber* ForthThread::GetNextReadyFiber()
{
	int originalFiberIndex = mActiveFiberIndex;

	do	
	{
		mActiveFiberIndex++;
		if (mActiveFiberIndex >= (int)mFibers.size())
		{
			mActiveFiberIndex = 0;
		}
		ForthFiber* pNextFiber = mFibers[mActiveFiberIndex];
		if (pNextFiber->GetRunState() == kFTRSReady)
		{
			return pNextFiber;
		}
	} while (originalFiberIndex != mActiveFiberIndex);

	return nullptr;
}

ForthFiber* ForthThread::GetNextSleepingFiber()
{
	ForthFiber* pFiberToWake = nullptr;
	int originalFiberIndex = mActiveFiberIndex;
	uint32_t minWakeupTime = (uint32_t)(~0);
	do
	{
        mActiveFiberIndex++;
        if (mActiveFiberIndex >= (int)mFibers.size())
		{
			mActiveFiberIndex = 0;
		}
		ForthFiber* pNextFiber = mFibers[mActiveFiberIndex];
		if (pNextFiber->GetRunState() == kFTRSSleeping)
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

int32_t ForthThread::Start()
{
#ifdef WIN32
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		::CloseHandle(mHandle);
	}
	mHandle = (HANDLE)_beginthreadex(NULL, 0, ForthThread::RunLoop, this, 0, (unsigned *)&mThreadId);
#else
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		// TODO
		//::CloseHandle( mHandle );
	}
	mHandle = pthread_create(&mThread, NULL, ForthThread::RunLoop, this);

#endif
	return (int32_t)mHandle;
}

void ForthThread::Exit()
{
	// TBD: make sure this isn't the main thread
	if (mpNext != NULL)
	{
#ifdef WIN32
		mRunState = kFTRSExited;
        // signal all threads waiting for us to exit
        if (!SetEvent(mExitSignal))
        {
            printf("ForthThread::Exit SetEvent error: %d\n", GetLastError());
        }
        _endthreadex(0);
#else
        pthread_mutex_lock(&mExitMutex);
		mRunState = kFTRSExited;
    	pthread_cond_broadcast(&mExitSignal);
        pthread_mutex_unlock(&mExitMutex);
        pthread_exit(&mExitStatus);
#endif
    }
}

void ForthThread::Join()
{
#ifdef WIN32
    DWORD waitResult = WaitForSingleObject(mExitSignal, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        printf("ForthThread::Join WaitForSingleObject failed (%d)\n", GetLastError());
    }
#else
    pthread_mutex_lock(&mExitMutex);
	while (mRunState != kFTRSExited)
	{
		pthread_cond_wait(&mExitSignal, &mExitMutex);
	}
    pthread_mutex_unlock(&mExitMutex);
#endif
}

ForthFiber* ForthThread::CreateFiber(ForthEngine *pEngine, forthop threadOp, int paramStackLongs, int returnStackLongs)
{
	ForthFiber* pFiber = new ForthFiber(pEngine, this, (int)mFibers.size(), paramStackLongs, returnStackLongs);
	pFiber->SetOp(threadOp);
	pFiber->SetRunState(kFTRSStopped);
	ForthFiber* pPrimaryFiber = GetFiber(0);
	pFiber->InitTables(pPrimaryFiber);
	mFibers.push_back(pFiber);

	return pFiber;
}

void ForthThread::DeleteFiber(ForthFiber* pInFiber)
{
	// TODO: do something when erasing last thread?  what about thread 0?
	int lastIndex = mFibers.size() - 1;
	for (int i = 0; i <= lastIndex; ++i)
	{
		ForthFiber* pFiber = mFibers[i];
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


const char* ForthThread::GetName() const
{
    return mName.c_str();
}

void ForthThread::SetName(const char* newName)
{
    mName.assign(newName);
}


namespace OThread
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 oThread
	//

	static ForthClassVocabulary* gpThreadVocabulary;
	static ForthClassVocabulary* gpFiberVocabulary;

	void CreateThreadObject(ForthObject& outThread, ForthEngine *pEngine, forthop threadOp, int paramStackLongs, int returnStackLongs)
	{
		MALLOCATE_OBJECT(oThreadStruct, pThreadStruct, gpThreadVocabulary);
        pThreadStruct->pMethods = gpThreadVocabulary->GetMethods();
        pThreadStruct->refCount = 1;
		ForthThread* pThread = pEngine->CreateThread(threadOp, paramStackLongs, returnStackLongs);
		pThreadStruct->pThread = pThread;
		pThread->Reset();
		outThread = (ForthObject) pThreadStruct;
		pThread->SetThreadObject(outThread);
		OThread::FixupFiber(pThread->GetFiber(0));
	}

	FORTHOP(oThreadNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a Thread object");
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
		ForthThread* pThread = pThreadStruct->pThread;
		ForthFiber* pFiber = pThread->GetFiber(0);
		pFiber->Reset();
		int32_t result = pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartWithArgsMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthThread* pThread = pThreadStruct->pThread;
		ForthFiber* pFiber = pThread->GetFiber(0);
		pFiber->Reset();
		ForthCoreState* pDstCore = pFiber->GetCore();
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
		int32_t result = pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

    FORTHOP(oThreadJoinMethod)
    {
        GET_THIS(oThreadStruct, pThreadStruct);
        ForthThread* pThread = pThreadStruct->pThread;
        pThread->Join();
        METHOD_RETURN;
    }

    FORTHOP(oThreadResetMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oThreadCreateFiberMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthEngine* pEngine = GET_ENGINE;
		ForthObject thread;

		int returnStackLongs = (int)SPOP;
		int paramStackLongs = (int)SPOP;
        forthop fiberOp = (forthop)SPOP;
        OThread::CreateFiberObject(thread, pThreadStruct->pThread, pEngine, fiberOp, paramStackLongs, returnStackLongs);

		PUSH_OBJECT(thread);
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
		METHOD_RET("start", oThreadStartMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("startWithArgs", oThreadStartWithArgsMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("join", oThreadJoinMethod),
        METHOD("reset", oThreadResetMethod),
		METHOD_RET("createFiber", oThreadCreateFiberMethod, RETURNS_OBJECT(kBCIFiber)),
		METHOD_RET("getRunState", oThreadGetRunStateMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("getName", oThreadGetNameMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("setName", oThreadSetNameMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

		// following must be last in table
		END_MEMBERS
	};
	

	//////////////////////////////////////////////////////////////////////
	///
	//                 oFiber
	//

	void CreateFiberObject(ForthObject& outFiber, ForthThread *pParentThread, ForthEngine *pEngine, forthop threadOp, int paramStackLongs, int returnStackLongs)
	{
		MALLOCATE_OBJECT(oFiberStruct, pFiberStruct, gpFiberVocabulary);
        pFiberStruct->pMethods = gpFiberVocabulary->GetMethods();
        pFiberStruct->refCount = 1;
		pFiberStruct->pFiber = pParentThread->CreateFiber(pEngine, threadOp, paramStackLongs, returnStackLongs);
		pFiberStruct->pFiber->Reset();

        outFiber = (ForthObject)pFiberStruct;
        pFiberStruct->pFiber->SetFiberObject(outFiber);
	}

	void FixupFiber(ForthFiber* pFiber)
	{
		MALLOCATE_OBJECT(oFiberStruct, pFiberStruct, gpFiberVocabulary);
		ForthObject& primaryFiber = pFiber->GetFiberObject();
        pFiberStruct->pMethods = gpFiberVocabulary->GetMethods();
        pFiberStruct->refCount = 1;
		pFiberStruct->pFiber = pFiber;
		primaryFiber = (ForthObject)pFiberStruct;
	}

	void FixupThread(ForthThread* pThread)
	{
		MALLOCATE_OBJECT(oThreadStruct, pThreadStruct, gpThreadVocabulary);
        pThreadStruct->pMethods = gpThreadVocabulary->GetMethods();
        pThreadStruct->refCount = 1;
		pThreadStruct->pThread = pThread;
		ForthObject& primaryThread = pThread->GetThreadObject();
        primaryThread = (ForthObject)pThreadStruct;

        OThread::FixupFiber(pThread->GetFiber(0));
	}

	FORTHOP(oFiberNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a Fiber object");
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
		pFiberStruct->pFiber->SetRunState(kFTRSReady);
		METHOD_RETURN;
	}

	FORTHOP(oFiberStartWithArgsMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		ForthFiber* pFiber = pFiberStruct->pFiber;
		ForthCoreState* pDstCore = pFiber->GetCore();
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
		pFiber->SetRunState(kFTRSReady);
        SPUSH(-1);
		METHOD_RETURN;
	}

	FORTHOP(oFiberStopMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
		pFiberStruct->pFiber->SetRunState(kFTRSStopped);
		METHOD_RETURN;
	}

    FORTHOP(oFiberJoinMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        ForthFiber* pJoiner = pFiberStruct->pFiber->GetParent()->GetActiveFiber();
        pFiberStruct->pFiber->Join(pJoiner);
        SET_STATE(kResultYield);
        METHOD_RETURN;
    }

    FORTHOP(oFiberSleepMethod)
	{
		GET_THIS(oFiberStruct, pFiberStruct);
        SET_STATE(kResultYield);
        uint32_t sleepMilliseconds = SPOP;
		pFiberStruct->pFiber->Sleep(sleepMilliseconds);
		METHOD_RETURN;
	}

    FORTHOP(oFiberWakeMethod)
    {
        GET_THIS(oFiberStruct, pFiberStruct);
        SET_STATE(kResultYield);
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
		ForthFiber* pFiber = pFiberStruct->pFiber;
		ForthCoreState* pFiberCore = pFiber->GetCore();
		forthop op = *(pFiberCore->IP)++;
		int32_t result;
#ifdef ASM_INNER_INTERPRETER
        ForthEngine *pEngine = GET_ENGINE;
		if (pEngine->GetFastMode())
		{
			result = (int32_t)InterpretOneOpFast(pFiberCore, op);
		}
		else
#endif
		{
			result = (int32_t)InterpretOneOp(pFiberCore, op);
		}
		if (result == kResultDone)
		{
			pFiber->ResetIP();
		}
		SPUSH(result);
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
        SPUSH((int32_t)(pFiberStruct->pFiber->GetCore()));
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
        METHOD_RET("startWithArgs", oFiberStartWithArgsMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("stop", oFiberStopMethod),
        METHOD("join", oFiberJoinMethod),
        METHOD("sleep", oFiberSleepMethod),
        METHOD("wake", oFiberWakeMethod),
        METHOD("push", oFiberPushMethod),
		METHOD_RET("pop", oFiberPopMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("rpush", oFiberRPushMethod),
		METHOD_RET("rpop", oFiberRPopMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("getRunState", oFiberGetRunStateMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("step", oFiberStepMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("reset", oFiberResetMethod),
		METHOD("resetIP", oFiberResetIPMethod),
        METHOD("getCore", oFiberGetCoreMethod),
        METHOD_RET("getName", oFiberGetNameMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("setName", oFiberSetNameMethod),
        //METHOD_RET("getParent", oFiberGetParentMethod, RETURNS_NATIVE(kBaseType)),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		gpFiberVocabulary = pEngine->AddBuiltinClass("Fiber", kBCIFiber, kBCIObject, oFiberMembers);
		gpThreadVocabulary = pEngine->AddBuiltinClass("Thread", kBCIThread, kBCIObject, oThreadMembers);
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
        ucell           refCount;
		cell            id;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
        pthread_mutex_t* pLock;
#endif
	};

	static ForthClassVocabulary* gpAsyncLockVocabulary;

	void CreateAsyncLockObject(ForthObject& outAsyncLock, ForthEngine *pEngine)
	{
		MALLOCATE_OBJECT(oAsyncLockStruct, pLockStruct, gpAsyncLockVocabulary);
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
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create an AsyncLock object");
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
		SPUSH((int)result);
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
		METHOD_RET("tryGrab", oAsyncLockTryGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("ungrab", oAsyncLockUngrabMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
		MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),
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
        ucell           refCount;
		cell            id;
		cell            lockDepth;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
		pthread_mutex_t* pLock;
#endif
		ForthFiber* pLockHolder;
		std::deque<ForthFiber*> *pBlockedFibers;
	};

	FORTHOP(oLockNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oLockStruct, pLockStruct, pClassVocab);

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
		pLockStruct->pBlockedFibers = new std::deque<ForthFiber*>;
		pLockStruct->lockDepth = 0;

		PUSH_OBJECT(pLockStruct);
	}

	FORTHOP(oLockDeleteMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.delete called with threads blocked on lock");

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

		ForthFiber* pFiber = (ForthFiber*)(pCore->pFiber);
		if (pLockStruct->pLockHolder == nullptr)
		{
			if (pLockStruct->lockDepth != 0)
			{
				GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.grab called with no lock holder and lock depth not 0");
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
				SET_STATE(kResultYield);
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
		ForthFiber* pFiber = (ForthFiber*)(pCore->pFiber);
		if (pLockStruct->pLockHolder == nullptr)
		{
			if (pLockStruct->lockDepth != 0)
			{
				GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.tryGrab called with no lock holder and lock depth not 0");
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
			GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.ungrab called on ungrabbed lock");
		}
		else
		{
			if (pLockStruct->pLockHolder != (ForthFiber*)(pCore->pFiber))
			{
				GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.ungrab called by thread which does not have lock");
			}
			else
			{
				if (pLockStruct->lockDepth <= 0)
				{
					GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.ungrab called with lock depth <= 0");
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
							ForthFiber* pFiber = pLockStruct->pBlockedFibers->front();
							pLockStruct->pBlockedFibers->pop_front();
							pLockStruct->pLockHolder = pFiber;
							pLockStruct->lockDepth++;
							pFiber->Wake();
							SET_STATE(kResultYield);
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
		METHOD_RET("tryGrab", oLockTryGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("ungrab", oLockUngrabMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
		MEMBER_VAR("lockDepth", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
		MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),
		MEMBER_VAR("__lockHolder", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),
		MEMBER_VAR("__blockedThreads", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

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
        uint32_t			refCount;
        cell            id;
        cell            count;
#ifdef WIN32
        CRITICAL_SECTION* pLock;
#else
        pthread_mutex_t* pLock;
#endif
        std::deque<ForthFiber*> *pBlockedThreads;
    };

    FORTHOP(oSemaphoreNew)
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        MALLOCATE_OBJECT(oSemaphoreStruct, pSemaphoreStruct, pClassVocab);

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
        pSemaphoreStruct->pBlockedThreads = new std::deque<ForthFiber*>;
        pSemaphoreStruct->count = 0;

        PUSH_OBJECT(pSemaphoreStruct);
    }

    FORTHOP(oSemaphoreDeleteMethod)
    {
        GET_THIS(oSemaphoreStruct, pSemaphoreStruct);
        //GET_ENGINE->SetError(kForthErrorIllegalOperation, " OSemaphore.delete called with threads blocked on lock");

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
            ForthFiber* pFiber = (ForthFiber*)(pCore->pFiber);
            pFiber->Block();
            SET_STATE(kResultYield);
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
                ForthFiber* pFiber = pSemaphoreStruct->pBlockedThreads->front();
                pSemaphoreStruct->pBlockedThreads->pop_front();
                pFiber->Wake();
                SET_STATE(kResultYield);
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

        MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
        MEMBER_VAR("count", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
        MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),
        MEMBER_VAR("__blockedThreads", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

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
        uint32_t			refCount;
        cell            id;
#ifdef WIN32
        HANDLE pSemaphore;
#else
        sem_t* pSemaphore;
#endif
    };

    static ForthClassVocabulary* gpSemaphoreVocabulary;

    void CreateAsyncSemaphoreObject(ForthObject& outSemaphore, ForthEngine *pEngine)
    {
        MALLOCATE_OBJECT(oAsyncSemaphoreStruct, pSemaphoreStruct, gpSemaphoreVocabulary);
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
        GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a Semaphore object");
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

        MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeCell)),
        MEMBER_VAR("__semaphore", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

        // following must be last in table
        END_MEMBERS
    };
   
    
    void AddClasses(ForthEngine* pEngine)
	{
		gpAsyncLockVocabulary = pEngine->AddBuiltinClass("AsyncLock", kBCIAsyncLock, kBCIObject, oAsyncLockMembers);
        pEngine->AddBuiltinClass("Lock", kBCILock, kBCIObject, oLockMembers);
        gpSemaphoreVocabulary = pEngine->AddBuiltinClass("AsyncSemaphore", kBCIAsyncSemaphore, kBCIObject, oAsyncSemaphoreMembers);
        pEngine->AddBuiltinClass("Semaphore", kBCISemaphore, kBCIObject, oSemaphoreMembers);
    }

} // namespace OLock

