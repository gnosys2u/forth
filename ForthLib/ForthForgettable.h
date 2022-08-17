#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthForgettable.h: interface for the ForthForgettable class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

class ForthForgettable
{
public:
    ForthForgettable( void *pOpAddress, forthop op );
    virtual ~ForthForgettable();

    static void     ForgetPropagate( void *pForgetLimit, forthop op );

    static inline ForthForgettable *GetForgettableChainHead( void ) {
        return mpChainHead;
    };

    inline ForthForgettable *GetNextForgettable( void ) {
        return mpNext;
    };

    // type of forgettable, like 'vocabulary' or 'globalObject'
    virtual const char* GetTypeName();
    virtual const char* GetName();

    virtual void AfterStart();
    virtual int Save( FILE* pOutFile );
    // return false
    virtual bool Restore( const char* pBuffer, uint32_t numBytes );

protected:
    virtual void    ForgetCleanup( void *pForgetLimit, forthop op ) = 0;

    void*                       mpOpAddress;
    int32_t                        mOp;
    ForthForgettable*           mpNext;
private:
    static ForthForgettable*    mpChainHead;
};


