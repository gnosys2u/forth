#pragma once

//////////////////////////////////////////////////////////////////////
//
// InputStream.h: interface for the InputStream class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

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

