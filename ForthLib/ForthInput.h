#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthInput.h: interface for the ForthInputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include <string>

#ifndef DEFAULT_INPUT_BUFFER_LEN
#define DEFAULT_INPUT_BUFFER_LEN MAX_STRING_SIZE
#endif

enum
{
    kInputTypeUnknown,
    kInputTypeBuffer,
    kInputTypeConsole,
    kInputTypeFile,
    kInputTypeBlock
};

class ForthInputStack;
class ForthParseInfo;
class ForthBlockFileManager;

class ForthInputStream
{
public:
    ForthInputStream( int bufferLen );
    virtual ~ForthInputStream();

    virtual char    *GetLine( const char *pPrompt ) = 0;

    virtual const char* GetBufferPointer( void );
    virtual const char* GetBufferBasePointer( void );
    virtual const char* GetReportedBufferBasePointer( void );
    virtual int     *GetReadOffsetPointer( void );
    virtual int     GetReadOffset( void );
    virtual void    SetReadOffset( int offset );
    virtual int     GetWriteOffset( void );
    virtual void    SetWriteOffset( int offset );
	virtual bool	IsEmpty();
    virtual bool	IsGenerated();
    virtual bool	IsFile();

    virtual int     GetBufferLength( void );
    virtual void    SetBufferPointer( const char *pBuff );
    virtual bool    IsInteractive( void ) = 0;
    virtual int     GetLineNumber( void );
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    
    virtual int     GetSourceID() = 0;		// for the 'source' ansi forth op
    virtual void    SeekToLineEnd();
    virtual int32_t    GetBlockNumber();

    virtual cell*   GetInputState() = 0;
    virtual bool    SetInputState(cell* pState) = 0;

    virtual void    StuffBuffer(const char* pSrc);
    virtual void    PrependString(const char* pSrc);
    virtual void    AppendString(const char* pSrc);
    virtual void    CropCharacters(int numCharacters);

	virtual bool	DeleteWhenEmpty();
    virtual void    SetDeleteWhenEmpty(bool deleteIt);

    friend class ForthInputStack;

protected:
    ForthInputStream    *mpNext;
    int                 mReadOffset;
    int                 mWriteOffset;
    char                *mpBufferBase;
    int                 mBufferLen;
    bool                mbDeleteWhenEmpty;
};


// save-input items:
//  0   5
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)
//  5   lineStartOffset

class ForthFileInputStream : public ForthInputStream
{
public:
    ForthFileInputStream( FILE *pInFile, const char* pFilename, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthFileInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return false; };
    virtual int     GetLineNumber( void );
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    virtual int     GetSourceID();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);
    virtual bool	IsFile();

protected:
    FILE            *mpInFile;
    char*           mpName;
    int             mLineNumber;
    uint32_t    mLineStartOffset;
    cell            mState[8];
};

// save-input items:
//  0   4
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class ForthConsoleInputStream : public ForthInputStream
{
public:
    ForthConsoleInputStream( int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthConsoleInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return true; };
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    virtual int     GetSourceID();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

protected:
    int             mLineNumber;    // number of times GetLine has been called
    cell            mState[8];
};


// save-input items:
//  0   4
//  1   this pointer
//  2   instanceNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class ForthBufferInputStream : public ForthInputStream
{
public:
    ForthBufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive = true, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthBufferInputStream();

    virtual int     GetSourceID();
    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return mIsInteractive; };
	virtual const char* GetType( void );
    virtual const char* GetReportedBufferBasePointer( void );
 
    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

	// TODO: should this return true?
	//virtual bool	IsGenerated();

protected:
    static int      sInstanceNumber;    // used for checking consistency in restore-input

    int             mInstanceNumber;    // used for checking consistency in restore-input
    const char      *mpSourceBuffer;
    char			*mpDataBuffer;
    char			*mpDataBufferBase;
    char			*mpDataBufferLimit;
    cell            mState[8];
	bool			mIsInteractive;
};


// save-input items:
//  0   4
//  1   this pointer
//  2   blockNumber
//  3   readOffset

class ForthBlockInputStream : public ForthInputStream
{
public:
    ForthBlockInputStream(ForthBlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock);
    virtual ~ForthBlockInputStream();

    virtual int     GetSourceID();
    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return false; };
	virtual const char* GetType( void );
 
    virtual void    SeekToLineEnd();
    virtual int32_t    GetBlockNumber();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

    virtual bool	IsFile();


protected:
    bool            ReadBlock();

    ForthBlockFileManager* mpManager;
    uint32_t    mCurrentBlock;
    uint32_t    mLastBlock;
    char			*mpDataBuffer;
    cell            mState[8];
};


class ForthExpressionInputStream : public ForthInputStream
{
public:
	ForthExpressionInputStream();
	virtual ~ForthExpressionInputStream();

	// returns true IFF expression was processed successfully
	bool ProcessExpression(ForthInputStream* pInputStream);

	virtual int     GetSourceID();
	virtual char    *GetLine(const char *pPrompt);
    virtual bool    IsInteractive(void) { return false; };
	virtual const char* GetType(void);

	virtual void    SeekToLineEnd();

	virtual cell*   GetInputState();
	virtual bool    SetInputState(cell* pState);

	virtual bool	IsGenerated();

protected:
	void			PushStrings();
	void			PushString(char *pString, int numBytes);
	void			PopStrings();
	void			AppendCharToRight(char c);
	void			AppendStringToRight(const char* pString);
	void			CombineRightIntoLeft();
	void			ResetStrings();
	inline bool		StackEmpty() { return mpStackCursor == mpStackTop; }

	uint32_t		mStackSize;
	char*				mpStackBase;
	char*				mpStackTop;
	char*				mpStackCursor;
	char*				mpLeftBase;
	char*				mpLeftCursor;
	char*				mpLeftTop;
	char*				mpRightBase;
	char*				mpRightCursor;
	char*				mpRightTop;
	ForthParseInfo*		mpParseInfo;
};


class ForthInputStack
{
public:
    ForthInputStack();
    virtual ~ForthInputStack();

    void                    PushInputStream( ForthInputStream *pStream );
    bool                    PopInputStream();
    void                    Reset( void );
    const char              *GetLine( const char *pPrompt );
    inline ForthInputStream *InputStream( void ) { return mpHead; };
	// returns NULL if no filename can be found, else returns name & number of topmost input stream on stack which has info available
	const char*             GetFilenameAndLineNumber(int& lineNumber);

    const char*             GetBufferPointer( void );
    const char*             GetBufferBasePointer( void );
    int                     *GetReadOffsetPointer( void );
    int                     GetBufferLength( void );
    void                    SetBufferPointer( const char *pBuff );
    int                     GetReadOffset( void );
    void                    SetReadOffset( int );
    int                     GetWriteOffset( void );
    void                    SetWriteOffset( int offset );
	virtual bool			IsEmpty();

protected:
    ForthInputStream        *mpHead;
};

