//////////////////////////////////////////////////////////////////////
//
// BlockInputStream.cpp: implementation of the BlockInputStream class.
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
,   mNextBlock( firstBlock )
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
        if ( mNextBlock <= mLastBlock )
        {
            if ( ReadBlock() )
            {
                pBuffer = mpBufferBase;
                mReadOffset = 0;
                mWriteOffset = mBufferLen;
            }
            mNextBlock++;
        }
    }
        
    return pBuffer;
}

char* BlockInputStream::Refill()
{
    if (mbForcedEmpty)
    {
        return nullptr;
    }

    char* pBuffer = nullptr;

    if (mNextBlock > mLastBlock)
    {
        // this case happens when a REFILL happens inside a single block loaded by LOAD,
        // in that case mLastBlock == mNextBlock after the start of the LOAD.
        // just force the last block to match the current block
        mLastBlock = mNextBlock;
    }

    if (ReadBlock())
    {
        pBuffer = mpBufferBase;
        mReadOffset = 0;
        mWriteOffset = mBufferLen;
        mNextBlock++;
    }

    return pBuffer;
}

void BlockInputStream::TrimLine()
{
    mWriteOffset = mBufferLen;

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
    pState[2] = mNextBlock - 1;
    pState[3] = mReadOffset;
    
    return pState;
}

bool
BlockInputStream::SetInputState(cell* pState)
{
    if ( pState[0] != 3 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (cell)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if (pState[2] != (mNextBlock - 1))
    {
        uint32_t savedCurrentBlock = mNextBlock;
        mNextBlock = (uint32_t)(pState[2]);
        if (ReadBlock())
        {
            mReadOffset = 0;
            mWriteOffset = mBufferLen;
            mNextBlock++;
        }
        else
        {
            mNextBlock = savedCurrentBlock;
            Engine::GetInstance()->SetError(ForthError::blockReadException, "BlockInputStream - failure in restore-input");
            return false;
        }

    }
    mReadOffset = pState[3];
    return true;
}

cell BlockInputStream::GetBlockNumber()
{
    return mNextBlock;
}

bool BlockInputStream::ReadBlock()
{
    bool success = true;
    Engine* pEngine = Engine::GetInstance();

    mpBufferBase = mpManager->GetBlock(mNextBlock, true);
    if (mpBufferBase != nullptr)
    {
        *(mpManager->GetBlockPtr()) = mNextBlock;
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

