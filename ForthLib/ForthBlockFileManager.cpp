//////////////////////////////////////////////////////////////////////
//
// ForthBlockFileManager.cpp: implementation of the ForthBlockFileManager class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ForthBlockFileManager.h"
#include "ForthEngine.h"

// TODO:
// \
// blk
// block
// buffer
// flush
// load
// save-buffers
// update
//
// empty-buffers
// list
// refill
// scr
// thru
// 

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthBlockFileManager
// 

#define INVALID_BLOCK_NUMBER ((unsigned int) ~0)
// INVALID_BLOCK_NUMBER is also used to indicate 'no current buffer'


ForthBlockFileManager::ForthBlockFileManager( const char* pBlockFilename , unsigned int numBuffers )
:   mNumBuffers( numBuffers )
,   mCurrentBuffer( INVALID_BLOCK_NUMBER )
,   mNumBlocksInFile( 0 )
{
    if ( pBlockFilename == NULL )
    {
        pBlockFilename = "_blocks.blk";
    }
    mpBlockFilename = new char[ strlen( pBlockFilename ) + 1 ];
    strcpy( mpBlockFilename, pBlockFilename );

    mLRUBuffers = new unsigned int[ numBuffers ];
    mAssignedBlocks = new unsigned int[ numBuffers ];
    mUpdatedBlocks = new bool[ numBuffers ];

    mpBlocks = new char[ BYTES_PER_BLOCK * numBuffers ];

    EmptyBuffers();
}

ForthBlockFileManager::~ForthBlockFileManager()
{
    SaveBuffers( false );

    delete [] mpBlockFilename;
    delete [] mLRUBuffers;
    delete [] mAssignedBlocks;
    delete [] mUpdatedBlocks;
    delete [] mpBlocks;
}

unsigned int
ForthBlockFileManager::GetNumBlocksInFile()
{
    if ( mNumBlocksInFile == 0 )
    {
        FILE* pBlockFile = fopen( mpBlockFilename, "rb" );
        if ( pBlockFile != NULL )
        {
            if ( !fseek( pBlockFile, 0, SEEK_END ) )
            {
                mNumBlocksInFile = ftell( pBlockFile ) / BYTES_PER_BLOCK;
            }
            fclose( pBlockFile );
        }
    }
    return mNumBlocksInFile;
}

const char*
ForthBlockFileManager::GetBlockFilename()
{
    return mpBlockFilename;
}

FILE*
ForthBlockFileManager::OpenBlockFile( bool forWrite )
{
    FILE* pBlockFile = NULL;
    if ( forWrite )
    {
        pBlockFile = fopen( mpBlockFilename, "r+b" );
        if ( pBlockFile == NULL )
        {
            pBlockFile = fopen( mpBlockFilename, "w+b" );
        }
    }
    else
    {
        pBlockFile = fopen( mpBlockFilename, "rb" );
    }
    return pBlockFile;
}

char*
ForthBlockFileManager::GetBlock( unsigned int blockNum, bool readContents )
{
    mCurrentBuffer = AssignBuffer( blockNum, readContents );
    UpdateLRU();
    return &(mpBlocks[BYTES_PER_BLOCK * mCurrentBuffer]);
}

void
ForthBlockFileManager::UpdateCurrentBuffer()
{
    if ( mCurrentBuffer > mNumBuffers )
    {
        ReportError( kForthErrorBadParameter, "UpdateCurrentBuffer - no current buffer" );
        return;
    }

    mUpdatedBlocks[ mCurrentBuffer ] = true;
}

bool
ForthBlockFileManager::SaveBuffer( unsigned int bufferNum )
{
    FILE* pBlockFile = OpenBlockFile( true );
    if ( pBlockFile == NULL )
    {
        ReportError( kForthErrorIO, "SaveBuffer - failed to open block file" );
        return false;
    }

    if ( bufferNum > mNumBuffers )
    {
        ReportError( kForthErrorBadParameter, "SaveBuffer - invalid buffer number" );
        return false;
    }
    
    if ( mAssignedBlocks[bufferNum] == INVALID_BLOCK_NUMBER )
    {
        ReportError( kForthErrorBadParameter, "SaveBuffer - buffer wasn't assigned to a block" );
        return false;
    }

    TRACE( "ForthBlockFileManager::AssignBuffer writing block %d from buffer %d\n", mAssignedBlocks[bufferNum], bufferNum );
    fseek( pBlockFile, BYTES_PER_BLOCK * mAssignedBlocks[bufferNum], SEEK_SET );
    size_t numWritten = fwrite( &(mpBlocks[BYTES_PER_BLOCK * bufferNum]), BYTES_PER_BLOCK, 1, pBlockFile );
    if ( numWritten != 1 )
    {
        ReportError( kForthErrorIO, "SaveBuffer - failed to write block file" );
        return false;
    }
    fclose( pBlockFile );

    mUpdatedBlocks[bufferNum] = false;
    return true;
}

unsigned int
ForthBlockFileManager::AssignBuffer( unsigned int blockNum, bool readContents )
{
    TRACE( "ForthBlockFileManager::AssignBuffer to block %d\n", blockNum );
    unsigned int availableBuffer = INVALID_BLOCK_NUMBER;
    for ( unsigned int i = 0; i < mNumBuffers; ++i )
    {
        unsigned int bufferBlockNum = mAssignedBlocks[i];
        if ( bufferBlockNum == blockNum )
        {
            return i;
        }
        else if ( bufferBlockNum == INVALID_BLOCK_NUMBER )
        {
            availableBuffer = i;
        }
    }

    // block is not in a buffer, assign it one
    if ( availableBuffer >= mNumBuffers )
    {
        // there are no unassigned buffers, assign the least recently used one
        availableBuffer = mLRUBuffers[ mNumBuffers - 1 ];
    }
    else
    {
        TRACE( "ForthBlockFileManager::AssignBuffer using unassigned buffer %d\n", availableBuffer );
    }

    if ( mUpdatedBlocks[ availableBuffer ] )
    {
        SaveBuffer( availableBuffer );
    }

    mAssignedBlocks[ availableBuffer ] = blockNum;

    if ( readContents )
    {
        FILE* pInFile = OpenBlockFile( false );
        if ( pInFile == NULL )
        {
            ReportError( kForthErrorIO, "AssignBuffer - failed to open block file" );
        }
        else
        {
            TRACE( "ForthBlockFileManager::AssignBuffer reading block %d into buffer %d\n", blockNum, availableBuffer );
            fseek( pInFile, BYTES_PER_BLOCK * blockNum, SEEK_SET );
            int numRead = fread( &(mpBlocks[BYTES_PER_BLOCK * availableBuffer]), BYTES_PER_BLOCK, 1, pInFile );
            if ( numRead != 1 )
            {
                ReportError( kForthErrorIO, "AssignBuffer - failed to read block file" );
            }
            fclose( pInFile );
        }
    }

    return availableBuffer;
}

void
ForthBlockFileManager::UpdateLRU()
{
    TRACE( "ForthBlockFileManager::UpdateLRU current=%d\n", mCurrentBuffer );
    if ( mCurrentBuffer < mNumBuffers )
    {
        for ( unsigned int i = 0; i < mNumBuffers; ++i )
        {
            if ( mLRUBuffers[i] == mCurrentBuffer )
            {
                if ( i != 0 )
                {
                    for ( int j = i; j != 0; j-- )
                    {
                        mLRUBuffers[j] = mLRUBuffers[j - 1];
                    }
                    mLRUBuffers[0] = mCurrentBuffer;
                }
            }
        }
    }
}

void ForthBlockFileManager::SaveBuffers( bool unassignAfterSaving )
{
    TRACE( "ForthBlockFileManager::SaveBuffers\n" );
    FILE* pOutFile = OpenBlockFile( true );
    if ( pOutFile == NULL )
    {
        // TODO: report error
        return;
    }

    for ( unsigned int i = 0; i < mNumBuffers; ++i )
    {
        if ( mUpdatedBlocks[i] )
        {
            SaveBuffer( i );
        }
        if ( unassignAfterSaving )
        {
            mAssignedBlocks[i] = INVALID_BLOCK_NUMBER;
        }
    }

    if ( unassignAfterSaving )
    {
        mCurrentBuffer = INVALID_BLOCK_NUMBER;
    }

    fclose( pOutFile );
}

void
ForthBlockFileManager::EmptyBuffers()
{
    TRACE( "ForthBlockFileManager::EmptyBuffers\n" );
    for ( unsigned int i = 0; i < mNumBuffers; ++i )
    {
        mLRUBuffers[i] = i;
        mAssignedBlocks[i] = INVALID_BLOCK_NUMBER;
        mUpdatedBlocks[i] = false;
    }
    mCurrentBuffer = INVALID_BLOCK_NUMBER;
}

void
ForthBlockFileManager::ReportError( eForthError errorCode, const char* pErrorMessage )
{
    ForthEngine::GetInstance()->SetError( errorCode, pErrorMessage );
}
