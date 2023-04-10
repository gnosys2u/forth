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
:   InputStream(0)
,   mpManager(pManager)
,   mCurrentBlock( firstBlock )
,   mLastBlock( lastBlock )
{
    mBufferLen = mpManager->GetBytesPerBlock();
    mReadOffset = mBufferLen;
    mWriteOffset = mBufferLen;
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
    if (mbForcedEmpty)
    {
        return nullptr;
    }

    char* pBuffer = nullptr;

    if ( mReadOffset < mBufferLen )
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
                mWriteOffset = mBufferLen;
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
    if ( mReadOffset > mBufferLen )
    {
        mReadOffset = mBufferLen;
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

    mpBufferBase = mpManager->GetBlock(mCurrentBlock, true);
    if (mpBufferBase != nullptr)
    {
        *(mpManager->GetBlockPtr()) = mCurrentBlock;
    }
    else
    {
        pEngine->SetError(ForthError::openFile, "BlockInputStream - failed to open block file");
        success = false;
    }

    return success;
}

bool BlockInputStream::IsFile(void)
{
    return true;
}

