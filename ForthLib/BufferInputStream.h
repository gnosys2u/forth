#pragma once
//////////////////////////////////////////////////////////////////////
//
// BufferInputStream.h: interface for the BufferInputStream class.
//
//////////////////////////////////////////////////////////////////////

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
    BufferInputStream( const char *pDataBuffer, int dataBufferLen, bool isInteractive = false);
    virtual ~BufferInputStream();

    virtual cell    GetSourceID() const;
    virtual char* GetLine(const char* pPrompt);
    virtual char* AddLine();
    virtual bool    IsInteractive(void) { return mIsInteractive; };
	virtual InputStreamType GetType( void ) const;
    virtual const char* GetName(void) const;
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
    cell            mState[kNumStateMembers];
	bool			mIsInteractive;
};


