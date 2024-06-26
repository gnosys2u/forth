//////////////////////////////////////////////////////////////////////
//
// FileInputStream.cpp: implementation of the FileInputStream class.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the �Software�), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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

const char* FileInputStream::GetName( void ) const
{
    return mpName;
}


char * FileInputStream::GetLine( const char *pPrompt )
{
    if (mbForcedEmpty)
    {
        return nullptr;
    }

    char *pBuffer;

    mLineStartOffset = ftell( mpInFile );

    pBuffer = fgets( mpBufferBase, mBufferLen, mpInFile );

    mReadOffset = 0;

    TrimLine();

    mLineNumber++;
    return pBuffer;
}

char* FileInputStream::AddLine()
{
    char* pBuffer = nullptr;
    cell bufferLen = mBufferLen - mWriteOffset;
    char* bufferBase = mpBufferBase + mWriteOffset;
    if (bufferLen <= 0)
    {
        return pBuffer;
    }

    pBuffer = fgets(bufferBase, bufferLen, mpInFile);

    TrimLine();
    mLineNumber++;

    return mpBufferBase;
}

cell FileInputStream::GetLineNumber(void) const
{
    return mLineNumber;
}


InputStreamType FileInputStream::GetType( void ) const
{
    return InputStreamType::kFile;
}

cell FileInputStream::GetSourceID() const
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

const std::string& FileInputStream::GetSavedWorkDir() const
{
    return mSavedDir;
}

void FileInputStream::SetSavedWorkDir(const std::string& s)
{
    mSavedDir = s;
}

