#pragma once
//////////////////////////////////////////////////////////////////////
//
// BufferInputStream.h: interface for the BufferInputStream class.
//
//////////////////////////////////////////////////////////////////////

//#include "InputStream.h"
#include "InputStream.h"

// save-input items:
//  0   4
//  1   this pointer
//  2   instanceNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class BufferInputStream : public InputStream
{
public:
    BufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive = true, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~BufferInputStream();

    virtual cell    GetSourceID();
    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return mIsInteractive; };
	virtual const char* GetType( void );
    virtual const char* GetReportedBufferBasePointer( void );
 
    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

	// TODO: should this return true?
	//virtual bool	IsGenerated();

protected:
    static const int kNumStateMembers = 8;

    static int      sInstanceNumber;    // used for checking consistency in restore-input

    int             mInstanceNumber;    // used for checking consistency in restore-input
    const char      *mpSourceBuffer;
    char			*mpDataBuffer;
    char			*mpDataBufferBase;
    char			*mpDataBufferLimit;
    cell            mState[kNumStateMembers];
	bool			mIsInteractive;
};


