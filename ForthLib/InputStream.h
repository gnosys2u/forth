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

enum class InputStreamType:ucell
{
    kUnknown,
    kBuffer,
    kConsole,
    kFile,
    kBlock,
    kExpression,
    kServer
};


class InputStack;
class ParseInfo;
class BlockFileManager;

class InputStream
{
public:
    InputStream( int bufferLen );
    virtual ~InputStream();

    virtual char*       GetLine( const char *pPrompt ) = 0;
    // return nullptr if adding line would overflow buffer
    virtual char*       AddLine() = 0;
    // return false IFF buffer has less than numChars available
    virtual bool        Shorten(int numChars);

    virtual const char* GetReadPointer( void );
    virtual const char* GetBufferBasePointer( void );
    virtual const char* GetReportedBufferBasePointer( void );
    virtual cell*       GetReadOffsetPointer( void );
    virtual cell        GetReadOffset( void );
    virtual void        SetReadOffset( int offset );
    virtual cell        GetWriteOffset( void );
    virtual void        SetWriteOffset( int offset );
	virtual bool	    IsEmpty();
    virtual bool	    IsGenerated();

    virtual cell        GetBufferLength( void );
    virtual void        SetReadPointer( const char *pBuff );
    virtual bool        IsInteractive( void ) = 0;
    virtual cell        GetLineNumber( void ) const;
	virtual InputStreamType GetType( void ) const;
	virtual const char* GetName( void ) const;
    
    virtual cell        GetSourceID() const = 0;		// for the 'source' ansi forth op
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
    void                SetForcedEmpty();

    friend class InputStack;

protected:
    // set mWriteOffset and trim off trailing newline if preset
    virtual void        TrimLine();

    InputStream    *mpNext;
    cell                mReadOffset;
    cell                mWriteOffset;
    char                *mpBufferBase;
    cell                mBufferLen;
    bool                mbDeleteWhenEmpty;
    bool                mbBaseOwnsBuffer;
    bool                mbForcedEmpty;
};

