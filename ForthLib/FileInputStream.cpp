//////////////////////////////////////////////////////////////////////
//
// FileInputStream.cpp: implementation of the FileInputStream class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "FileInputStream.h"
#include "Engine.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     FileInputStream
// 

FileInputStream::FileInputStream( FILE *pInFile, const char *pFilename, int bufferLen )
: InputStream(bufferLen)
, mpInFile( pInFile )
, mLineNumber( 0 )
, mLineStartOffset( 0 )
{
	mpName = (char *)__MALLOC(strlen(pFilename) + 1);
    strcpy( mpName, pFilename );
}

FileInputStream::~FileInputStream()
{
    // TODO: should we be closing file here?
    if ( mpInFile != NULL )
    {
        fclose( mpInFile );
    }
    __FREE( mpName );
}

const char* FileInputStream::GetName( void )
{
    return mpName;
}


char * FileInputStream::GetLine( const char *pPrompt )
{
    char *pBuffer;

    mLineStartOffset = ftell( mpInFile );

    pBuffer = fgets( mpBufferBase, mBufferLen, mpInFile );

    mReadOffset = 0;
    //printf("%s\n", pBuffer);
    mpBufferBase[ mBufferLen - 1 ] = '\0';
    mWriteOffset = strlen( mpBufferBase );
    if ( mWriteOffset > 0 )
    {
        // trim trailing linefeed if any
        if ( mpBufferBase[ mWriteOffset - 1 ] == '\n' )
        {
            --mWriteOffset;
            mpBufferBase[ mWriteOffset ] = '\0';
        }
    }
    mLineNumber++;
    return pBuffer;
}

cell FileInputStream::GetLineNumber(void)
{
    return mLineNumber;
}


const char* FileInputStream::GetType( void )
{
    return "File";
}

cell FileInputStream::GetSourceID()
{
    return static_cast<int>(reinterpret_cast<intptr_t>(mpInFile));
}

cell* FileInputStream::GetInputState()
{
    // save-input items:
    //  0   5
    //  1   this pointer
    //  2   lineNumber
    //  3   readOffset
    //  4   writeOffset (count of valid bytes in buffer)
    //  5   lineStartOffset

    cell* pState = &(mState[0]);
    pState[0] = 5;
    pState[1] = (cell)this;
    pState[2] = mLineNumber;
    pState[3] = mReadOffset;
    pState[4] = mWriteOffset;
    pState[5] = mLineStartOffset;
    
    return pState;
}

bool FileInputStream::SetInputState( cell* pState)
{
    if ( pState[0] != 5 )
    {
        // TODO: report restore-input error - wrong number of parameters
        return false;
    }
    if ( pState[1] != (cell)this )
    {
        // TODO: report restore-input error - input object mismatch
        return false;
    }
    if ( fseek( mpInFile, pState[5], SEEK_SET )  != 0 )
    {
        // TODO: report restore-input error - error seeking to beginning of line
        return false;
    }
    GetLine(NULL);
    if ( mWriteOffset != pState[4] )
    {
        // TODO: report restore-input error - line length doesn't match save-input value
        return false;
    }
    mLineNumber = pState[2];
    mReadOffset = pState[3];
    return true;
}

bool FileInputStream::IsFile(void)
{
    return true;
}

const std::string& FileInputStream::GetSavedWorkDir() const
{
    return mSavedDir;
}

void FileInputStream::SetSavedWorkDir(const std::string& s)
{
    mSavedDir = s;
}

