//////////////////////////////////////////////////////////////////////
//
// BlockInputStream.cpp: implementation of the BlockInputStream class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "BlockInputStream.h"
#include "Engine.h"
#include "BlockFileManager.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     BlockInputStream
// 

BlockInputStream::BlockInputStream(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock)
:   InputStream( BYTES_PER_BLOCK + 1 )
,   mpManager(pManager)
,   mCurrentBlock( firstBlock )
,   mLastBlock( lastBlock )
{
    mReadOffset = BYTES_PER_BLOCK;
    mWriteOffset = BYTES_PER_BLOCK;
    mpBufferBase[BYTES_PER_BLOCK] = '\0';
    ReadBlock();
}

BlockInputStream::~BlockInputStream()
{
}

cell BlockInputStream::GetSourceID() const
{
    return -1;
}


char * BlockInputStream::GetLine( const char *pPrompt )
{
    // TODO!
    char* pBuffer = NULL;
    if ( mReadOffset < BYTES_PER_BLOCK )
    {
        pBuffer = mpBufferBase + mReadOffset;
    }
    else
    {
        if ( mCurrentBlock <= mLastBlock )
        {
            if ( ReadBlock() )
            {
                pBuffer = mpBufferBase;
                mReadOffset = 0;
                mWriteOffset = BYTES_PER_BLOCK;
            }
            mCurrentBlock++;
        }
    }
        
    return pBuffer;
}

char* BlockInputStream::AddLine()
{
    // don't have continuation in blocks
    return nullptr;
}



InputStreamType BlockInputStream::GetType( void ) const
{
    return InputStreamType::kBlock;
}


const char* BlockInputStream::GetName(void) const
{
    return "Block";
}

void BlockInputStream::SeekToLineEnd()
{
    // TODO! this 
    mReadOffset = (mReadOffset + 64) & 0xFFFFFFC0;
    if ( mReadOffset > BYTES_PER_BLOCK )
    {
        mReadOffset = BYTES_PER_BLOCK;
    }
}


cell* BlockInputStream::GetInputState()
{
    // save-input items:
    //  0   3
    //  1   this pointer
    //  2   blockNumber
    //  3   readOffset

    cell* pState = &(mState[0]);
    pState[0] = 3;
    pState[1] = (cell)this;
    pState[2] = mCurrentBlock;
    pState[3] = mReadOffset;
    
    return pState;
}

bool
BlockInputStream::SetInputState(cell* pState)
{
    if ( pState[0] != 4 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (cell)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( pState[2] != mCurrentBlock )
    {
        // TODO: report restore-input error - wrong block
        return false;
    }
    mReadOffset = pState[3];
    return true;
}

cell BlockInputStream::GetBlockNumber()
{
    return mCurrentBlock;
}

bool BlockInputStream::ReadBlock()
{
    bool success = true;
    Engine* pEngine = Engine::GetInstance();
    FILE * pInFile = mpManager->OpenBlockFile(false);
    if ( pInFile == NULL )
    {
        pEngine->SetError( ForthError::kIO, "BlockInputStream - failed to open block file" );
        success = false;
    }
    else
    {
        fseek( pInFile, BYTES_PER_BLOCK * mCurrentBlock, SEEK_SET );
        int numRead = fread( mpBufferBase, BYTES_PER_BLOCK, 1, pInFile );
        if ( numRead != 1 )
        {
            pEngine->SetError( ForthError::kIO, "BlockInputStream - failed to read block file" );
            success = false;
        }
        fclose( pInFile );
    }
    return success;
}

bool BlockInputStream::IsFile(void)
{
    return true;
}

