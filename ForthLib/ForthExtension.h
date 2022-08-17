#pragma once


//////////////////////////////////////////////////////////////////////
//
// ForthExtension.h: interface for the ForthExtension class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "Forth.h"

class ForthEngine;

class ForthExtension
{
public:
    ForthExtension();
    virtual ~ForthExtension();

    // Initialize is called by the engine when it is done initializing itself
    //   the extension should load its built-in ops (if any) when Initialize is called
    virtual void Initialize( ForthEngine* pEngine );

    // Reset is called by the engine every time there is an error reset
    virtual void Reset();

    // Shutdown is called by the engine when it is about to be deleted
    virtual void Shutdown();

    // ForgetOp is called whenever a forget operation occurs, so the extension can cleanup any
    //  internal state that is affected by the ops which were just deleted
    virtual void ForgetOp( uint32_t opNumber );

protected:
    ForthEngine*    mpEngine;
};
