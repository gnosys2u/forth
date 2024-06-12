#pragma once

//////////////////////////////////////////////////////////////////////
//
// InputStream.h: interface for the InputStream class.
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
    virtual char*       Refill();
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
    // set mWriteOffset and trim off trailing newline if preset
    virtual void        TrimLine();
    void                SetForcedEmpty();
    friend class InputStack;

protected:

    InputStream    *mpNext;
    cell                mReadOffset;
    cell                mWriteOffset;
    char                *mpBufferBase;
    cell                mBufferLen;
    bool                mbDeleteWhenEmpty;
    bool                mbBaseOwnsBuffer;
    bool                mbForcedEmpty;
};

