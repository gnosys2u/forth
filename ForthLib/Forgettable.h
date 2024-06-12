#pragma once
//////////////////////////////////////////////////////////////////////
//
// Forgettable.h: interface for the Forgettable class.
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


