#pragma once
//////////////////////////////////////////////////////////////////////
//
// InputStack.h: interface for the InputStack class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

class InputStream;

class InputStack
{
public:
    InputStack();
    virtual ~InputStack();

    void                    PushInputStream( InputStream *pStream );
    bool                    PopInputStream();
    ucell                   GetDepth();
    void                    FlushToDepth(ucell depth);

    void                    Reset( void );
    char*                   GetLine( const char *pPrompt );
    char*                   Refill();
    // return null IFF adding another input line would overflow buffer
    char*                   AddLine();
    // return false IFF buffer has less than numChars available
    bool                    Shorten(int numChars);
    inline InputStream* Top(void) { return mpHead; };
    //inline InputStream* Top(void) { return mStack.empty() ? nullptr : mStack.back(); };
	// returns NULL if no filename can be found, else returns name & number of topmost input stream on stack which has info available
	const char*             GetFilenameAndLineNumber(int& lineNumber);

    const char*             GetReadPointer( void );
    const char*             GetBufferBasePointer( void );
    cell*                   GetReadOffsetPointer( void );
    cell                    GetBufferLength( void );
    void                    SetReadPointer( const char *pBuff );
    cell                    GetReadOffset( void );
    void                    SetReadOffset( cell offset );
    cell                    GetWriteOffset( void );
    void                    SetWriteOffset(cell offset );
	virtual bool			IsEmpty();

protected:
    std::vector<InputStream *>   mStack;
    InputStream* mpHead;
};

