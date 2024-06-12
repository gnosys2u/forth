#pragma once
//////////////////////////////////////////////////////////////////////
//
// InputStack.h: interface for the InputStack class.
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

class InputStream;

class InputStack
{
public:
    InputStack();
    virtual ~InputStack();

    void                    PushInputStream( InputStream *pStream );
    bool                    PopInputStream();
    ucell                   GetDepth();
    void                    FlushToDepth(ucell depth);

    void                    Reset( void );
    char*                   GetLine( const char *pPrompt );
    char*                   Refill();
    // return null IFF adding another input line would overflow buffer
    char*                   AddLine();
    // return false IFF buffer has less than numChars available
    bool                    Shorten(int numChars);
    inline InputStream* Top(void) { return mpHead; };
    //inline InputStream* Top(void) { return mStack.empty() ? nullptr : mStack.back(); };
	// returns NULL if no filename can be found, else returns name & number of topmost input stream on stack which has info available
	const char*             GetFilenameAndLineNumber(int& lineNumber);

    const char*             GetReadPointer( void );
    const char*             GetBufferBasePointer( void );
    cell*                   GetReadOffsetPointer( void );
    cell                    GetBufferLength( void );
    void                    SetReadPointer( const char *pBuff );
    cell                    GetReadOffset( void );
    void                    SetReadOffset( cell offset );
    cell                    GetWriteOffset( void );
    void                    SetWriteOffset(cell offset );
	virtual bool			IsEmpty();

protected:
    std::vector<InputStream *>   mStack;
    InputStream* mpHead;
};

