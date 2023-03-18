#pragma once
//////////////////////////////////////////////////////////////////////
//
// ConsoleInputStream.h: interface for the ConsoleInputStream class.
//
//////////////////////////////////////////////////////////////////////

//#include "InputStream.h"
#include "InputStream.h"

// save-input items:
//  0   4
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)

class ConsoleInputStream : public InputStream
{
public:
    ConsoleInputStream( int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ConsoleInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual bool    IsInteractive(void) { return true; };
	virtual const char* GetType( void );
	virtual const char* GetName( void );
    virtual cell      GetSourceID();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

protected:
    int             mLineNumber;    // number of times GetLine has been called
    cell            mState[8];
};


