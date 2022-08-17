//////////////////////////////////////////////////////////////////////
//
// ForthInput.cpp: implementation of the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthInput.h"
#include "ForthEngine.h"
#include "ForthBlockFileManager.h"
#include "ForthParseInfo.h"

#if defined(LINUX) || defined(MACOSX)
#include <readline/readline.h>
#include <readline/history.h>
#endif

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthInputStack
// 

ForthInputStack::ForthInputStack()
{
    mpHead = NULL;
}

ForthInputStack::~ForthInputStack()
{
    // TODO: should we be closing file here?
    while ( mpHead != NULL )
    {
        ForthInputStream *pNextStream = mpHead->mpNext;
        delete mpHead;
        mpHead = pNextStream;
    }

}

void
ForthInputStack::PushInputStream( ForthInputStream *pNewStream )
{
    ForthInputStream *pOldStream;
    
    pOldStream = mpHead;
    mpHead = pNewStream;
    mpHead->mpNext = pOldStream;

    //printf("ForthInputStack::PushInputStream %s  gen:%d   file:%d\n", mpHead->GetType(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    *(ForthEngine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

	SPEW_SHELL("PushInputStream %s:%s\n", pNewStream->GetType(), pNewStream->GetName());
}


bool
ForthInputStack::PopInputStream( void )
{
    ForthInputStream *pNext;

    if ( (mpHead == NULL) || (mpHead->mpNext == NULL) )
    {
        //printf("ForthInputStack::PopInputStream NO MORE STREAMS\n");
        // all done!
        return true;
    }

    //printf("ForthInputStack::PopInputStream %s  gen:%d   file:%d\n", mpHead->GetType(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    pNext = mpHead->mpNext;
	if (mpHead->DeleteWhenEmpty())
	{
		delete mpHead;
	}
    mpHead = pNext;

    *(ForthEngine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

	SPEW_SHELL("PopInputStream %s\n", (mpHead == NULL) ? "NULL" : mpHead->GetType());

    return false;
}


const char *
ForthInputStack::GetLine( const char *pPrompt )
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


const char*
ForthInputStack::GetFilenameAndLineNumber(int& lineNumber)
{
	ForthInputStream *pStream = mpHead;
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

const char *
ForthInputStack::GetBufferPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetBufferPointer();
}


const char *
ForthInputStack::GetBufferBasePointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetBufferBasePointer();
}


int *
ForthInputStack::GetReadOffsetPointer( void )
{
    return (mpHead == NULL) ? NULL : mpHead->GetReadOffsetPointer();
}


int
ForthInputStack::GetBufferLength( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetBufferLength();
}


void
ForthInputStack::SetBufferPointer( const char *pBuff )
{
	if (mpHead != NULL)
    {
        mpHead->SetBufferPointer( pBuff );
    }
}


int
ForthInputStack::GetReadOffset( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetReadOffset();
}


void
ForthInputStack::SetReadOffset( int offset )
{
    if (mpHead == NULL)
    {
        mpHead->SetReadOffset( offset );
    }
}


int
ForthInputStack::GetWriteOffset( void )
{
    return (mpHead == NULL) ? 0 : mpHead->GetWriteOffset();
}


void
ForthInputStack::SetWriteOffset( int offset )
{
    if (mpHead == NULL)
    {
        mpHead->SetWriteOffset( offset );
    }
}


void
ForthInputStack::Reset( void )
{
    // dump all nested input streams
    if ( mpHead != NULL )
    {
        while ( mpHead->mpNext != NULL )
        {
            PopInputStream();
        }
    }
}


bool
ForthInputStack::IsEmpty(void)
{
	return (mpHead == NULL) ? true : mpHead->IsEmpty();
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthInputStream
// 

ForthInputStream::ForthInputStream( int bufferLen )
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


ForthInputStream::~ForthInputStream()
{
    if ( mpBufferBase != NULL )
    {
        __FREE( mpBufferBase );
    }
}

const char *
ForthInputStream::GetBufferPointer(void)
{
    return mpBufferBase + mReadOffset;
}


const char *
ForthInputStream::GetBufferBasePointer( void )
{
    return mpBufferBase;
}


const char *
ForthInputStream::GetReportedBufferBasePointer( void )
{
    return mpBufferBase;
}


int
ForthInputStream::GetBufferLength( void )
{
    return mBufferLen;
}


void
ForthInputStream::SetBufferPointer( const char *pBuff )
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

int*
ForthInputStream::GetReadOffsetPointer( void )
{
    return &mReadOffset;
}


int
ForthInputStream::GetReadOffset( void )
{
    return mReadOffset;
}


void
ForthInputStream::SetReadOffset( int offset )
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


int
ForthInputStream::GetWriteOffset( void )
{
    return mWriteOffset;
}


void
ForthInputStream::SetWriteOffset( int offset )
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


int
ForthInputStream::GetLineNumber( void )
{
    return -1;
}

const char*
ForthInputStream::GetType( void )
{
    return "Base";
}

const char*
ForthInputStream::GetName( void )
{
    return "mysteriousStream";
}

void
ForthInputStream::SeekToLineEnd()
{
    mReadOffset = mWriteOffset;
}

long
ForthInputStream::GetBlockNumber()
{
    return 0;
}

void
ForthInputStream::StuffBuffer( const char* pSrc )
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

void ForthInputStream::PrependString(const char* pSrc)
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

void ForthInputStream::AppendString(const char* pSrc)
{
    int len = strlen(pSrc);
    if (len < (mBufferLen - 1) - mWriteOffset)
    {
        memcpy(mpBufferBase + mWriteOffset, pSrc, len);
        mWriteOffset += len;
        mpBufferBase[mWriteOffset] = '\0';
    }
}

void ForthInputStream::CropCharacters(int numCharacters)
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
ForthInputStream::DeleteWhenEmpty()
{
	return mbDeleteWhenEmpty;
}

void
ForthInputStream::SetDeleteWhenEmpty(bool deleteIt)
{
    mbDeleteWhenEmpty = deleteIt;
}

bool
ForthInputStream::IsEmpty()
{
	return mReadOffset >= mWriteOffset;
}


bool
ForthInputStream::IsGenerated(void)
{
	return false;
}


bool
ForthInputStream::IsFile(void)
{
    return false;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthFileInputStream
// 

ForthFileInputStream::ForthFileInputStream( FILE *pInFile, const char *pFilename, int bufferLen )
: ForthInputStream(bufferLen)
, mpInFile( pInFile )
, mLineNumber( 0 )
, mLineStartOffset( 0 )
{
	mpName = (char *)__MALLOC(strlen(pFilename) + 1);
    strcpy( mpName, pFilename );
}

ForthFileInputStream::~ForthFileInputStream()
{
    // TODO: should we be closing file here?
    if ( mpInFile != NULL )
    {
        fclose( mpInFile );
    }
    __FREE( mpName );
}

const char*
ForthFileInputStream::GetName( void )
{
    return mpName;
}


char *
ForthFileInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

    mLineStartOffset = ftell( mpInFile );

    pBuffer = fgets( mpBufferBase, mBufferLen, mpInFile );

    mReadOffset = 0;
    //printf("%s\n", pBuffer);
    mpBufferBase[ mBufferLen - 1 ] = '\0';
    mWriteOffset = strlen( mpBufferBase );
    if ( mWriteOffset > 0 )
    {
        // trim trailing linefeed if any
        if ( mpBufferBase[ mWriteOffset - 1 ] == '\n' )
        {
            --mWriteOffset;
            mpBufferBase[ mWriteOffset ] = '\0';
        }
    }
    mLineNumber++;
    return pBuffer;
}

int
ForthFileInputStream::GetLineNumber( void )
{
    return mLineNumber;
}


const char*
ForthFileInputStream::GetType( void )
{
    return "File";
}

int
ForthFileInputStream::GetSourceID()
{
    return (int) mpInFile;
}

cell* ForthFileInputStream::GetInputState()
{
    // save-input items:
    //  0   5
    //  1   this pointer
    //  2   lineNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)
    //  5   lineStartOffset

    cell* pState = &(mState[0]);
    pState[0] = 5;
    pState[1] = (int)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    pState[5] = mLineStartOffset;
    
    return pState;
}

bool
ForthFileInputStream::SetInputState( cell* pState)
{
    if ( pState[0] != 5 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( fseek( mpInFile, pState[5], SEEK_SET )  != 0 )
    {
        // TODO: report restore-input error - error seeking to beginning of line
        return false;
    }
    GetLine(NULL);
    if ( mWriteOffset != pState[4] )
    {
        // TODO: report restore-input error - line length doesn't match save-input value
        return false;
    }
    mLineNumber = pState[2];
    mReadOffset = pState[3];
    return true;
}

bool
ForthFileInputStream::IsFile(void)
{
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthConsoleInputStream
// 

ForthConsoleInputStream::ForthConsoleInputStream( int bufferLen )
: ForthInputStream(bufferLen)
, mLineNumber(0)
{
}

ForthConsoleInputStream::~ForthConsoleInputStream()
{
}


char *
ForthConsoleInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

	printf("\n%s ", pPrompt);
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

const char*
ForthConsoleInputStream::GetType( void )
{
    return "Console";
}

const char*
ForthConsoleInputStream::GetName( void )
{
    return "Console";
}

int
ForthConsoleInputStream::GetSourceID()
{
    return 0;
}

cell* ForthConsoleInputStream::GetInputState()
{
    // save-input items:
    //  0   4
    //  1   this pointer
    //  2   lineNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)

    cell* pState = &(mState[0]);
    pState[0] = 4;
    pState[1] = (int)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    
    return &(mState[0]);
}

bool
ForthConsoleInputStream::SetInputState(cell* pState)
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
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



//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBufferInputStream
// 

// to be compliant with the ANSI Forth standard we have to:
//
// 1) allow the original input buffer to not be null terminated
// 2) return the original input buffer pointer when queried
//
// so we make a copy of the original buffer with a null terminator,
// but we return the original buffer pointer when queried

int ForthBufferInputStream::sInstanceNumber = 0;    // used for checking consistency in restore-input

ForthBufferInputStream::ForthBufferInputStream( const char *pSourceBuffer, int sourceBufferLen, bool isInteractive, int bufferLen )
: ForthInputStream(bufferLen)
, mIsInteractive(isInteractive)
, mpSourceBuffer(pSourceBuffer)
{
	SPEW_SHELL("ForthBufferInputStream %s:%s  {%s}\n", GetType(), GetName(), pSourceBuffer);
	mpDataBufferBase = (char *)__MALLOC(sourceBufferLen + 1);
	memcpy( mpDataBufferBase, pSourceBuffer, sourceBufferLen );
    mpDataBufferBase[ sourceBufferLen ] = '\0';
	mpDataBuffer = mpDataBufferBase;
	mpDataBufferLimit = mpDataBuffer + sourceBufferLen;
    mWriteOffset = sourceBufferLen;
    mInstanceNumber = sInstanceNumber++;
}

ForthBufferInputStream::~ForthBufferInputStream()
{
	__FREE(mpDataBufferBase);
}

int
ForthBufferInputStream::GetSourceID()
{
    return -1;
}


char *
ForthBufferInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer = NULL;
    char *pDst, c;

	SPEW_SHELL("ForthBufferInputStream::GetLine %s:%s  {%s}\n", GetType(), GetName(), mpDataBuffer);
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


const char*
ForthBufferInputStream::GetType( void )
{
    return "Buffer";
}


const char *
ForthBufferInputStream::GetReportedBufferBasePointer( void )
{
    return mpSourceBuffer;
}

cell* ForthBufferInputStream::GetInputState()
{
    // save-input items:
    //  0   4
    //  1   this pointer
    //  2   mInstanceNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)

    cell* pState = &(mState[0]);
    pState[0] = 4;
    pState[1] = (int)this;
    pState[2] = mInstanceNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    
    return pState;
}

bool
ForthBufferInputStream::SetInputState(cell* pState)
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
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

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBlockInputStream
// 

ForthBlockInputStream::ForthBlockInputStream(ForthBlockFileManager* pManager, unsigned int firstBlock, unsigned int lastBlock)
:   ForthInputStream( BYTES_PER_BLOCK + 1 )
,   mpManager(pManager)
,   mCurrentBlock( firstBlock )
,   mLastBlock( lastBlock )
{
    mReadOffset = BYTES_PER_BLOCK;
    mWriteOffset = BYTES_PER_BLOCK;
    ReadBlock();
}

ForthBlockInputStream::~ForthBlockInputStream()
{
}

int
ForthBlockInputStream::GetSourceID()
{
    return -1;
}


char *
ForthBlockInputStream::GetLine( const char *pPrompt )
{
    // TODO!
    char* pBuffer = NULL;
    if ( mReadOffset < BYTES_PER_BLOCK )
    {
        pBuffer = mpBufferBase + mReadOffset;
    }
    else
    {
        if ( mCurrentBlock <= mLastBlock )
        {
            if ( ReadBlock() )
            {
                pBuffer = mpBufferBase;
                mReadOffset = 0;
                mWriteOffset = BYTES_PER_BLOCK;
            }
            mCurrentBlock++;
        }
    }
        
    return pBuffer;
}


const char*
ForthBlockInputStream::GetType( void )
{
    return "Block";
}


void
ForthBlockInputStream::SeekToLineEnd()
{
    // TODO! this 
    mReadOffset = (mReadOffset + 64) & 0xFFFFFFC0;
    if ( mReadOffset > BYTES_PER_BLOCK )
    {
        mReadOffset = BYTES_PER_BLOCK;
    }
}


cell* ForthBlockInputStream::GetInputState()
{
    // save-input items:
    //  0   3
    //  1   this pointer
    //  2   blockNumber
    //  3   readOffset

    cell* pState = &(mState[0]);
    pState[0] = 3;
    pState[1] = (int)this;
    pState[2] = mCurrentBlock;
    pState[3] = mReadOffset;
    
    return pState;
}

bool
ForthBlockInputStream::SetInputState(cell* pState)
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (long)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( pState[2] != mCurrentBlock )
    {
        // TODO: report restore-input error - wrong block
        return false;
    }
    mReadOffset = pState[3];
    return true;
}

long
ForthBlockInputStream::GetBlockNumber()
{
    return mCurrentBlock;
}

bool
ForthBlockInputStream::ReadBlock()
{
    bool success = true;
    ForthEngine* pEngine = ForthEngine::GetInstance();
    FILE * pInFile = mpManager->OpenBlockFile(false);
    if ( pInFile == NULL )
    {
        pEngine->SetError( kForthErrorIO, "BlockInputStream - failed to open block file" );
        success = false;
    }
    else
    {
        fseek( pInFile, BYTES_PER_BLOCK * mCurrentBlock, SEEK_SET );
        int numRead = fread( mpBufferBase, BYTES_PER_BLOCK, 1, pInFile );
        if ( numRead != 1 )
        {
            pEngine->SetError( kForthErrorIO, "BlockInputStream - failed to read block file" );
            success = false;
        }
        fclose( pInFile );
    }
    return success;
}

bool
ForthBlockInputStream::IsFile(void)
{
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthExpressionInputStream
// 
#define INITIAL_EXPRESSION_STACK_SIZE 2048

ForthExpressionInputStream::ForthExpressionInputStream()
	: ForthInputStream(INITIAL_EXPRESSION_STACK_SIZE)
	, mStackSize(INITIAL_EXPRESSION_STACK_SIZE)
{
    // expression input streams shouldn't be deleted when empty since they are
    //  used multiple times
    mbDeleteWhenEmpty = false;

	mpStackBase = static_cast<char *>(__MALLOC(mStackSize));
	mpLeftBase = static_cast<char *>(__MALLOC(mStackSize + 1));
	mpRightBase = static_cast<char *>(__MALLOC(mStackSize + 1));
	ResetStrings();
}

void
ForthExpressionInputStream::ResetStrings()
{
	mpStackTop = mpStackBase + mStackSize;
	mpStackCursor = mpStackTop;
	*--mpStackCursor = '\0';
	*--mpStackCursor = '\0';
	mpLeftCursor = mpLeftBase;
	mpLeftTop = mpLeftBase + mStackSize;
	*mpLeftCursor = '\0';
	*mpLeftTop = '\0';
	mpRightCursor = mpRightBase;
	mpRightTop = mpRightCursor + mStackSize;
	*mpRightCursor = '\0';
	*mpRightTop = '\0';
}

ForthExpressionInputStream::~ForthExpressionInputStream()
{
	__FREE(mpStackBase);
	__FREE(mpLeftBase);
	__FREE(mpRightBase);
}

char* topStr = NULL;
char* nextStr = NULL;

//#define LOG_EXPRESSION(STR) SPEW_SHELL("ForthExpressionInputStream::%s L:{%s} R:{%s}  (%s)(%s)\n",\
//	STR, mpLeftBase, mpRightBase, mpStackCursor, (mpStackCursor + strlen(mpStackCursor) + 1))
#define LOG_EXPRESSION(STR)

bool
ForthExpressionInputStream::ProcessExpression(ForthInputStream* pInputStream)
{
	// z	(a,b) -> (a,bz)
	// _	(a,b) -> (ab_,)		underscore represents space, tab or EOL
	// )	(a,b)(c,d)  -> (c,ab_d)
	// (	(a,b) -> (,)(a,b)

	bool result = true;

	int nestingDepth = 0;
	ResetStrings();
	const char* pSrc = pInputStream->GetBufferPointer();
	const char* pNewSrc = pSrc;
	char c;
	char previousChar = '\0';
	bool danglingPeriod = false;	 // to allow ")." at end of line to force continuation to next line
	ForthParseInfo parseInfo((long *)mpBufferBase, mBufferLen);
	ForthEngine* pEngine = ForthEngine::GetInstance();

	bool done = false;
	bool atEOL = false;
	while (!done)
	{
		const char* pSrcLimit = pInputStream->GetBufferBasePointer() + pInputStream->GetWriteOffset();
		if (atEOL || (pSrc >= pSrcLimit))
		{
			if ((nestingDepth != 0) || danglingPeriod)
			{
				// input buffer is empty
				pSrc = pInputStream->GetLine("expression>");
				pSrcLimit = pInputStream->GetBufferBasePointer() + pInputStream->GetWriteOffset();
				// TODO: skip leading whitespace
			}
			else
			{
				done = true;
			}
		}
		if (pSrc != NULL)
		{
			c = *pSrc++;
			//SPEW_SHELL("process character {%c} 0x%x\n", c, c);
			if (c == '\\')
			{
                c = ForthParseInfo::BackslashChar(pSrc);
			}
			pInputStream->SetBufferPointer(pSrc);
			switch (c)
			{
				case ' ':
				case '\t':
					// whitespace completes the token sitting in right string
					if (mpRightCursor != mpRightBase)
					{
						CombineRightIntoLeft();
					}
					done = (nestingDepth == 0);
					break;

				case '\0':
					atEOL = true;
					break;

				case '(':
					PushStrings();
					nestingDepth++;
					break;

				case ')':
					PopStrings();
					nestingDepth--;
					break;

				case '/':
					if (*pSrc == '/')
					{
						// this is an end-of-line comment
						atEOL = true;
					}
					else
					{
						AppendCharToRight(c);
					}
					break;

				case '`':
					if (mpRightCursor != mpRightBase)
					{
						CombineRightIntoLeft();
					}
					pNewSrc = parseInfo.ParseSingleQuote(pSrc - 1, pSrcLimit, pEngine, true);
					if ((pNewSrc == (pSrc - 1)) && ((*pSrc == ' ') || (*pSrc == '\t')))
					{
						// this is tick operator
						AppendCharToRight(c);
						AppendCharToRight(*pSrc++);
						CombineRightIntoLeft();
					}
					else
					{
						AppendCharToRight(c);
						if (parseInfo.GetFlags() & PARSE_FLAG_QUOTED_CHARACTER)
						{
							const char* pChars = (char *)parseInfo.GetToken();
							while (*pChars != '\0')
							{
								char cc = *pChars++;
								/*
								if (cc == ' ')
								{
								// need to prefix spaces in character constants with backslash
								AppendCharToRight('\\');
								}
								*/
								AppendCharToRight(cc);
							}
							pSrc = pNewSrc;
							AppendCharToRight(c);
							if (parseInfo.GetFlags() & PARSE_FLAG_FORCE_LONG)
							{
								AppendCharToRight('L');
							}
							CombineRightIntoLeft();
						}
					}
					break;

				case '\"':
					if (mpRightCursor != mpRightBase)
					{
						CombineRightIntoLeft();
                    }
                    pNewSrc = pSrc - 1;   // point back at the quote
                    parseInfo.ParseDoubleQuote(pNewSrc, pSrcLimit, true);
					if (pNewSrc == (pSrc - 1))
					{
						// TODO: report error
					}
					else
					{
						AppendCharToRight(c);
						AppendStringToRight((char *)parseInfo.GetToken());
						pSrc = pNewSrc;
						AppendCharToRight(c);
						CombineRightIntoLeft();
					}
					break;

				default:
					if ((previousChar == ')') && (c != '.'))
					{
						// this seems hokey, but it fixes cases like "a(b(1)2)" becoming "1 b2 a" instead of "1 b 2 a"
						//   and doesn't break "a(1).b(2)" like some other fixes
						CombineRightIntoLeft();
					}
					AppendCharToRight(c);
					break;
			}
		}
		else
		{
			AppendCharToRight(' ');		// TODO: is this necessary?
			CombineRightIntoLeft();
			done = true;
		}
		danglingPeriod = (previousChar == ')') && (c == '.');
		previousChar = c;
	}

	strcpy(mpBufferBase, mpLeftBase);
	strcat(mpBufferBase, " ");
	strcat(mpBufferBase, mpRightBase);
	mReadOffset = 0;
	mWriteOffset = strlen(mpBufferBase);
	SPEW_SHELL("ForthExpressionInputStream::ProcessExpression  result:{%s}\n", mpBufferBase);
	return result;
}

int
ForthExpressionInputStream::GetSourceID()
{
	return -1;
}

char*
ForthExpressionInputStream::GetLine(const char *pPrompt)
{
	return NULL;
}

const char*
ForthExpressionInputStream::GetType(void)
{
	return "Expression";
}

void
ForthExpressionInputStream::SeekToLineEnd()
{

}

cell* ForthExpressionInputStream::GetInputState()
{
	// TODO: error!
	return nullptr;
}

bool
ForthExpressionInputStream::SetInputState(cell* pState)
{
	// TODO: error!
	return false;
}

void
ForthExpressionInputStream::PushString(char *pString, int numBytes)
{
	char* pNewBase = mpStackCursor - (numBytes + 1);
	if (pNewBase > mpStackBase)
	{
		memcpy(pNewBase, pString, numBytes + 1);
		mpStackCursor = pNewBase;
	}
	else
	{
		// TODO: report stack overflow
	}
}

void
ForthExpressionInputStream::PushStrings()
{
	//SPEW_SHELL("ForthExpressionInputStream::PushStrings  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	PushString(mpRightBase, mpRightCursor - mpRightBase);
	nextStr = mpStackCursor;
	PushString(mpLeftBase, mpLeftCursor - mpLeftBase);
	mpRightCursor = mpRightBase;
	*mpRightCursor = '\0';
	mpLeftCursor = mpLeftBase;
	*mpLeftCursor = '\0';
	LOG_EXPRESSION("PushStrings");
}

void
ForthExpressionInputStream::PopStrings()
{
	// at start, leftString is a, rightString is b, top of string stack is c, next on stack is d
	// (a,b)(c,d)  -> (c,ab_d)   OR  rightString = leftString + rightString + space + stack[1], leftString = stack[0]
	if (mpStackCursor < mpStackTop)
	{
		int lenA = mpLeftCursor - mpLeftBase;
		int lenB = mpRightCursor - mpRightBase;
		int lenStackTop = strlen(mpStackCursor);
		char* pStackNext = mpStackCursor + lenStackTop + 1;
		int lenStackNext = strlen(pStackNext);
		// TODO: check that ab_d will fit in right string
		// append right to left
		if (lenB > 0)
		{
			if (lenA > 0)
			{
				*mpLeftCursor++ = ' ';
			}
			memcpy(mpLeftCursor, mpRightBase, lenB + 1);
			mpLeftCursor += lenB;
		}
		// append second stacked string to left
		if (lenStackNext > 0)
		{
			if (mpLeftCursor != mpLeftBase)
			{
				*mpLeftCursor++ = ' ';
			}
			memcpy(mpLeftCursor, pStackNext, lenStackNext + 1);
			mpLeftCursor += lenStackNext;
		}
		lenA = mpLeftCursor - mpLeftBase;
		if (lenA > 0)
		{
			memcpy(mpRightBase, mpLeftBase, lenA + 1);
		}
		mpRightCursor = mpRightBase + lenA;
		//AppendCharToRight(' ');

		// copy top of stack to left
		memcpy(mpLeftBase, mpStackCursor, lenStackTop + 1);
		mpLeftCursor = mpLeftBase + lenStackTop;

		// remove both strings from stack
		mpStackCursor = pStackNext + lenStackNext + 1;
		// TODO: check that stack cursor is not above stackTop
		LOG_EXPRESSION("PopStrings");
		//SPEW_SHELL("ForthExpressionInputStream::PopStrings  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report pop of empty stack
	}
	nextStr = mpStackCursor + strlen(mpStackCursor) + 1;
}

void
ForthExpressionInputStream::AppendStringToRight(const char* pString)
{
	int len = strlen(pString);
	if ((mpRightCursor + len) < mpRightTop)
	{
		memcpy(mpRightCursor, pString, len + 1);
		mpRightCursor += len;
		LOG_EXPRESSION("AppendStringToRight");
		//SPEW_SHELL("ForthExpressionInputStream::AppendStringToRight  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report right string overflow
	}
}


void
ForthExpressionInputStream::AppendCharToRight(char c)
{
	if (mpRightCursor < mpRightTop)
	{
		*mpRightCursor++ = c;
		*mpRightCursor = '\0';
		LOG_EXPRESSION("AppendCharToRight");
		//SPEW_SHELL("ForthExpressionInputStream::AppendCharToRight  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report right string overflow
	}
}

void
ForthExpressionInputStream::CombineRightIntoLeft()
{
	int spaceRemainingInLeft = mpLeftTop - mpLeftCursor;
	int rightLen = mpRightCursor - mpRightBase;
	if (spaceRemainingInLeft > rightLen)
	{
		if (mpLeftCursor != mpLeftBase)
		{
			*mpLeftCursor++ = ' ';
		}
		memcpy(mpLeftCursor, mpRightBase, rightLen + 1);
		mpLeftCursor += rightLen;
	}
	mpRightCursor = mpRightBase;
	*mpRightCursor = '\0';
	LOG_EXPRESSION("CombineRightIntoLeft");
	//SPEW_SHELL("ForthExpressionInputStream::CombineRightIntoLeft  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
}


bool
ForthExpressionInputStream::IsGenerated()
{
	return true;
}
