#pragma once
//////////////////////////////////////////////////////////////////////
//
// ConsoleInputStream.h: interface for the ConsoleInputStream class.
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

//#include "InputStream.h"
#include "InputStream.h"

// save-input items:
//  0   4
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class ConsoleInputStream : public InputStream
{
public:
    ConsoleInputStream( int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ConsoleInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual char*   AddLine();
    virtual bool    IsInteractive(void) { return true; };
    virtual InputStreamType GetType(void) const;
    virtual const char* GetName( void ) const;
    virtual cell      GetSourceID() const;

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

protected:
    int             mLineNumber;    // number of times GetLine has been called
    cell            mState[8];
};


