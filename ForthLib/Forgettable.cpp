//////////////////////////////////////////////////////////////////////
//
// Forgettable.cpp: implementation of the Forgettable abstract base class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthForgettable.h"
#include "ForthEngine.h"
#include "ForthShell.h"

//############################################################################
//
//   allocatable memory blocks that are cleaned up on "forget"
//
//############################################################################

//////////////////////////////////////////////////////////////////////
////
///     Forgettable - abstract forgettable base class
//
// a forgettable object is associated with a forth op address, when a "forget"
// causes that forth op to be destroyed, the forgettable chain is walked, and
// all forgettables which are associated with ops that have been destroyed
// are also deleted...

Forgettable* Forgettable::mpChainHead = NULL;

Forgettable::Forgettable( void* pOpAddress, forthop op )
: mpNext( mpChainHead )
, mpOpAddress( pOpAddress )
, mOp( op )
{
    mpChainHead = this;
}

Forgettable::~Forgettable()
{
    // remove us from the forgettable chain
    if ( this == mpChainHead )
    {
        mpChainHead = mpNext;
    }
    else
    {
        Forgettable* pNext = mpChainHead;
        while ( pNext != NULL )
        {
            Forgettable* pTmp = pNext->mpNext;

            if ( pTmp == this )
            {
                pNext->mpNext = pTmp->mpNext;
                break;
            }
            pNext = pTmp;
        }
    }
}
const char *
Forgettable::GetName( void )
{
    return "noName";
}

const char *
Forgettable::GetTypeName( void )
{
    return "noType";
}

void
Forgettable::AfterStart()
{
}

int
Forgettable::Save( FILE* pOutFile )
{
    (void) pOutFile;
    return 0;
}

bool
Forgettable::Restore( const char* pBuffer, uint32_t numBytes )
{
    (void) pBuffer;
    (void) numBytes;
    return true;
}


void Forgettable::ForgetPropagate( void* pForgetLimit, forthop op )
{
    Forgettable *pNext;
    Forgettable *pTmp;

    // delete all forgettables that are below the forget limit
    pNext = mpChainHead;
    while ( pNext != NULL )
    {
        pTmp = pNext->mpNext;
        if ( (ucell) pNext->mpOpAddress > (ucell) pForgetLimit )
        {
            delete pNext;
        }
        pNext = pTmp;
    }
    // give each forgettable a chance to do internal cleanup
    pNext = mpChainHead;
    while ( pNext != NULL )
    {
        SPEW_ENGINE( "forgetting %s:%s\n", pNext->GetTypeName(), pNext->GetName() );
        pNext->ForgetCleanup( pForgetLimit, op );
        pNext = pNext->mpNext;
    }
}



