#pragma once
//////////////////////////////////////////////////////////////////////
//
// BlockInputStream.h: interface for the BlockInputStream class.
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

//#include "InputStream.h"
#include "InputStream.h"

class BlockFileManager;

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

    virtual cell    GetSourceID() const;
    virtual char*   GetLine( const char *pPrompt );
    virtual char*   Refill();
    virtual void    TrimLine();
    virtual char*   AddLine();
    virtual bool    IsInteractive(void) { return false; };
	virtual InputStreamType GetType( void ) const;
    virtual const char* GetName(void) const;

    virtual void    SeekToLineEnd();
    virtual cell    GetBlockNumber();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

    virtual bool	IsFile();


protected:
    bool            ReadBlock();

    BlockFileManager*   mpManager;
    uint32_t            mNextBlock;
    uint32_t            mLastBlock;
    cell                mState[8];
};


