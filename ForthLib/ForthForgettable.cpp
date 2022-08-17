//////////////////////////////////////////////////////////////////////
//
// ForthForgettable.cpp: implementation of the ForthForgettable abstract base class.
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
///     ForthForgettable - abstract forgettable base class
//
// a forgettable object is associated with a forth op address, when a "forget"
// causes that forth op to be destroyed, the forgettable chain is walked, and
// all forgettables which are associated with ops that have been destroyed
// are also deleted...

ForthForgettable* ForthForgettable::mpChainHead = NULL;

ForthForgettable::ForthForgettable( void* pOpAddress, forthop op )
: mpNext( mpChainHead )
, mpOpAddress( pOpAddress )
, mOp( op )
{
    mpChainHead = this;
}

ForthForgettable::~ForthForgettable()
{
    // remove us from the forgettable chain
    if ( this == mpChainHead )
    {
        mpChainHead = mpNext;
    }
    else
    {
        ForthForgettable* pNext = mpChainHead;
        while ( pNext != NULL )
        {
            ForthForgettable* pTmp = pNext->mpNext;

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
ForthForgettable::GetName( void )
{
    return "noName";
}

const char *
ForthForgettable::GetTypeName( void )
{
    return "noType";
}

void
ForthForgettable::AfterStart()
{
}

int
ForthForgettable::Save( FILE* pOutFile )
{
    (void) pOutFile;
    return 0;
}

bool
ForthForgettable::Restore( const char* pBuffer, unsigned int numBytes )
{
    (void) pBuffer;
    (void) numBytes;
    return true;
}


void ForthForgettable::ForgetPropagate( void* pForgetLimit, forthop op )
{
    ForthForgettable *pNext;
    ForthForgettable *pTmp;

    // delete all forgettables that are below the forget limit
    pNext = mpChainHead;
    while ( pNext != NULL )
    {
        pTmp = pNext->mpNext;
        if ( (ulong) pNext->mpOpAddress > (ulong) pForgetLimit )
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



