#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBlockFileManager.h: interface for the ForthBlockFileManager class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

#ifndef NUM_BLOCK_BUFFERS
#define NUM_BLOCK_BUFFERS 8
#endif
#define BYTES_PER_BLOCK 1024

class ForthBlockInputStream;
class ForthEngine;

class ForthBlockFileManager
{
public:
    ForthBlockFileManager(const char* pBlockFilename, ucell numBuffers = NUM_BLOCK_BUFFERS, ucell bytesPerBlock = BYTES_PER_BLOCK);
    ~ForthBlockFileManager();

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
};

namespace OBlockFile
{
    void AddClasses(ForthEngine* pEngine);
} // namespace OBlockFile
