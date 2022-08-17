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
    char*           GetBlock( unsigned int blockNum, bool readContents );
    void            UpdateCurrentBuffer();
    void            SaveBuffers( bool unassignAfterSaving );
    void            EmptyBuffers();
    FILE*           OpenBlockFile( bool forWrite = false );
    unsigned int    GetNumBlocksInFile();
    inline long*    GetBlockPtr() { return &mBlockNumber; };
    unsigned int    GetBytesPerBlock() const;
    unsigned int    GetNumBuffers() const;

private:

    unsigned int    AssignBuffer( unsigned int blockNum, bool readContents );
    void            UpdateLRU();
    bool            SaveBuffer( unsigned int bufferNum );
    void            ReportError( eForthError errorCode, const char* pErrorMessage );

    char*           mpBlockFilename;
    unsigned int    mNumBlocksInFile;
    unsigned int    mNumBuffers;
    unsigned int    mCurrentBuffer;
    unsigned int*   mLRUBuffers;
    unsigned int*   mAssignedBlocks;
    char*           mpBlocks;
    bool*           mUpdatedBlocks;
    long            mBlockNumber;       // number returned by 'blk'
    unsigned int    mBytesPerBlock;
};

namespace OBlockFile
{
    void AddClasses(ForthEngine* pEngine);
} // namespace OBlockFile
