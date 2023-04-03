#pragma once
//////////////////////////////////////////////////////////////////////
//
// FileInputStream.h: interface for the FileInputStream class.
//
//////////////////////////////////////////////////////////////////////

//#include "InputStream.h"
#include "InputStream.h"

// save-input items:
//  0   5
//  1   this pointer
//  2   lineNumber
//  3   readOffset
//  4   writeOffset (count of valid bytes in buffer)
//  5   lineStartOffset

class FileInputStream : public InputStream
{
public:
    FileInputStream( FILE *pInFile, const char* pFilename, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~FileInputStream();

    virtual char    *GetLine( const char *pPrompt );
    virtual char*   AddLine();
    virtual bool    IsInteractive(void) { return false; };
    virtual cell    GetLineNumber( void ) const;
    virtual InputStreamType GetType(void) const;
    virtual const char* GetName( void ) const;
    virtual cell    GetSourceID() const;

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

    // The saved work dir is the current directory before this input stream was started.
    // When a file stream becomes the current input the directory containing that file
    // becomes the current working directory.  When a file input stream is exhausted,
    // the current working directory is set to its saved work directory
    virtual const std::string& GetSavedWorkDir() const;
    virtual void SetSavedWorkDir(const std::string& s);

protected:
    FILE            *mpInFile;
    char*           mpName;
    cell            mLineNumber;
    uint32_t        mLineStartOffset;
    cell            mState[8];
    std::string     mSavedDir;
};

