//////////////////////////////////////////////////////////////////////
//
// ForthInput.cpp: implementation of the InputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Input.h"
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
//                     InputStack
// 

InputStack::InputStack()
{
    mpHead = NULL;
}

InputStack::~InputStack()
{
    // TODO: should we be closing file here?
    while ( mpHead != NULL )
    {
        InputStream *pNextStream = mpHead->mpNext;
        delete mpHead;
        mpHead = pNextStream;
    }

}

void
InputStack::PushInputStream( InputStream *pNewStream )
{
    InputStream *pOldStream;
    
    pOldStream = mpHead;
    mpHead = pNewStream;
    mpHead->mpNext = pOldStream;

    //printf("InputStack::PushInputStream %s  gen:%d   file:%d\n", mpHead->GetType(),
    //    mpHead->IsGenerated(), mpHead->IsFile());
    *(Engine::GetInstance()->GetBlockFileManager()->GetBlockPtr()) = mpHead->GetBlockNumber();

	SPEW_SHELL("PushInputStream %s:%s\n", pNewStream->GetType(), pNewStream->GetName());
}


bool
InputStack::PopInputStream( void )
{
    InputStream *pNext;

    if ( (mpHead == NULL) || (mpHead->mpNext == NULL) )
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


const char *
InputStack::GetLine( const char *pPrompt )
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
InputStack::GetFilenameAndLineNumber(int& lineNumber)
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
        while ( mpHead->mpNext != NULL )
        {
            PopInputStream();
        }
    }
}


bool InputStack::IsEmpty(void)
{
	return (mpHead == NULL) ? true : mpHead->IsEmpty();
}


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


//////////////////////////////////////////////////////////////////////
////
///
//                     FileInputStream
// 

FileInputStream::FileInputStream( FILE *pInFile, const char *pFilename, int bufferLen )
: InputStream(bufferLen)
, mpInFile( pInFile )
, mLineNumber( 0 )
, mLineStartOffset( 0 )
{
	mpName = (char *)__MALLOC(strlen(pFilename) + 1);
    strcpy( mpName, pFilename );
}

FileInputStream::~FileInputStream()
{
    // TODO: should we be closing file here?
    if ( mpInFile != NULL )
    {
        fclose( mpInFile );
    }
    __FREE( mpName );
}

const char* FileInputStream::GetName( void )
{
    return mpName;
}


char * FileInputStream::GetLine( const char *pPrompt )
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

cell FileInputStream::GetLineNumber(void)
{
    return mLineNumber;
}


const char* FileInputStream::GetType( void )
{
    return "File";
}

cell FileInputStream::GetSourceID()
{
    return static_cast<int>(reinterpret_cast<intptr_t>(mpInFile));
}

cell* FileInputStream::GetInputState()
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
    pState[1] = (cell)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    pState[5] = mLineStartOffset;
    
    return pState;
}

bool FileInputStream::SetInputState( cell* pState)
{
    if ( pState[0] != 5 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (cell)this )
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

bool FileInputStream::IsFile(void)
{
    return true;
}

const std::string& FileInputStream::GetSavedWorkDir() const
{
    return mSavedDir;
}

void FileInputStream::SetSavedWorkDir(const std::string& s)
{
    mSavedDir = s;
}

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

//////////////////////////////////////////////////////////////////////
////
///
//                     BlockInputStream
// 

BlockInputStream::BlockInputStream(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock)
:   InputStream( BYTES_PER_BLOCK + 1 )
,   mpManager(pManager)
,   mCurrentBlock( firstBlock )
,   mLastBlock( lastBlock )
{
    mReadOffset = BYTES_PER_BLOCK;
    mWriteOffset = BYTES_PER_BLOCK;
    ReadBlock();
}

BlockInputStream::~BlockInputStream()
{
}

cell BlockInputStream::GetSourceID()
{
    return -1;
}


char * BlockInputStream::GetLine( const char *pPrompt )
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


const char* BlockInputStream::GetType( void )
{
    return "Block";
}


void BlockInputStream::SeekToLineEnd()
{
    // TODO! this 
    mReadOffset = (mReadOffset + 64) & 0xFFFFFFC0;
    if ( mReadOffset > BYTES_PER_BLOCK )
    {
        mReadOffset = BYTES_PER_BLOCK;
    }
}


cell* BlockInputStream::GetInputState()
{
    // save-input items:
    //  0   3
    //  1   this pointer
    //  2   blockNumber
    //  3   readOffset

    cell* pState = &(mState[0]);
    pState[0] = 3;
    pState[1] = (cell)this;
    pState[2] = mCurrentBlock;
    pState[3] = mReadOffset;
    
    return pState;
}

bool
BlockInputStream::SetInputState(cell* pState)
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
    if ( pState[2] != mCurrentBlock )
    {
        // TODO: report restore-input error - wrong block
        return false;
    }
    mReadOffset = pState[3];
    return true;
}

cell BlockInputStream::GetBlockNumber()
{
    return mCurrentBlock;
}

bool BlockInputStream::ReadBlock()
{
    bool success = true;
    Engine* pEngine = Engine::GetInstance();
    FILE * pInFile = mpManager->OpenBlockFile(false);
    if ( pInFile == NULL )
    {
        pEngine->SetError( ForthError::kIO, "BlockInputStream - failed to open block file" );
        success = false;
    }
    else
    {
        fseek( pInFile, BYTES_PER_BLOCK * mCurrentBlock, SEEK_SET );
        int numRead = fread( mpBufferBase, BYTES_PER_BLOCK, 1, pInFile );
        if ( numRead != 1 )
        {
            pEngine->SetError( ForthError::kIO, "BlockInputStream - failed to read block file" );
            success = false;
        }
        fclose( pInFile );
    }
    return success;
}

bool BlockInputStream::IsFile(void)
{
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ExpressionInputStream
// 
#define INITIAL_EXPRESSION_STACK_SIZE 2048

ExpressionInputStream::ExpressionInputStream()
	: InputStream(INITIAL_EXPRESSION_STACK_SIZE)
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
ExpressionInputStream::ResetStrings()
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

ExpressionInputStream::~ExpressionInputStream()
{
	__FREE(mpStackBase);
	__FREE(mpLeftBase);
	__FREE(mpRightBase);
}

char* topStr = NULL;
char* nextStr = NULL;

#if 0
#define LOG_EXPRESSION(STR) SPEW_SHELL("ExpressionInputStream::%s L:{%s} R:{%s}  (%s)(%s)\n",\
	STR, mpLeftBase, mpRightBase, mpStackCursor, (mpStackCursor + strlen(mpStackCursor) + 1))
#endif
#define LOG_EXPRESSION(STR)

bool
ExpressionInputStream::ProcessExpression(InputStream* pInputStream)
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
    char c = 0;
	char previousChar = '\0';
	bool danglingPeriod = false;	 // to allow ")." at end of line to force continuation to next line
	ParseInfo parseInfo((int32_t *)mpBufferBase, mBufferLen);
	Engine* pEngine = Engine::GetInstance();

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
                c = ParseInfo::BackslashChar(pSrc);
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
	SPEW_SHELL("ExpressionInputStream::ProcessExpression  result:{%s}\n", mpBufferBase);
	return result;
}

cell ExpressionInputStream::GetSourceID()
{
	return -1;
}

char* ExpressionInputStream::GetLine(const char *pPrompt)
{
	return NULL;
}

const char* ExpressionInputStream::GetType(void)
{
	return "Expression";
}

void ExpressionInputStream::SeekToLineEnd()
{

}

cell* ExpressionInputStream::GetInputState()
{
	// TODO: error!
	return nullptr;
}

bool
ExpressionInputStream::SetInputState(cell* pState)
{
	// TODO: error!
	return false;
}

void
ExpressionInputStream::PushString(char *pString, int numBytes)
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
ExpressionInputStream::PushStrings()
{
	//SPEW_SHELL("ExpressionInputStream::PushStrings  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
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
ExpressionInputStream::PopStrings()
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
		//SPEW_SHELL("ExpressionInputStream::PopStrings  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report pop of empty stack
	}
	nextStr = mpStackCursor + strlen(mpStackCursor) + 1;
}

void
ExpressionInputStream::AppendStringToRight(const char* pString)
{
	int len = strlen(pString);
	if ((mpRightCursor + len) < mpRightTop)
	{
		memcpy(mpRightCursor, pString, len + 1);
		mpRightCursor += len;
		LOG_EXPRESSION("AppendStringToRight");
		//SPEW_SHELL("ExpressionInputStream::AppendStringToRight  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report right string overflow
	}
}


void
ExpressionInputStream::AppendCharToRight(char c)
{
	if (mpRightCursor < mpRightTop)
	{
		*mpRightCursor++ = c;
		*mpRightCursor = '\0';
		LOG_EXPRESSION("AppendCharToRight");
		//SPEW_SHELL("ExpressionInputStream::AppendCharToRight  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report right string overflow
	}
}

void
ExpressionInputStream::CombineRightIntoLeft()
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
	//SPEW_SHELL("ExpressionInputStream::CombineRightIntoLeft  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
}


bool
ExpressionInputStream::IsGenerated()
{
	return true;
}
