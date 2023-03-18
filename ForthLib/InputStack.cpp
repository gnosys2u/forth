//////////////////////////////////////////////////////////////////////
//
// InputStack.cpp: implementation of the InputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "Engine.h"
#include "InputStack.h"
#include "InputStream.h"
#include "BlockFileManager.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     InputStack
// 

InputStack::InputStack()
{
    mpHead = NULL;
}

InputStack::~InputStack()
{
    // TODO: should we be closing file here?
    while (mpHead != NULL)
    {
        InputStream* pNextStream = mpHead->mpNext;
        delete mpHead;
        mpHead = pNextStream;
    }

}

void InputStack::PushInputStream( InputStream *pNewStream )
{
    InputStream* pOldStream;

    pOldStream = mpHead;
    mpHead = pNewStream;
    mpHead->mpNext = pOldStream;

    //printf("InputStack::PushInputStream %s  gen:%d   file:%d\n", mpHead->GetType(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    *(Engine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

    SPEW_SHELL("PushInputStream %s:%s\n", pNewStream->GetType(), pNewStream->GetName());
}


bool InputStack::PopInputStream( void )
{
    InputStream* pNext;

    if ((mpHead == NULL) || (mpHead->mpNext == NULL))
    {
        //printf("InputStack::PopInputStream NO MORE STREAMS\n");
        // all done!
        return true;
    }

    //printf("InputStack::PopInputStream %s  gen:%d   file:%d\n", mpHead->GetType(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    pNext = mpHead->mpNext;
    if (mpHead->DeleteWhenEmpty())
    {
        delete mpHead;
    }
    mpHead = pNext;

    *(Engine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

    SPEW_SHELL("PopInputStream %s\n", (mpHead == NULL) ? "NULL" : mpHead->GetType());

    return false;
}


const char * InputStack::GetLine( const char *pPrompt )
{
    char *pBuffer, *pEndLine;

    if ( mpHead == NULL )
    {
        return NULL;
    }

    pBuffer = mpHead->GetLine( pPrompt );

    if ( pBuffer != NULL )
    {
        // get rid of the trailing linefeed (if any)
        pEndLine = strchr( pBuffer, '\n' );
        if ( pEndLine )
        {
            *pEndLine = '\0';
        }
#if defined(LINUX) || defined(MACOSX)
        pEndLine = strchr( pBuffer, '\r' );
        if ( pEndLine )
        {
            *pEndLine = '\0';
        }
 #endif
    }

    return pBuffer;
}


const char* InputStack::GetFilenameAndLineNumber(int& lineNumber)
{
	InputStream *pStream = mpHead;
	// find topmost input stream which has line number info, and return that
	// without this, errors in parenthesized expressions never display the line number of the error
	while (pStream != NULL)
	{
		int line = pStream->GetLineNumber();
		if (line > 0)
		{
			lineNumber = line;
			return pStream->GetName();
		}
		pStream = pStream->mpNext;
	}
	return NULL;
}

const char * InputStack::GetBufferPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetBufferPointer();
}


const char * InputStack::GetBufferBasePointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetBufferBasePointer();
}


cell * InputStack::GetReadOffsetPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetReadOffsetPointer();
}


cell InputStack::GetBufferLength( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetBufferLength();
}


void InputStack::SetBufferPointer( const char *pBuff )
{
	if (mpHead != NULL)
    {
        mpHead->SetBufferPointer( pBuff );
    }
}


cell InputStack::GetReadOffset( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetReadOffset();
}


void InputStack::SetReadOffset( cell offset )
{
    if (mpHead == NULL)
    {
        mpHead->SetReadOffset( offset );
    }
}


cell InputStack::GetWriteOffset( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetWriteOffset();
}


void InputStack::SetWriteOffset( cell offset )
{
    if (mpHead == NULL)
    {
        mpHead->SetWriteOffset( offset );
    }
}


void InputStack::Reset( void )
{
    // dump all nested input streams
    if ( mpHead != NULL )
    {
        while ( mpHead->mpNext != nullptr)
        {
            PopInputStream();
        }
    }
}


bool InputStack::IsEmpty(void)
{
	return (mpHead == NULL) ? true : mpHead->IsEmpty();
}


