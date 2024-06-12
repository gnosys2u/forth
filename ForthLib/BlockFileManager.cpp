//////////////////////////////////////////////////////////////////////
//
// BlockFileManager.cpp: implementation of the BlockFileManager class.
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

#include "pch.h"
#include "BlockFileManager.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#include "ClassVocabulary.h"
#include "BlockInputStream.h"

// TODO:
//
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
//                     BlockFileManager
// 

#define INVALID_BLOCK_NUMBER ((uint32_t) ~0)
// INVALID_BLOCK_NUMBER is also used to indicate 'no current buffer'


BlockFileManager::BlockFileManager( const char* pBlockFilename , ucell numBuffers, ucell bytesPerBlock )
:   mNumBuffers( numBuffers )
,   mBytesPerBlock(bytesPerBlock)
,   mCurrentBuffer( INVALID_BLOCK_NUMBER )
,   mBlockNumber(INVALID_BLOCK_NUMBER)
,   mNumBlocksInFile( 0 )
,   mBytesBetweenBlocks(bytesPerBlock + BLOCK_MARGIN)
{
    if ( pBlockFilename == NULL )
    {
        pBlockFilename = "_blocks.blk";
    }
	mpBlockFilename = (char *)__MALLOC(strlen(pBlockFilename) + 1);
    strcpy( mpBlockFilename, pBlockFilename );

	mLRUBuffers = (ucell *)__MALLOC(sizeof(ucell) * numBuffers);
	mAssignedBlocks = (ucell *)__MALLOC(sizeof(ucell) * numBuffers);
	mUpdatedBlocks = (bool *)__MALLOC(sizeof(bool) * numBuffers);

	mpBlocks = (char *)__MALLOC(mBytesBetweenBlocks * numBuffers);

    EmptyBuffers();
}

BlockFileManager::~BlockFileManager()
{
    SaveBuffers( false );

    __FREE( mpBlockFilename );
    __FREE( mLRUBuffers );
    __FREE( mAssignedBlocks );
    __FREE( mUpdatedBlocks );
    __FREE( mpBlocks );
}

ucell BlockFileManager::GetNumBlocksInFile()
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

const char* BlockFileManager::GetBlockFilename()
{
    return mpBlockFilename;
}

FILE* BlockFileManager::OpenBlockFile( bool forWrite )
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

char* BlockFileManager::GetBlock( ucell blockNum, bool readContents )
{
    mCurrentBuffer = AssignBuffer( blockNum, readContents );
    UpdateLRU();
    char* pBufferBase = BufferBase(mCurrentBuffer);
    pBufferBase[mBytesPerBlock] = '\0';
    return pBufferBase;
}

void BlockFileManager::UpdateCurrentBuffer()
{
    if ( mCurrentBuffer > mNumBuffers )
    {
        ReportError( ForthError::invalidParameter, "UpdateCurrentBuffer - no current buffer" );
        return;
    }
    SPEW_IO( "BlockFileManager::UpdateCurrentBuffer setting update flag of buffer %d (block %d)\n", mCurrentBuffer, mAssignedBlocks[mCurrentBuffer]);
    mUpdatedBlocks[ mCurrentBuffer ] = true;
}

bool BlockFileManager::SaveBuffer( ucell bufferNum )
{
    FILE* pBlockFile = OpenBlockFile( true );
    if ( pBlockFile == NULL )
    {
        ReportError( ForthError::openFile, "SaveBuffer - failed to open block file" );
        return false;
    }

    if ( bufferNum > mNumBuffers )
    {
        ReportError( ForthError::invalidParameter, "SaveBuffer - invalid buffer number" );
        return false;
    }
    
    if ( mAssignedBlocks[bufferNum] == INVALID_BLOCK_NUMBER )
    {
        ReportError( ForthError::invalidParameter, "SaveBuffer - buffer wasn't assigned to a block" );
        return false;
    }

    SPEW_IO("BlockFileManager::SaveBuffer writing block %d from buffer %d\n", mAssignedBlocks[bufferNum], bufferNum );
    char* pBufferBase = BufferBase(bufferNum);
    fseek( pBlockFile, (long)(mBytesPerBlock * mAssignedBlocks[bufferNum]), SEEK_SET );
    size_t numWritten = fwrite(pBufferBase, mBytesPerBlock, 1, pBlockFile );
    if ( numWritten != 1 )
    {
        ReportError( ForthError::writeFile, "SaveBuffer - failed to write block file" );
        return false;
    }
    fclose( pBlockFile );

    mUpdatedBlocks[bufferNum] = false;
    return true;
}

ucell BlockFileManager::AssignBuffer( ucell blockNum, bool readContents )
{
    SPEW_IO( "BlockFileManager::AssignBuffer to block %d\n", blockNum );
    ucell availableBuffer = INVALID_BLOCK_NUMBER;
    for (ucell i = 0; i < mNumBuffers; ++i )
    {
        ucell bufferBlockNum = mAssignedBlocks[i];
        if ( bufferBlockNum == blockNum )
        {
            return i;
        }
        else if (bufferBlockNum == INVALID_BLOCK_NUMBER)
        {
            availableBuffer = i;
        }
    }

    // block is not in a buffer, assign it one
    if ( availableBuffer >= mNumBuffers )
    {
        // there are no unassigned buffers, assign the least recently used one
        availableBuffer = mLRUBuffers[ mNumBuffers - 1 ];
        SPEW_IO( "BlockFileManager::AssignBuffer assign LRU buffer %d to block %d\n", availableBuffer, blockNum);
    }
    else
    {
        SPEW_IO( "BlockFileManager::AssignBuffer using unassigned buffer %d\n", availableBuffer );
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
            ReportError( ForthError::openFile, "AssignBuffer - failed to open block file" );
        }
        else
        {
            SPEW_IO("BlockFileManager::AssignBuffer reading block %d into buffer %d\n", blockNum, availableBuffer );
            fseek( pInFile, mBytesPerBlock * blockNum, SEEK_SET );
            char* pBufferBase = BufferBase(availableBuffer);
            size_t numRead = fread(pBufferBase, mBytesPerBlock, 1, pInFile);
            if ( numRead != 1 )
            {
                ReportError( ForthError::readFile, "AssignBuffer - failed to read block file" );
            }
            fclose( pInFile );
        }
    }

    return availableBuffer;
}

void BlockFileManager::UpdateLRU()
{
    SPEW_IO( "BlockFileManager::UpdateLRU current=%d\n", mCurrentBuffer );
    if ( mCurrentBuffer < mNumBuffers )
    {
        for ( ucell i = 0; i < mNumBuffers; ++i )
        {
            if ( mLRUBuffers[i] == mCurrentBuffer )
            {
                if ( i != 0 )
                {
                    for (ucell j = i; j != 0; j--)
                    {
                        mLRUBuffers[j] = mLRUBuffers[j - 1];
                    }
                    mLRUBuffers[0] = mCurrentBuffer;
                }
            }
        }
    }
}

void BlockFileManager::SaveBuffers( bool unassignAfterSaving )
{
    SPEW_IO( "BlockFileManager::SaveBuffers\n" );
    FILE* pOutFile = OpenBlockFile( true );
    if ( pOutFile == NULL )
    {
        // TODO: report error
        return;
    }

    for ( ucell i = 0; i < mNumBuffers; ++i )
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
BlockFileManager::EmptyBuffers()
{
    SPEW_IO( "BlockFileManager::EmptyBuffers - unassigning all buffers\n" );
    for ( ucell i = 0; i < mNumBuffers; ++i )
    {
        mLRUBuffers[i] = i;
        mAssignedBlocks[i] = INVALID_BLOCK_NUMBER;
        mUpdatedBlocks[i] = false;
    }
    mCurrentBuffer = INVALID_BLOCK_NUMBER;
}

void
BlockFileManager::ReportError( ForthError errorCode, const char* pErrorMessage )
{
    Engine::GetInstance()->SetError( errorCode, pErrorMessage );
}

ucell BlockFileManager::GetBytesPerBlock() const
{
    return mBytesPerBlock;
}

ucell BlockFileManager::GetNumBuffers() const
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
        REFCOUNTER              refCount;
        BlockFileManager*  pManager;
    };


    FORTHOP(oBlockFileNew)
    {
        ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
        ALLOCATE_OBJECT(oBlockFileStruct, pBlockFile, pClassVocab);
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
        pBlockFile->pManager = new BlockFileManager(pBlockFileName, numBuffers, bytesPerBlock);
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
        char* pBlock = pBlockFile->pManager->GetBlock((ucell)SPOP, true);
        SPUSH((cell)(pBlock));
        METHOD_RETURN;
    }

    FORTHOP(oBlockFileBufferMethod)
    {
        GET_THIS(oBlockFileStruct, pBlockFile);
        char* pBlock = pBlockFile->pManager->GetBlock((ucell)SPOP, false);
        SPUSH((cell)(pBlock));
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
        uint32_t lastBlock = (uint32_t)SPOP;
        uint32_t firstBlock = (uint32_t)SPOP;
        Engine* pEngine = GET_ENGINE;
        if (lastBlock < firstBlock)
        {
            pEngine->SetError(ForthError::invalidBlockNumber, "thru - last block less than first block");
        }
        else
        {
            BlockFileManager*  pManager = pBlockFile->pManager;
            if (lastBlock < pManager->GetNumBlocksInFile())
            {
                GET_ENGINE->GetShell()->RunOneStream(new BlockInputStream(pManager, firstBlock, lastBlock));
            }
            else
            {
                pEngine->SetError(ForthError::invalidBlockNumber, "thru - last block beyond end of block file");
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

        METHOD_RET("blk", oBlockFileBlkMethod, RETURNS_NATIVE_PTR(BaseType::kInt)),
        METHOD_RET("block", oBlockFileBlockMethod, RETURNS_NATIVE_PTR(BaseType::kByte)),
        METHOD_RET("buffer", oBlockFileBufferMethod, RETURNS_NATIVE_PTR(BaseType::kByte)),
        METHOD("emptyBuffers", oBlockFileEmptyBuffersMethod),
        METHOD("flush", oBlockFileFlushMethod),
        METHOD("saveBuffers", oBlockFileSaveBuffersMethod),
        METHOD("update", oBlockFileUpdateMethod),
        METHOD("thru", oBlockFileThruMethod),

        METHOD("bytesPerBlock", oBlockFileBytesPerBlockMethod),
        METHOD("numBuffers", oBlockFileNumBuffersMethod),

        MEMBER_VAR("__manager", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),

        // following must be last in table
        END_MEMBERS
    };


    void AddClasses(OuterInterpreter* pOuter)
    {
        pOuter->AddBuiltinClass("Block", kBCIBlockFile, kBCIObject, oBlockFileMembers);
    }

}
