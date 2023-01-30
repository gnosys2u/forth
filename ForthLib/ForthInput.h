#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthInput.h: interface for the InputStack class.
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

class InputStack;
class ParseInfo;
class BlockFileManager;

class InputStream
{
public:
    InputStream( int bufferLen );
    virtual ~InputStream();

    virtual char    *GetLine( const char *pPrompt ) = 0;

    virtual const char* GetBufferPointer( void );
    virtual const char* GetBufferBasePointer( void );
    virtual const char* GetReportedBufferBasePointer( void );
    virtual cell*       GetReadOffsetPointer( void );
    virtual cell        GetReadOffset( void );
    virtual void        SetReadOffset( int offset );
    virtual cell        GetWriteOffset( void );
    virtual void        SetWriteOffset( int offset );
	virtual bool	    IsEmpty();
    virtual bool	    IsGenerated();
    virtual bool	    IsFile();

    virtual cell        GetBufferLength( void );
    virtual void        SetBufferPointer( const char *pBuff );
    virtual bool        IsInteractive( void ) = 0;
    virtual cell        GetLineNumber( void );
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    
    virtual cell        GetSourceID() = 0;		// for the 'source' ansi forth op
    virtual void        SeekToLineEnd();
    virtual cell        GetBlockNumber();

    virtual cell*       GetInputState() = 0;
    virtual bool        SetInputState(cell* pState) = 0;

    virtual void        StuffBuffer(const char* pSrc);
    virtual void        PrependString(const char* pSrc);
    virtual void        AppendString(const char* pSrc);
    virtual void        CropCharacters(cell numCharacters);

	virtual bool	    DeleteWhenEmpty();
    virtual void        SetDeleteWhenEmpty(bool deleteIt);

    friend class InputStack;

protected:
    InputStream    *mpNext;
    cell                mReadOffset;
    cell                mWriteOffset;
    char                *mpBufferBase;
    cell                mBufferLen;
    bool                mbDeleteWhenEmpty;
};


// save-input items:
//  0   5
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)
//  5   lineStartOffset

class FileInputStream : public InputStream
{
public:
    FileInputStream( FILE *pInFile, const char* pFilename, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~FileInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return false; };
    virtual cell    GetLineNumber( void );
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    virtual cell    GetSourceID();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);
    virtual bool	IsFile();

    // The saved work dir is the current directory before this input stream was started.
    // When a file stream becomes the current input the directory containing that file
    // becomes the current working directory.  When a file input stream is exhausted,
    // the current working directory is set to its saved work directory
    virtual const std::string& GetSavedWorkDir() const;
    virtual void SetSavedWorkDir(const std::string& s);

protected:
    FILE            *mpInFile;
    char*           mpName;
    cell            mLineNumber;
    uint32_t        mLineStartOffset;
    cell            mState[8];
    std::string     mSavedDir;
};

// save-input items:
//  0   4
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class ConsoleInputStream : public InputStream
{
public:
    ConsoleInputStream( int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ConsoleInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return true; };
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    virtual cell      GetSourceID();

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

class BufferInputStream : public InputStream
{
public:
    BufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive = true, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~BufferInputStream();

    virtual cell    GetSourceID();
    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return mIsInteractive; };
	virtual const char* GetType( void );
    virtual const char* GetReportedBufferBasePointer( void );
 
    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

	// TODO: should this return true?
	//virtual bool	IsGenerated();

protected:
    static const int kNumStateMembers = 8;

    static int      sInstanceNumber;    // used for checking consistency in restore-input

    int             mInstanceNumber;    // used for checking consistency in restore-input
    const char      *mpSourceBuffer;
    char			*mpDataBuffer;
    char			*mpDataBufferBase;
    char			*mpDataBufferLimit;
    cell            mState[kNumStateMembers];
	bool			mIsInteractive;
};


// save-input items:
//  0   4
//  1   this pointer
//  2   blockNumber
//  3   readOffset

class BlockInputStream : public InputStream
{
public:
    BlockInputStream(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock);
    virtual ~BlockInputStream();

    virtual cell    GetSourceID();
    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return false; };
	virtual const char* GetType( void );
 
    virtual void    SeekToLineEnd();
    virtual cell    GetBlockNumber();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

    virtual bool	IsFile();


protected:
    bool            ReadBlock();

    BlockFileManager* mpManager;
    uint32_t    mCurrentBlock;
    uint32_t    mLastBlock;
    char			*mpDataBuffer;
    cell            mState[8];
};


class ExpressionInputStream : public InputStream
{
public:
	ExpressionInputStream();
	virtual ~ExpressionInputStream();

	// returns true IFF expression was processed successfully
	bool ProcessExpression(InputStream* pInputStream);

	virtual cell    GetSourceID();
	virtual char*   GetLine(const char *pPrompt);
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
	ParseInfo*		mpParseInfo;
};


class InputStack
{
public:
    InputStack();
    virtual ~InputStack();

    void                    PushInputStream( InputStream *pStream );
    bool                    PopInputStream();
    void                    Reset( void );
    const char              *GetLine( const char *pPrompt );
    inline InputStream      *Top( void ) { return mpHead; };
	// returns NULL if no filename can be found, else returns name & number of topmost input stream on stack which has info available
	const char*             GetFilenameAndLineNumber(int& lineNumber);

    const char*             GetBufferPointer( void );
    const char*             GetBufferBasePointer( void );
    cell*                   GetReadOffsetPointer( void );
    cell                    GetBufferLength( void );
    void                    SetBufferPointer( const char *pBuff );
    cell                    GetReadOffset( void );
    void                    SetReadOffset( cell offset );
    cell                    GetWriteOffset( void );
    void                    SetWriteOffset(cell offset );
	virtual bool			IsEmpty();

protected:
    InputStream        *mpHead;
};

