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
#include "ForthPortability.h"

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
    mpBufferBase = (char *)__MALLOC(bufferLen + 1);
    mpBufferBase[0] = '\0';
    mpBufferBase[bufferLen] = '\0';
}


InputStream::~InputStream()
{
    if ( mpBufferBase != NULL )
    {
        __FREE( mpBufferBase );
    }
}

// return false IFF buffer has less than numChars available
bool InputStream::Shorten(int numChars)
{
    bool result = false;
    
    if (numChars > 0 && mWriteOffset >= numChars)
    {
        mWriteOffset -= numChars;
        mpBufferBase[mWriteOffset] = '\0';
        result = true;
    }

    return result;
}

// set mWriteOffset and trim off trailing newline if preset
void InputStream::TrimLine()
{
    const char* pEnd = (const char*)memchr(mpBufferBase, '\0', mBufferLen);
    if (pEnd == nullptr)
    {
        mWriteOffset = mBufferLen;
    }
    else
    {
        if (pEnd > mpBufferBase)
        {
            if (pEnd[-1] == '\n')
            {
                pEnd--;
            }
        }
        mWriteOffset = pEnd - mpBufferBase;
    }

    mpBufferBase[mWriteOffset] = '\0';
}

const char * InputStream::GetReadPointer(void)
{
    return mpBufferBase + mReadOffset;
}


const char * InputStream::GetBufferBasePointer( void )
{
    return mpBufferBase;
}


const char * InputStream::GetReportedBufferBasePointer( void )
{
    // this is only used to make 'source' work when used with 'evaluate', in
    // that case BufferInputStream overrides this method to return a pointer to
    // the input buffer which was passed into its constructor.  For all other
    // cases this method is just the same as GetBufferBasePointer
    return mpBufferBase;
}


cell InputStream::GetBufferLength( void )
{
    return mBufferLen;
}


void InputStream::SetReadPointer( const char *pBuff )
{
	int offset = pBuff - mpBufferBase;
    if (offset < 0)
    {
        // TODO: report error!
    }
    else if (offset > mBufferLen)
    {
        mReadOffset = mBufferLen;
    }
    else
    {
        mReadOffset = offset;
    }
	//SPEW_SHELL("SetReadPointer %s  offset %d  {%s}\n", GetName(), offset, pBuff);
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
        char buffer[128];
        SNPRINTF(buffer, sizeof(buffer), "InputStream::SetReadOffset - %d is outside range 0:%d\n",
            offset, mBufferLen);
        Engine::GetInstance()->SetError(ForthError::kIllegalOperation, buffer);
    }
    else
    {
        mReadOffset = offset;
    }
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


cell InputStream::GetLineNumber( void ) const
{
    return -1;
}

InputStreamType InputStream::GetType( void ) const
{
    return InputStreamType::kUnknown;
}

const char* InputStream::GetName( void ) const
{
    return "InputStream base class";
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
    if (len > mBufferLen)
    {
        // buffer is actually mBufferLen+1 chars long
        len = mBufferLen;
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


