//////////////////////////////////////////////////////////////////////
//
// InputStack.cpp: implementation of the InputStack class.
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

    //printf("InputStack::PushInputStream %s  gen:%d   file:%d\n", mpHead->GetName(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    *(Engine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

    SPEW_SHELL("PushInputStream %s\n", pNewStream->GetName());
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

    //printf("InputStack::PopInputStream %s  gen:%d   file:%d\n", mpHead->GetName(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    pNext = mpHead->mpNext;
    if (mpHead->DeleteWhenEmpty())
    {
        delete mpHead;
    }
    mpHead = pNext;

    *(Engine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

    SPEW_SHELL("PopInputStream %s\n", (mpHead == NULL) ? "NULL" : mpHead->GetName());

    return false;
}

ucell InputStack::GetDepth()
{
    ucell depth = 0;
    InputStream* pStream = mpHead;
    while (pStream != nullptr)
    {
        depth++;
        pStream = pStream->mpNext;
    }

    return depth;
}

void InputStack::FlushToDepth(ucell flushDepth)
{
    ucell currentDepth = GetDepth();
    InputStream* pStream = mpHead;
    while (pStream != nullptr && currentDepth > flushDepth)
    {
        pStream->SetForcedEmpty();
        currentDepth--;
        pStream = pStream->mpNext;
    }

}

char * InputStack::GetLine( const char *pPrompt )
{
    char *pBuffer, *pEndLine;

    if ( mpHead == NULL )
    {
        return NULL;
    }

    pBuffer = mpHead->GetLine( pPrompt );
    mpHead->TrimLine();

    return pBuffer;
}

char* InputStack::Refill()
{
    char* pBuffer, * pEndLine;

    if (mpHead == NULL)
    {
        return NULL;
    }

    pBuffer = mpHead->Refill();
    mpHead->TrimLine();

    return pBuffer;
}

char* InputStack::AddLine()
{
    char* result = nullptr;
    if (mpHead != nullptr)
    {
        result = mpHead->AddLine();
        if (result != nullptr)
        {
            // get rid of the trailing linefeed (if any)
            char* pEndLine = strchr(result, '\n');
            if (pEndLine)
            {
                *pEndLine = '\0';
            }
#if defined(LINUX) || defined(MACOSX)
            pEndLine = strchr(result, '\r');
            if (pEndLine)
            {
                *pEndLine = '\0';
            }
#endif
        }
    }
    return result;
}

// return false IFF buffer has less than numChars available
bool InputStack::Shorten(int numChars)
{
    bool result = false;
    if (mpHead != nullptr)
    {
        result = mpHead->Shorten(numChars);
    }

    return result;
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

const char * InputStack::GetReadPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetReadPointer();
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


void InputStack::SetReadPointer( const char *pBuff )
{
	if (mpHead != NULL)
    {
        mpHead->SetReadPointer( pBuff );
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


