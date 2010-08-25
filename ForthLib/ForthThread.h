#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthThread.h: interface for the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthInner.h"

class ForthEngine;

#define DEFAULT_PSTACK_SIZE 128
#define DEFAULT_RSTACK_SIZE 128


// define CHECK_GAURD_AREAS if there is a bug where memory immediately above or below the stacks is getting trashed
//#define CHECK_GAURD_AREAS

#ifdef CHECK_GAURD_AREAS
#define CHECK_STACKS(THREAD_PTR)    (THREAD_PTR)->CheckGaurdAreas()
#else
#define CHECK_STACKS(THREAD_PTR)
#endif

class ForthThread  
{
public:
    ForthThread( ForthEngine *pEngine, int paramStackLongs=DEFAULT_PSTACK_SIZE, int returnStackLongs=DEFAULT_PSTACK_SIZE );
    virtual ~ForthThread();

#ifdef CHECK_GAURD_AREAS
    bool CheckGaurdAreas();
#endif

    virtual void Activate( ForthCoreState* pCore );
    virtual void Deactivate( ForthCoreState* pCore );

    void                Reset( void );

    inline void         SetIP( long *pNewIP ) { mState.IP = pNewIP; };

    inline void         Push( long value ) { *--mState.SP = value; };
    inline long         Pop() { return *mState.SP++; };
    inline void         RPush( long value ) { *--mState.RP = value; };
    inline long         RPop() { return *mState.RP++; };

    friend class ForthEngine;

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    
    ForthThreadState    mState;
};

class ForthThreadQueue
{
public:
    ForthThreadQueue( int initialSize=16 );
    ~ForthThreadQueue();

    void                AddThread( ForthThread* pThread );

    // how many threads are in queue
    int                 Count();

    // returns NULL if queue is empty
    ForthThread*        RemoveThread();

protected:
    ForthThread**   mQueue;
    int             mFirst;
    int             mCount;
    int             mSize;
};

