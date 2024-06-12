//////////////////////////////////////////////////////////////////////
//
// BufferInputStream.cpp: implementation of the BufferInputStream class.
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

#include "pch.h"
#include "BufferInputStream.h"
#include "Engine.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     BufferInputStream
// 

// to be compliant with the ANSI Forth standard we have to:
//
// 1) allow the original input buffer to not be null terminated
// 2) return the original input buffer pointer when queried
//
// so we make a copy of the original buffer with a null terminator,
// but we return the original buffer pointer when queried

int BufferInputStream::sInstanceNumber = 0;    // used for checking consistency in restore-input

BufferInputStream::BufferInputStream( const char *pSourceBuffer, int sourceBufferLen, bool isInteractive )
: InputStream(sourceBufferLen)
, mIsInteractive(isInteractive)
, mpSourceBuffer(pSourceBuffer)
{
	SPEW_SHELL("BufferInputStream %s  {%s}\n", GetName(), pSourceBuffer);
    if (pSourceBuffer != nullptr)
    {
        memcpy(mpBufferBase, pSourceBuffer, sourceBufferLen);
        mpBufferBase[sourceBufferLen] = '\0';
        mWriteOffset = sourceBufferLen;
    }
    mInstanceNumber = sInstanceNumber++;
    for (int i = 0; i < kNumStateMembers; ++i)
    {
        mState[i] = 0;
    }
}

BufferInputStream::~BufferInputStream()
{
}

cell BufferInputStream::GetSourceID() const
{
    return -1;
}

InputStreamType BufferInputStream::GetType( void ) const
{
    return InputStreamType::kBuffer;
}

char* BufferInputStream::GetLine(const char* pPrompt)
{
    if (mbForcedEmpty)
    {
        return nullptr;
    }

    char* pBuffer = mpBufferBase + mReadOffset;

    SPEW_SHELL("BufferInputStream::GetLine %s  {%s}\n", GetName(), pBuffer);
    if (mReadOffset >= mBufferLen)
    {
        return nullptr;
    }
    
    int ix = 0;
    while (ix < mBufferLen)
    {
        char c = mpBufferBase[ix++];
        if (c == '\n')
        {
            // replace the newline with a null to terminate the string
            mpBufferBase[ix - 1] = '\0';
            break;
        }
        else if (c == '\0')
        {
            break;
        }
    }

    return (mReadOffset == mBufferLen && pBuffer[0] == '\0') ? nullptr : pBuffer;
}

char* BufferInputStream::AddLine()
{
    // nothing is added to a buffer input stream after it is created
    return nullptr;
}

const char* BufferInputStream::GetName(void) const
{
    return "Buffer";
}

const char * BufferInputStream::GetReportedBufferBasePointer( void )
{
    return mpSourceBuffer;
}

cell* BufferInputStream::GetInputState()
{
    // save-input items:
    //  0   4
    //  1   this pointer
    //  2   mInstanceNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)

    cell* pState = &(mState[0]);
    pState[0] = 4;
    pState[1] = (cell)this;
    pState[2] = mInstanceNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    
    return pState;
}

bool BufferInputStream::SetInputState(cell* pState)
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (cell)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( pState[2] != mInstanceNumber )
    {
        // TODO: report restore-input error - instance number mismatch
        return false;
    }
    if ( mWriteOffset != pState[4] )
    {
        // TODO: report restore-input error - line length doesn't match save-input value
        return false;
    }
    mReadOffset = pState[3];
    return true;
}

