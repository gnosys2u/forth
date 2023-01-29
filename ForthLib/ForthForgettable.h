#pragma once
//////////////////////////////////////////////////////////////////////
//
// Forgettable.h: interface for the Forgettable class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

class Forgettable
{
public:
    Forgettable( void *pOpAddress, forthop op );
    virtual ~Forgettable();

    static void     ForgetPropagate( void *pForgetLimit, forthop op );

    static inline Forgettable *GetForgettableChainHead( void ) {
        return mpChainHead;
    };

    inline Forgettable *GetNextForgettable( void ) {
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
    Forgettable*           mpNext;
private:
    static Forgettable*    mpChainHead;
};


