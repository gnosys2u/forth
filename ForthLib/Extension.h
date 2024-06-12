#pragma once


//////////////////////////////////////////////////////////////////////
//
// Extension.h: interface for the Extension class.
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

#include "Forth.h"

class Engine;

class Extension
{
public:
    Extension();
    virtual ~Extension();

    // Initialize is called by the engine when it is done initializing itself
    //   the extension should load its built-in ops (if any) when Initialize is called
    virtual void Initialize( Engine* pEngine );

    // Reset is called by the engine every time there is an error reset
    virtual void Reset();

    // Shutdown is called by the engine when it is about to be deleted
    virtual void Shutdown();

    // ForgetOp is called whenever a forget operation occurs, so the extension can cleanup any
    //  internal state that is affected by the ops which were just deleted
    virtual void ForgetOp( uint32_t opNumber );

protected:
    Engine*    mpEngine;
};
