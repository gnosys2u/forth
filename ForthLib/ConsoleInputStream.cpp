//////////////////////////////////////////////////////////////////////
//
// ConsoleInputStream.cpp: implementation of the ConsoleInputStream class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ConsoleInputStream.h"
#include "Engine.h"

#if defined(LINUX) || defined(MACOSX)
#include <readline/readline.h>
#include <readline/history.h>
#endif

//////////////////////////////////////////////////////////////////////
////
///
//                     ConsoleInputStream
// 

ConsoleInputStream::ConsoleInputStream( int bufferLen )
: InputStream(bufferLen)
, mLineNumber(0)
{
}

ConsoleInputStream::~ConsoleInputStream()
{
}


char *
ConsoleInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

    if (pPrompt)
    {
        printf("\n%s ", pPrompt);
    }
#if defined(LINUX) || defined(MACOSX)
    do
    {
        pBuffer = readline("");
    } while (pBuffer == nullptr);
	add_history(pBuffer);
    strncpy(mpBufferBase, pBuffer, mBufferLen);
#else
    pBuffer = fgets(mpBufferBase, mBufferLen - 1, stdin);
#endif

    mReadOffset = 0;
    const char* pEnd = (const char*) memchr( pBuffer, '\0', mBufferLen );
    mWriteOffset = (pEnd == NULL) ? (mBufferLen - 1) : (pEnd - pBuffer);
    mLineNumber++;
    return pBuffer;
}

const char* ConsoleInputStream::GetType( void )
{
    return "Console";
}

const char* ConsoleInputStream::GetName( void )
{
    return "Console";
}

cell ConsoleInputStream::GetSourceID()
{
    return 0;
}

cell* ConsoleInputStream::GetInputState()
{
    // save-input items:
    //  0   4
    //  1   this pointer
    //  2   lineNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)

    cell* pState = &(mState[0]);
    pState[0] = 4;
    pState[1] = (cell)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    
    return &(mState[0]);
}

bool
ConsoleInputStream::SetInputState(cell* pState)
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
    if ( pState[2] != mLineNumber )
    {
        // TODO: report restore-input error - line number mismatch
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


