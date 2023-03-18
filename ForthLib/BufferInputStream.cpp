//////////////////////////////////////////////////////////////////////
//
// BufferInputStream.cpp: implementation of the BufferInputStream class.
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

BufferInputStream::BufferInputStream( const char *pSourceBuffer, int sourceBufferLen, bool isInteractive, int bufferLen )
: InputStream(bufferLen)
, mIsInteractive(isInteractive)
, mpSourceBuffer(pSourceBuffer)
{
	SPEW_SHELL("BufferInputStream %s:%s  {%s}\n", GetType(), GetName(), pSourceBuffer);
	mpDataBufferBase = (char *)__MALLOC(sourceBufferLen + 1);
	memcpy( mpDataBufferBase, pSourceBuffer, sourceBufferLen );
    mpDataBufferBase[ sourceBufferLen ] = '\0';
	mpDataBuffer = mpDataBufferBase;
	mpDataBufferLimit = mpDataBuffer + sourceBufferLen;
    mWriteOffset = sourceBufferLen;
    mInstanceNumber = sInstanceNumber++;
    for (int i = 0; i < kNumStateMembers; ++i)
    {
        mState[i] = 0;
    }
}

BufferInputStream::~BufferInputStream()
{
	__FREE(mpDataBufferBase);
}

cell BufferInputStream::GetSourceID()
{
    return -1;
}


char * BufferInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer = NULL;
    char *pDst, c;

	SPEW_SHELL("BufferInputStream::GetLine %s:%s  {%s}\n", GetType(), GetName(), mpDataBuffer);
	if (mpDataBuffer < mpDataBufferLimit)
    {
		pDst = mpBufferBase;
		while ( mpDataBuffer < mpDataBufferLimit )
		{
			c = *mpDataBuffer++;
			if ( (c == '\0') || (c == '\n') || (c == '\r') )
			{
				break;
			} 
			else
			{
				*pDst++ = c;
			}
		}
		*pDst = '\0';

        mReadOffset = 0;
        mWriteOffset = (pDst - mpBufferBase);
		pBuffer = mpBufferBase;
    }

    return pBuffer;
}


const char* BufferInputStream::GetType( void )
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

