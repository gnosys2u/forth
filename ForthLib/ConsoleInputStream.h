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
    virtual char*   AddLine();
    virtual bool    IsInteractive(void) { return true; };
    virtual InputStreamType GetType(void) const;
    virtual const char* GetName( void ) const;
    virtual cell      GetSourceID() const;

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

protected:
    int             mLineNumber;    // number of times GetLine has been called
    cell            mState[8];
};


