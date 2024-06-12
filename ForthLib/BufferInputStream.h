#pragma once
//////////////////////////////////////////////////////////////////////
//
// BufferInputStream.h: interface for the BufferInputStream class.
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

#include "InputStream.h"

// save-input items:
//  0   4
//  1   this pointer
//  2   instanceNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class BufferInputStream : public InputStream
{
public:
    BufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive = false);
    virtual ~BufferInputStream();

    virtual cell    GetSourceID() const;
    virtual char* GetLine(const char* pPrompt);
    virtual char* AddLine();
    virtual bool    IsInteractive(void) { return mIsInteractive; };
	virtual InputStreamType GetType( void ) const;
    virtual const char* GetName(void) const;
    virtual const char* GetReportedBufferBasePointer( void );
 
    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

	// TODO: should this return true?
	//virtual bool	IsGenerated();

protected:
    static const int kNumStateMembers = 8;

    static int      sInstanceNumber;    // used for checking consistency in restore-input

    int             mInstanceNumber;    // used for checking consistency in restore-input
    const char      *mpSourceBuffer;
    cell            mState[kNumStateMembers];
	bool			mIsInteractive;
};


