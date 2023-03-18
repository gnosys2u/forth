//////////////////////////////////////////////////////////////////////
//
// InputStream.cpp: implementation of the InputStream class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "InputStream.h"
#include "Engine.h"
#include "BlockFileManager.h"
#include "ParseInfo.h"

#if defined(LINUX) || defined(MACOSX)
#include <readline/readline.h>
#include <readline/history.h>
#endif


//////////////////////////////////////////////////////////////////////
////
///
//                     InputStream
// 

InputStream::InputStream( int bufferLen )
: mpNext(NULL)
, mBufferLen(bufferLen)
, mReadOffset(0)
, mWriteOffset(0)
, mbDeleteWhenEmpty(true)
{
    mpBufferBase = (char *)__MALLOC(bufferLen);
    mpBufferBase[0] = '\0';
    mpBufferBase[bufferLen - 1] = '\0';
}


InputStream::~InputStream()
{
    if ( mpBufferBase != NULL )
    {
        __FREE( mpBufferBase );
    }
}

const char * InputStream::GetBufferPointer(void)
{
    return mpBufferBase + mReadOffset;
}


const char * InputStream::GetBufferBasePointer( void )
{
    return mpBufferBase;
}


const char * InputStream::GetReportedBufferBasePointer( void )
{
    return mpBufferBase;
}


cell InputStream::GetBufferLength( void )
{
    return mBufferLen;
}


void InputStream::SetBufferPointer( const char *pBuff )
{
	int offset = pBuff - mpBufferBase;
    if ( (offset < 0) || (offset >= mBufferLen) )
    {
        // TODO: report error!
    }
    else
    {
        mReadOffset = offset;
    }
	//SPEW_SHELL("SetBufferPointer %s:%s  offset %d  {%s}\n", GetType(), GetName(), offset, pBuff);
}

cell* InputStream::GetReadOffsetPointer( void )
{
    return &mReadOffset;
}


cell InputStream::GetReadOffset( void )
{
    return mReadOffset;
}


void
InputStream::SetReadOffset( int offset )
{
    if ( (offset < 0) || (offset >= mBufferLen) )
    {
        // TODO: report error!
    }
    else
    {
        mReadOffset = offset;
    }
    mReadOffset = offset;
}


cell InputStream::GetWriteOffset( void )
{
    return mWriteOffset;
}


void
InputStream::SetWriteOffset( int offset )
{
    if ( (offset < 0) || (offset >= mBufferLen) )
    {
        // TODO: report error!
    }
    else
    {
        mWriteOffset = offset;
    }
    mWriteOffset = offset;
}


cell InputStream::GetLineNumber( void )
{
    return -1;
}

const char* InputStream::GetType( void )
{
    return "Base";
}

const char* InputStream::GetName( void )
{
    return "mysteriousStream";
}

void InputStream::SeekToLineEnd()
{
    mReadOffset = mWriteOffset;
}

cell InputStream::GetBlockNumber()
{
    return 0;
}

void InputStream::StuffBuffer( const char* pSrc )
{
    int len = strlen( pSrc );
    if ( len > (mBufferLen - 1) )
    {
        len = mBufferLen - 1;
    }

    memcpy( mpBufferBase, pSrc, len );
    mpBufferBase[len] = '\0';
    mReadOffset = 0;
    mWriteOffset = len;
}

void InputStream::PrependString(const char* pSrc)
{
    int len = strlen(pSrc);
    if (len < (mBufferLen - 1) - mWriteOffset)
    {
        memmove(mpBufferBase + len, mpBufferBase, mWriteOffset);
        memcpy(mpBufferBase, pSrc, len);
        mWriteOffset += len;
        mpBufferBase[mWriteOffset] = '\0';
    }
}

void InputStream::AppendString(const char* pSrc)
{
    int len = strlen(pSrc);
    if (len < (mBufferLen - 1) - mWriteOffset)
    {
        memcpy(mpBufferBase + mWriteOffset, pSrc, len);
        mWriteOffset += len;
        mpBufferBase[mWriteOffset] = '\0';
    }
}

void InputStream::CropCharacters(cell numCharacters)
{
    if (mWriteOffset >= numCharacters)
    {
        mWriteOffset -= numCharacters;
    }
    else
    {
        mWriteOffset = 0;
    }
    mpBufferBase[mWriteOffset] = '\0';
}


bool
InputStream::DeleteWhenEmpty()
{
	return mbDeleteWhenEmpty;
}

void
InputStream::SetDeleteWhenEmpty(bool deleteIt)
{
    mbDeleteWhenEmpty = deleteIt;
}

bool
InputStream::IsEmpty()
{
	return mReadOffset >= mWriteOffset;
}


bool
InputStream::IsGenerated(void)
{
	return false;
}


bool
InputStream::IsFile(void)
{
    return false;
}

