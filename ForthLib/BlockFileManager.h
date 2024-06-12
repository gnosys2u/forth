#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBlockFileManager.h: interface for the BlockFileManager class.
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

#ifndef NUM_BLOCK_BUFFERS
#define NUM_BLOCK_BUFFERS 8
#endif
#define BYTES_PER_BLOCK 1024
// we allocate an additional 8 bytes at end of block to give room to add a null
// having a margin of just one byte would cause buffers in mpBlocks to not be well aligned
#define BLOCK_MARGIN 8

class BlockInputStream;
class Engine;
class OuterInterpreter;

class BlockFileManager
{
public:
    BlockFileManager(const char* pBlockFilename, ucell numBuffers = NUM_BLOCK_BUFFERS, ucell bytesPerBlock = BYTES_PER_BLOCK);
    ~BlockFileManager();

    const char*     GetBlockFilename();
    char*           GetBlock( ucell blockNum, bool readContents );
    void            UpdateCurrentBuffer();
    void            SaveBuffers( bool unassignAfterSaving );
    void            EmptyBuffers();
    FILE*           OpenBlockFile( bool forWrite = false );
    ucell    GetNumBlocksInFile();
    inline cell*    GetBlockPtr() { return &mBlockNumber; };
    ucell    GetBytesPerBlock() const;
    ucell    GetNumBuffers() const;

private:

    ucell    AssignBuffer( ucell blockNum, bool readContents );
    void            UpdateLRU();
    bool            SaveBuffer( ucell bufferNum );
    void            ReportError( ForthError errorCode, const char* pErrorMessage );
    inline char*    BufferBase(ucell blockNum) { return &(mpBlocks[blockNum * mBytesBetweenBlocks]); }

    char*           mpBlockFilename;
    ucell    mNumBlocksInFile;
    ucell    mNumBuffers;
    ucell    mCurrentBuffer;
    ucell*   mLRUBuffers;
    ucell*   mAssignedBlocks;
    char*           mpBlocks;
    bool*           mUpdatedBlocks;
    cell            mBlockNumber;       // number returned by 'blk'
    ucell    mBytesPerBlock;
    ucell    mBytesBetweenBlocks;
};

namespace OBlockFile
{
    void AddClasses(OuterInterpreter* pOuter);
} // namespace OBlockFile
