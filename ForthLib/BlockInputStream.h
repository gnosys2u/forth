#pragma once
//////////////////////////////////////////////////////////////////////
//
// BlockInputStream.h: interface for the BlockInputStream class.
//
//////////////////////////////////////////////////////////////////////

//#include "InputStream.h"
#include "InputStream.h"

class BlockFileManager;

// save-input items:
//  0   4
//  1   this pointer
//  2   blockNumber
//  3   readOffset

class BlockInputStream : public InputStream
{
public:
    BlockInputStream(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock);
    virtual ~BlockInputStream();

    virtual cell    GetSourceID();
    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return false; };
	virtual const char* GetType( void );
 
    virtual void    SeekToLineEnd();
    virtual cell    GetBlockNumber();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

    virtual bool	IsFile();


protected:
    bool            ReadBlock();

    BlockFileManager* mpManager;
    uint32_t    mCurrentBlock;
    uint32_t    mLastBlock;
    char			*mpDataBuffer;
    cell            mState[8];
};


