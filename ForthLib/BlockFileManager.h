#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBlockFileManager.h: interface for the BlockFileManager class.
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
