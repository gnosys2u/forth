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
    char*           GetBlock( uint32_t blockNum, bool readContents );
    void            UpdateCurrentBuffer();
    void            SaveBuffers( bool unassignAfterSaving );
    void            EmptyBuffers();
    FILE*           OpenBlockFile( bool forWrite = false );
    uint32_t    GetNumBlocksInFile();
    inline int32_t*    GetBlockPtr() { return &mBlockNumber; };
    uint32_t    GetBytesPerBlock() const;
    uint32_t    GetNumBuffers() const;

private:

    uint32_t    AssignBuffer( uint32_t blockNum, bool readContents );
    void            UpdateLRU();
    bool            SaveBuffer( uint32_t bufferNum );
    void            ReportError( eForthError errorCode, const char* pErrorMessage );

    char*           mpBlockFilename;
    uint32_t    mNumBlocksInFile;
    uint32_t    mNumBuffers;
    uint32_t    mCurrentBuffer;
    uint32_t*   mLRUBuffers;
    uint32_t*   mAssignedBlocks;
    char*           mpBlocks;
    bool*           mUpdatedBlocks;
    int32_t            mBlockNumber;       // number returned by 'blk'
    uint32_t    mBytesPerBlock;
};

namespace OBlockFile
{
    void AddClasses(ForthEngine* pEngine);
} // namespace OBlockFile
