//////////////////////////////////////////////////////////////////////
//
// ForthBlockFileManager.cpp: implementation of the ForthBlockFileManager class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
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


ForthBlockFileManager::ForthBlockFileManager( const char* pBlockFilename , ucell numBuffers, ucell bytesPerBlock )
:   mNumBuffers( numBuffers )
,   mBytesPerBlock(bytesPerBlock)
,   mCurrentBuffer( INVALID_BLOCK_NUMBER )
,   mNumBlocksInFile( 0 )
{
    if ( pBlockFilename == NULL )
    {
        pBlockFilename = "_blocks.blk";
    }
	mpBlockFilename = (char *)__MALLOC(strlen(pBlockFilename) + 1);
    strcpy( mpBlockFilename, pBlockFilename );

	mLRUBuffers = (unsigned int *)__MALLOC(sizeof(unsigned int) * numBuffers);
	mAssignedBlocks = (unsigned int *)__MALLOC(sizeof(unsigned int) * numBuffers);
	mUpdatedBlocks = (bool *)__MALLOC(sizeof(bool) * numBuffers);

	mpBlocks = (char *)__MALLOC(mBytesPerBlock * numBuffers);

    EmptyBuffers();
}

ForthBlockFileManager::~ForthBlockFileManager()
{
    SaveBuffers( false );

    __FREE( mpBlockFilename );
    __FREE( mLRUBuffers );
    __FREE( mAssignedBlocks );
    __FREE( mUpdatedBlocks );
    __FREE( mpBlocks );
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
                mNumBlocksInFile = ftell( pBlockFile ) / mBytesPerBlock;
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
    return &(mpBlocks[mBytesPerBlock * mCurrentBuffer]);
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

    SPEW_IO( "ForthBlockFileManager::AssignBuffer writing block %d from buffer %d\n", mAssignedBlocks[bufferNum], bufferNum );
    fseek( pBlockFile, mBytesPerBlock * mAssignedBlocks[bufferNum], SEEK_SET );
    size_t numWritten = fwrite( &(mpBlocks[mBytesPerBlock * bufferNum]), mBytesPerBlock, 1, pBlockFile );
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
    SPEW_IO( "ForthBlockFileManager::AssignBuffer to block %d\n", blockNum );
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
        SPEW_IO( "ForthBlockFileManager::AssignBuffer using unassigned buffer %d\n", availableBuffer );
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
            SPEW_IO( "ForthBlockFileManager::AssignBuffer reading block %d into buffer %d\n", blockNum, availableBuffer );
            fseek( pInFile, mBytesPerBlock * blockNum, SEEK_SET );
            size_t numRead = fread( &(mpBlocks[mBytesPerBlock * availableBuffer]), mBytesPerBlock, 1, pInFile );
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
    SPEW_IO( "ForthBlockFileManager::UpdateLRU current=%d\n", mCurrentBuffer );
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
    SPEW_IO( "ForthBlockFileManager::SaveBuffers\n" );
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
    SPEW_IO( "ForthBlockFileManager::EmptyBuffers\n" );
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

unsigned int
ForthBlockFileManager::GetBytesPerBlock() const
{
    return mBytesPerBlock;
}

unsigned int
ForthBlockFileManager::GetNumBuffers() const
{
    return mNumBuffers;
}

namespace OBlockFile
{

    //////////////////////////////////////////////////////////////////////
    ///
    //                 oBlockFile
    //

    struct oBlockFileStruct
    {
        forthop*                pMethods;
        ucell                   refCount;
        ForthBlockFileManager*  pManager;
    };


    FORTHOP(oBlockFileNew)
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        MALLOCATE_OBJECT(oBlockFileStruct, pBlockFile, pClassVocab);
        pBlockFile->pMethods = pClassVocab->GetMethods();
        pBlockFile->refCount = 0;
        pBlockFile->pManager = nullptr;
        PUSH_OBJECT(pBlockFile);
    }

    FORTHOP(oBlockFileDeleteMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        if (pBlockFile->pManager != nullptr)
        {
            delete pBlockFile->pManager;
        }
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileInitMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        ucell bytesPerBlock = (ucell) SPOP;
        if (bytesPerBlock == 0)
        {
            bytesPerBlock = BYTES_PER_BLOCK;
        }
        ucell numBuffers = (ucell) SPOP;
        if (numBuffers == 0)
        {
            numBuffers = NUM_BLOCK_BUFFERS;
        }
        const char* pBlockFileName = (const char *)(SPOP);
        pBlockFile->pManager = new ForthBlockFileManager(pBlockFileName, numBuffers, bytesPerBlock);
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileBlkMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        SPUSH((cell)(pBlockFile->pManager->GetBlockPtr()));
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileBlockMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        SPUSH((cell)(pBlockFile->pManager->GetBlock((unsigned int)SPOP, true)));
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileBufferMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        SPUSH((cell)(pBlockFile->pManager->GetBlock((unsigned int)SPOP, false)));
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileEmptyBuffersMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        pBlockFile->pManager->EmptyBuffers();
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileFlushMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        pBlockFile->pManager->SaveBuffers(true);
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileSaveBuffersMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        pBlockFile->pManager->SaveBuffers(false);
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileUpdateMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        pBlockFile->pManager->UpdateCurrentBuffer();
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileThruMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        unsigned int lastBlock = (unsigned int)SPOP;
        unsigned int firstBlock = (unsigned int)SPOP;
        ForthEngine* pEngine = GET_ENGINE;
        if (lastBlock < firstBlock)
        {
            pEngine->SetError(kForthErrorIO, "thru - last block less than first block");
        }
        else
        {
            ForthBlockFileManager*  pManager = pBlockFile->pManager;
            if (lastBlock < pManager->GetNumBlocksInFile())
            {
                GET_ENGINE->PushInputBlocks(pManager, firstBlock, lastBlock);
            }
            else
            {
                pEngine->SetError(kForthErrorIO, "thru - last block beyond end of block file");
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileBytesPerBlockMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        SPUSH(pBlockFile->pManager->GetBytesPerBlock());
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileNumBuffersMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        SPUSH(pBlockFile->pManager->GetNumBuffers());
        METHOD_RETURN;
    }

    baseMethodEntry oBlockFileMembers[] =
    {
        METHOD("__newOp", oBlockFileNew),
        METHOD("delete", oBlockFileDeleteMethod),

        METHOD_RET("blk", oBlockFileBlkMethod, RETURNS_NATIVE(kBaseTypeInt | kDTIsPtr)),
        METHOD_RET("block", oBlockFileBlockMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD_RET("buffer", oBlockFileBufferMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("emptyBuffers", oBlockFileEmptyBuffersMethod),
        METHOD("flush", oBlockFileFlushMethod),
        METHOD("saveBuffers", oBlockFileSaveBuffersMethod),
        METHOD("update", oBlockFileUpdateMethod),
        METHOD("thru", oBlockFileThruMethod),

        METHOD("bytesPerBlock", oBlockFileBytesPerBlockMethod),
        METHOD("numBuffers", oBlockFileNumBuffersMethod),

        MEMBER_VAR("__manager", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

        // following must be last in table
        END_MEMBERS
    };


    void AddClasses(ForthEngine* pEngine)
    {
        pEngine->AddBuiltinClass("Block", kBCIBlockFile, kBCIObject, oBlockFileMembers);
    }

}