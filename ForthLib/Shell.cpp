//////////////////////////////////////////////////////////////////////
//
// Shell.cpp: implementation of the Shell class.
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
#include <filesystem>
#include <iostream>

#if defined(LINUX) || defined(MACOSX)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(MACOSX)
#include <sys/uio.h>
#else
#ifndef RASPI
#include <sys/io.h>
#endif
#endif
#include <dirent.h>
#else
#include <io.h>
#include "sys/dirent.h"
#endif
#include "Engine.h"
#include "OuterInterpreter.h"
#include "Thread.h"
#include "Shell.h"
#include "InputStack.h"
#include "BlockInputStream.h"
#include "BufferInputStream.h"
#include "ConsoleInputStream.h"
#include "ExpressionInputStream.h"
#include "FileInputStream.h"
#include "Vocabulary.h"
#include "Extension.h"
#include "ParseInfo.h"

#define CATCH_EXCEPTIONS

#ifndef STORAGE_LONGS
#define STORAGE_LONGS (16 * 1024 * 1024)
#endif

#ifndef PSTACK_LONGS
#define PSTACK_LONGS 8192
#endif

#ifndef RSTACK_LONGS
#define RSTACK_LONGS 8192
#endif

#define CONTINUATION_MARKER "\\+"
#define CONTINUATION_MARKER_LEN 2

// switch checkOpResult on to provide a convenient place to put a breakpoint when trying
// down where an error is coming from in the debugger
#if 0
void checkOpResult(OpResult result)
{
    if (result != OpResult::kOk)
    {
        printf("bad OpResult: %d\n", (int) result);
    }
}
#else
#define checkOpResult(RESULT)
#endif


// split a string with delimiter
void splitString(const std::string inString, std::vector<std::string>& tokens, char delim)
{
    std::stringstream stringStream(inString);
    std::string token;

    while (getline(stringStream, token, delim))
    {
        tokens.push_back(token);
    }
}

namespace
{
    int fileExists( const char* pFilename )
    {
		FILE* pFile = NULL;
		pFile = fopen( pFilename, "r" );
        int result = (pFile != 0) ? ~0 : 0;
        if ( pFile != NULL )
        {
            fclose( pFile );
        }
        return result;
    }

    int32_t fileGetLength( FILE* pFile )
    {
        int32_t oldPos = ftell( pFile );
        fseek( pFile, 0, SEEK_END );
        int32_t result = ftell( pFile );
        fseek( pFile, oldPos, SEEK_SET );
        return result;
    }

	FILE* getStdIn()
	{
		return stdin;
	}

	FILE* getStdOut()
	{
		return stdout;
	}

	FILE* getStdErr()
	{
		return stderr;
	}

	int makeDir( const char* pPath, int mode )
	{
#ifdef WIN32
		return _mkdir( pPath );
#else
		return mkdir( pPath, mode );
#endif
	}

    // return is a DIR*
    void* openDir(const char* pPath)
    {
        return opendir(pPath);
    }

    // return is a struct dirent*
    void* readDir(void* pDir, void* pEntry)
    {
        struct dirent* pResult = readdir((DIR*)pDir);
        if (pResult)
        {
            memcpy(pEntry, pResult, sizeof(struct dirent));
        }

        return pResult;
    }

    int closeDir(void* pDir)
    {
        return closedir((DIR*)pDir);
    }

    void rewindDir(void* pDir)
    {
        rewinddir((DIR*)pDir);
    }

    int getWorkDir(char* pDstPath, int dstPathMax)
    {
        std::filesystem::path workDir = std::filesystem::current_path();
        memcpy(pDstPath, workDir.string().c_str(), (size_t)dstPathMax - 1);
        pDstPath[dstPathMax - 1] = '\0';
        return workDir.string().length();
    }

}

#if defined(WIN32)
DWORD WINAPI ConsoleInputThreadRoutine( void* pThreadData );
#elif defined(LINUX) || defined(MACOSX)
uint32_t ConsoleInputThreadRoutine( void* pThreadData );
#endif

//////////////////////////////////////////////////////////////////////
////
///
//                     Shell
// 

Shell::Shell(int argc, const char ** argv, const char ** envp, Engine *pEngine, Extension *pExtension, int controlStackLongs)
: mpEngine(pEngine)
, mFlags(0)
, mPoundIfDepth(0)
, mpInternalFiles(NULL)
, mInternalFileCount(0)
, mExpressionInputStream(NULL)
{
    startMemoryManager();

    mFileInterface.fileOpen = fopen;
    mFileInterface.fileClose = fclose;
    mFileInterface.fileRead = fread;
    mFileInterface.fileWrite = fwrite;
    mFileInterface.fileGetChar = fgetc;
    mFileInterface.filePutChar = fputc;
    mFileInterface.fileAtEnd = feof;
    mFileInterface.fileExists = fileExists;
    mFileInterface.fileSeek = fseek;
    mFileInterface.fileTell = ftell;
    mFileInterface.fileGetLength = fileGetLength;
    mFileInterface.fileGetString = fgets;
    mFileInterface.filePutString = fputs;
	mFileInterface.fileRemove = remove;
#if defined( WIN32 )
	mFileInterface.fileDup = _dup;
	mFileInterface.fileDup2 = _dup2;
	mFileInterface.fileNo = _fileno;
#else
	mFileInterface.fileDup = dup;
	mFileInterface.fileDup2 = dup2;
	mFileInterface.fileNo = fileno;
#endif
	mFileInterface.fileFlush = fflush;
	mFileInterface.renameFile = rename;
	mFileInterface.runSystem = system;
    mFileInterface.getWorkDir = getWorkDir;
#ifdef WIN32
    mFileInterface.setWorkDir = _chdir;
    mFileInterface.removeDir = _rmdir;
#else
    mFileInterface.setWorkDir = chdir;
    mFileInterface.removeDir = rmdir;
#endif
    mFileInterface.makeDir = makeDir;
	mFileInterface.getStdIn = getStdIn;
	mFileInterface.getStdOut = getStdOut;
	mFileInterface.getStdErr = getStdErr;
	mFileInterface.openDir = openDir;
	mFileInterface.readDir = readDir;
	mFileInterface.closeDir = closeDir;
	mFileInterface.rewindDir = rewindDir;

#if defined( WIN32 )
    DWORD result = GetCurrentDirectoryA(MAX_PATH, mWorkingDirPath);
    if (result == 0)
    {
        mWorkingDirPath[0] = '\0';
    }
#elif defined(LINUX) || defined(MACOSX)
    if (getcwd(mWorkingDirPath, MAX_PATH) == NULL)
    {
        // failed to get current directory
        strcpy(mWorkingDirPath, ".");
    }
#endif

    SetCommandLine(argc, argv);
    SetEnvironmentVars(envp);

    if ( mpEngine == NULL )
    {
        mpEngine = new Engine();
        mFlags = SHELL_FLAG_CREATED_ENGINE;
    }
    mpEngine->Initialize( this, STORAGE_LONGS, true, pExtension );

#if 0
    if ( mpThread == NULL )
    {
        mpThread = mpEngine->CreateThread( 0, PSTACK_LONGS, RSTACK_LONGS );
    }
    mpEngine->SetCurrentThread( mpThread );
#endif

    mpInput = new InputStack;
	mpStack = new ControlStack( controlStackLongs );

#if 0
    mMainThreadId = GetThreadId( GetMainFiber() );
    mConsoleInputThreadId = 0;
    mConsoleInputThreadHandle = _beginthreadex( NULL,		// thread security attribs
                                                0,			// stack size (default)
                                                ConsoleInputThreadLoop,
                                                this,
                                                CREATE_SUSPENDED,
                                                &mConsoleInputThreadId );

    mConsoleInputEvent = CreateEvent( NULL,               // default security attributes
                                      FALSE,              // auto-reset event
                                      FALSE,              // initial state is nonsignaled
                                      TEXT("ConsoleInputEvent") ); // object name

   mpReadyThreads = new ForthThreadQueue;
#endif

}

Shell::~Shell()
{
    DeleteEnvironmentVars();
    DeleteCommandLine();
    delete mpInput;
	delete mpStack;
	if (mExpressionInputStream != NULL)
	{
		delete mExpressionInputStream;
	}
    // engine will destroy thread for us if we created it
	if (mFlags & SHELL_FLAG_CREATED_ENGINE)
	{
		delete mpEngine;
	}

    stopMemoryManager();
#if 0
    delete mpReadyThreads;
    CloseHandle( mConsoleInputEvent );
#endif
}



//
// create a new file input stream & push on stack
//
bool Shell::PushInputFile( const char *pFilename )
{
    std::string containingDir;
    FILE *pInFile = OpenForthFile( pFilename, containingDir);
    if ( pInFile != NULL )
    {
        //printf("%s is contained in {%s}\n", pFilename, containingDir.c_str());
        loadedFileInfo lfi;
        lfi.startDP = mpEngine->GetDP();
        GetFileLeafName(pFilename, lfi.filename);
        mLoadedFiles.emplace_back(lfi);

        FileInputStream* newStream = new FileInputStream(pInFile, pFilename);
        std::string curWorkDir;
        GetWorkDir(curWorkDir);
        newStream->SetSavedWorkDir(curWorkDir);
        SetWorkDir(containingDir);

        mpInput->PushInputStream(newStream);
        return true;
    }
    return false;
}


//
// create a new buffer input stream & push on stack
//
void
Shell::PushInputBuffer( const char *pDataBuffer, int dataBufferLen )
{
    mpInput->PushInputStream( new BufferInputStream( pDataBuffer, dataBufferLen ) );
}


void
Shell::PushInputBlocks(BlockFileManager* pManager, uint32_t firstBlock, uint32_t lastBlock)
{
    mpInput->PushInputStream( new BlockInputStream(pManager, firstBlock, lastBlock) );
}


bool
Shell::PopInputStream( void )
{
    InputStream* curStream = mpInput->Top();
    if (curStream->GetType() == InputStreamType::kFile)
    {
        FileInputStream* fileStream = dynamic_cast<FileInputStream*>(curStream);
        if (!fileStream->GetSavedWorkDir().empty())
        {
            mFileInterface.setWorkDir(fileStream->GetSavedWorkDir().c_str());
        }
    }
    return mpInput->PopInputStream();
}


//
// interpret one stream, return when it is exhausted
//
int
Shell::RunOneStream(InputStream *pInStream)
{
	const char *pBuffer;
	int retVal = 0;
	bool bQuit = false;
	OpResult result = OpResult::kOk;

	InputStream* pOldInput = mpInput->Top();
	mpInput->PushInputStream(pInStream);

	while ((mpInput->Top() != pOldInput) && !bQuit)
	{
		// try to fetch a line from current stream
		pBuffer = mpInput->GetLine(mpEngine->GetFastMode() ? "ok>" : "OK>");
        if (pBuffer != nullptr)
        {
            while (LineHasContinuation(pBuffer))
            {
                mpInput->Shorten(CONTINUATION_MARKER_LEN);
                if (mpInput->AddLine() == nullptr)
                {
                    // TODO: report failure to add continuation line
                    break;
                }
            }
        }

        if (pBuffer == nullptr)
		{
            bQuit = PopInputStream() || (mpInput->Top() == pOldInput);
		}

		if (!bQuit)
		{
			result = ProcessLine();
            checkOpResult(result);

			switch (result)
			{

			case OpResult::kOk:
				break;

			case OpResult::kShutdown:			// what should shutdown do on non-client/server?
			case OpResult::kExitShell:
				// users has typed "bye", exit the shell
				bQuit = true;
				retVal = 0;
				break;

			case OpResult::kError:
			case OpResult::kUncaughtException:
				// an error has occured, empty input stream stack
				// TODO
				bQuit = true;
				retVal = 0;
				break;

			case OpResult::kFatalError:
			default:
				// a fatal error has occured, exit the shell
				bQuit = true;
				retVal = 1;
				break;
			}
		}
	}

	return retVal;
}

//
// interpret named file, interpret from standard in if
//   pFileName is NULL
// return 0 for normal exit
//
int
Shell::Run( InputStream *pInStream )
{
    const char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    OpResult result = OpResult::kOk;
    bool bInteractiveMode = pInStream->IsInteractive();

    mpInput->PushInputStream( pInStream );

    const char* autoloadFilename = "app_autoload.fs";
	FILE* pFile = OpenInternalFile( autoloadFilename );
	if ( pFile == NULL )
	{
		// no internal file found, try opening app_autoload.fs as a standard file
		pFile = fopen( autoloadFilename, "r" );
	}

    if ( pFile != NULL )
    {
        // there is an app autoload file, use that
        fclose( pFile );
    }
    else
    {
        // no app autload, try using the normal autoload file
        autoloadFilename = "forth_autoload.fs";
    }
    PushInputFile( autoloadFilename );

    const char* pPrompt = mpEngine->GetFastMode() ? "ok>" : "OK>";
    while ( !bQuit )
    {
        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine(pPrompt);
        if (pBuffer != nullptr)
        {
            while (LineHasContinuation(pBuffer))
            {
                mpInput->Shorten(CONTINUATION_MARKER_LEN);
                if (mpInput->AddLine() == nullptr)
                {
                    // TODO: report failure to add continuation line
                    break;
                }
            }
        }

        if (pBuffer == nullptr)
        {
            bQuit = PopInputStream();
        }

        if (!bQuit)
        {
            result = ProcessLine();
            checkOpResult(result);

            switch( result )
            {

            case OpResult::kOk:
                break;

            case OpResult::kShutdown:			// what should shutdown do on non-client/server?
            case OpResult::kExitShell:
                // users has typed "bye", exit the shell
                bQuit = true;
                retVal = 0;
                break;

            case OpResult::kError:
            case OpResult::kUncaughtException:
                // an error has occured, empty input stream stack
                if (result != OpResult::kOk)
                {
                    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();

                    // in case console out was redirected, point it back to the user-visible console
                    mpEngine->ResetConsoleOut();

                    bool exitingShell = (result == OpResult::kExitShell) || (result == OpResult::kShutdown);
                    if (!exitingShell)
                    {
                        ReportError();
                        if (mpEngine->GetError() == ForthError::undefinedWord)
                        {
                            ForthConsoleCharOut(mpEngine->GetCoreState(), '\n');
                            pOuter->ShowSearchInfo();
                        }
                        mpEngine->DumpCrashState();
                    }

                    ErrorReset();
                    if (!mpInput->Top()->IsInteractive() && !exitingShell)
                    {
                        // if the initial input stream was a file, any error
                        //   must be treated as a fatal error
                        result = OpResult::kFatalError;
                    }
                }

                if ( !bInteractiveMode )
                {
                    bQuit = true;
                }
                else
                {
                    // TODO: dump all but outermost input stream
                }
                retVal = 0;
                break;

            case OpResult::kFatalError:
            default:
                // a fatal error has occured, exit the shell
                bQuit = true;
                retVal = 1;
                break;
            }
        }
    } // while !bQuit

    checkOpResult(result);
    return retVal;
}

bool Shell::LineHasContinuation(const char* pBuffer)
{
    bool result = false;

    if (pBuffer != nullptr)
    {
        int lineLen = (int)strlen(pBuffer);
        int lenWithoutContinuation = lineLen - CONTINUATION_MARKER_LEN;
        if (lenWithoutContinuation >= 0)
        {
            result = !strcmp(pBuffer + lenWithoutContinuation, CONTINUATION_MARKER);
        }
    }
    return result;
}


// ProcessLine is the layer between Run and InterpretLine that implements pound directives
OpResult Shell::ProcessLine()
{
    OpResult result = OpResult::kOk;

    if ( (mFlags & SHELL_FLAG_SKIP_SECTION) != 0 )
    {
        // we are currently skipping input lines, check if we should stop skipping
        char* pToken = GetNextSimpleToken();
        if ( pToken != NULL )
        {
            if ( !strcmp( "#if", pToken ) )
            {
                mPoundIfDepth++;
            }
            else if ( !strcmp( "#ifdef", pToken ) )
            {
                mPoundIfDepth++;
            }
            else if ( !strcmp( "#ifndef", pToken ) )
            {
                mPoundIfDepth++;
            }
            else if ( !strcmp( "#else", pToken ) )
            {
                if ( mPoundIfDepth == 0 )
                {
                    mFlags &= ~SHELL_FLAG_SKIP_SECTION;
                }
            }
            else if ( !strcmp( "#endif", pToken ) )
            {
                if ( mPoundIfDepth == 0 )
                {
                    ControlStackEntry* pEntry = mpStack->Peek();
					if (pEntry == nullptr || pEntry->tag != kCSTagPoundIf)
					{
						// error - unexpected else
						mpEngine->SetError(ForthError::preprocessorError, "unexpected #endif");
					}
                    mpStack->Drop();
					mFlags &= ~SHELL_FLAG_SKIP_SECTION;
                }
                else
                {
                    mPoundIfDepth--;
                }

            }
        }
        if (mpEngine->GetError() != ForthError::none)
        {
            result = OpResult::kError;
            checkOpResult(result);
        }
    }
    else
    {
        // we are currently not skipping input lines
        result = InterpretLine();
        checkOpResult(result);

        if ( result == OpResult::kOk )
        {
            // process pound directive if needed
            if ( (mFlags & SHELL_FLAG_START_IF) != 0 )
            {
                // this line started with #if
                CoreState* pCore = mpEngine->GetCoreState();
                if ( GET_SDEPTH > 0 )
                {
                    cell expressionResult = SPOP;
                    mpStack->Push(kCSTagPoundIf);
                    if (expressionResult == 0)
                    {
                        // skip to #else or #endif
                        mFlags |= SHELL_FLAG_SKIP_SECTION;
                        mPoundIfDepth = 0;
                    }
                    if ( (mFlags & SHELL_FLAG_START_IF_C) != 0 )
                    {
                        // state was compile before #if, put it back
                        mpEngine->SetCompileState( 1 );
                    }
                }
                else
                {
                    mpEngine->SetError( ForthError::preprocessorError, "#if expression left empty stack" );
                }
                mFlags &= ~SHELL_FLAG_START_IF;
            }
        }
    }

    checkOpResult(result);
    return result;
}

static bool gbCatchExceptions = false;
//
// return true IFF the forth shell should exit
//
OpResult Shell::InterpretLine()
{
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    OpResult  result = OpResult::kOk;
    bool bLineEmpty;
    ParseInfo parseInfo( mTokenBuffer, sizeof(mTokenBuffer) >> 2 );

    SPEW_SHELL( "\n*** InterpretLine {%s}\n", mpInput->GetBufferBasePointer());
    bLineEmpty = false;
    mpEngine->SetError( ForthError::none );
    while ( !bLineEmpty && (result == OpResult::kOk) )
	{
        bLineEmpty = ParseToken( &parseInfo );
        if (mpEngine->GetError() != ForthError::none)
        {
            result = OpResult::kError;
        }
        InputStream* pInput = mpInput->Top();
        SPEW_SHELL("input %s[%d] buffer 0x%x readoffset %d write %d\n", pInput->GetName(),
            pInput->GetLineNumber(), pInput->GetReadPointer(), pInput->GetReadOffset(), pInput->GetWriteOffset() );

        if (!bLineEmpty && (result == OpResult::kOk))
		{


#ifdef WIN32
			if ( gbCatchExceptions )
			{
				try
				{
					result = pOuter->ProcessToken( &parseInfo );
					CHECK_STACKS( mpEngine->GetMainFiber() );
				}
				catch(...)
				{
                    // TODO - this isn't an uncaught exception
					result = OpResult::kUncaughtException;
					mpEngine->SetError( ForthError::illegalOperation );
				}
			}
			else
			{
                result = pOuter->ProcessToken( &parseInfo );
                CHECK_STACKS( mpEngine->GetMainFiber() );
			}
#else
            result = pOuter->ProcessToken( &parseInfo );
            checkOpResult(result);
            CHECK_STACKS( mpEngine->GetMainFiber() );
#endif
            if ( result == OpResult::kOk )
			{
                result = mpEngine->CheckStacks();
                checkOpResult(result);
            }
        }
    }

    return result;
}

void
Shell::ErrorReset( void )
{
	mpEngine->ErrorReset();
	mpInput->Reset();
	mpStack->EmptyStack();
    mPoundIfDepth = 0;
    mFlags &= SHELL_FLAGS_NOT_RESET_ON_ERROR;
}

void
Shell::ReportError( void )
{
    char errorBuf1[512];
    char errorBuf2[512];
    const char *pLastInputToken;

    ForthError err = mpEngine->GetError();
    mpEngine->GetErrorString( err, errorBuf1, sizeof(errorBuf1) );
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    pLastInputToken = pOuter->GetLastInputToken();
	CoreState* pCore = mpEngine->GetCoreState();

	if ( pLastInputToken != NULL )
    {
        sprintf( errorBuf2, "%s, last input token: <%s> last IP 0x%p",
            errorBuf1, pLastInputToken, pCore->IP );
    }
    else
    {
        sprintf( errorBuf2, "%s", errorBuf1 );
    }
	int lineNumber;
	const char* fileName = mpInput->GetFilenameAndLineNumber(lineNumber);
    if ( fileName != NULL )
    {
		sprintf(errorBuf1, "%s at line number %d of %s", errorBuf2, lineNumber, fileName);
    }
    else
    {
        strcpy( errorBuf1, errorBuf2 );
    }
    SPEW_SHELL( "%s", errorBuf1 );
	ERROR_STRING_OUT( errorBuf1 );
    const char *pBase = mpInput->GetBufferBasePointer();
    pLastInputToken = mpInput->GetReadPointer();
    if ( (pBase != NULL) && (pLastInputToken != NULL) )
    {
		char* pBuf = errorBuf1;
		*pBuf++ = '\n';
        while ((pBase < pLastInputToken) && (*pBase != '\0'))
        {
            *pBuf++ = *pBase++;
        }
        *pBuf++ = '{';
        *pBuf++ = '}';
		char *pBufferLimit = &(errorBuf1[0]) + (sizeof(errorBuf1) - 2);
        while ( (*pLastInputToken != '\0') && (pBuf < pBufferLimit))
        {
            *pBuf++ = *pLastInputToken++;
        }
        *pBuf++ = '\n';
        *pBuf++ = '\0';
    }
	SPEW_SHELL( "%s", errorBuf1 );
    ERROR_STRING_OUT( errorBuf1 );

	if (mpStack->GetDepth() > 0)
	{
		mpStack->ShowStack();
	}
}

void Shell::ReportWarning(const char* pMessage)
{
    char errorBuf1[512];
    char errorBuf2[512];
    const char *pLastInputToken;
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();

    pLastInputToken = pOuter->GetLastInputToken();
    CoreState* pCore = mpEngine->GetCoreState();

    if (pLastInputToken != NULL)
    {
        sprintf(errorBuf2, "WARNING %s, last input token: <%s> last IP 0x%p",
            pMessage, pLastInputToken, pCore->IP);
    }
    else
    {
        sprintf(errorBuf2, "WARNING %s", pMessage);
    }
    int lineNumber;
    const char* fileName = mpInput->GetFilenameAndLineNumber(lineNumber);
    if (fileName != NULL)
    {
        sprintf(errorBuf1, "%s at line number %d of %s", errorBuf2, lineNumber, fileName);
    }
    else
    {
        strcpy(errorBuf1, errorBuf2);
    }
    SPEW_SHELL("%s", errorBuf1);
    CONSOLE_STRING_OUT(errorBuf1);
    const char *pBase = mpInput->GetBufferBasePointer();
    pLastInputToken = mpInput->GetReadPointer();
    if ((pBase != NULL) && (pLastInputToken != NULL))
    {
        char* pBuf = errorBuf1;
        *pBuf++ = '\n';
        while (pBase < pLastInputToken)
        {
            *pBuf++ = *pBase++;
        }
        *pBuf++ = '{';
        *pBuf++ = '}';
        char *pBufferLimit = &(errorBuf1[0]) + (sizeof(errorBuf1) - 2);
        while ((*pLastInputToken != '\0') && (pBuf < pBufferLimit))
        {
            *pBuf++ = *pLastInputToken++;
        }
        *pBuf++ = '\n';
        *pBuf++ = '\0';
    }
    SPEW_SHELL("%s", errorBuf1);
    CONSOLE_STRING_OUT(errorBuf1);
}

// return true IFF done parsing line - in this case no string is returned in pInfo
// this is a stripped down version of ParseToken used just for building string tables
// TODO!!! there is nothing to keep us from writing past end of pTokenBuffer
bool
Shell::ParseString( ParseInfo *pInfo )
{
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    const char *pSrc;
    const char *pEndSrc = nullptr;
    char *pDst;
    bool gotAToken = false;
    const char* pSrcLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();

    pInfo->SetAllFlags( 0 );

    while ( !gotAToken )
    {

        pSrc = mpInput->GetReadPointer();
        pDst = pInfo->GetToken();
        if ( (*pSrc == '\0') || (pSrc >= pSrcLimit) )
        {
            // input buffer is empty
            return true;
        }

        *pDst = 0;

        // eat any leading white space
        while ( (*pSrc == ' ') || (*pSrc == '\t') )
        {
            pSrc++;
        }

        // parse symbol up till next white space
        switch ( *pSrc )
        {
           case '\"':
              // support C-style quoted strings...
              if (pOuter->CheckFeature( kFFCStringLiterals ) )
              {
                  pEndSrc = pSrc;
                  pInfo->ParseDoubleQuote(pEndSrc, pSrcLimit);
                  gotAToken = true;
              }
              break;

           default:
              break;
        }

        if ( pInfo->GetFlags() == 0 )
        {
            // token is not a special case, just parse till blank, space or EOL
           bool done = false;
            pEndSrc = pSrc;
            while ( !done && (pEndSrc < pSrcLimit) )
            {
               char ch = *pEndSrc;
               switch ( ch )
               {
                  case ' ':
                  case '\t':
                  case '\0':
                     done = true;
                     *pDst++ = '\0';
                     // set token length byte
                     pInfo->SetToken();
                     gotAToken = true;
                     if ( ch != '\0' )
                     {
                         ++pEndSrc;
                     }
                     break;

                  default:
                     *pDst++ = *pEndSrc++;
                     break;

               }
            } // while not done
        }

        mpInput->SetReadPointer( (char *) pEndSrc );
    }   // while ! gotAToken

    return false;
}


// return true IFF done parsing line - in this case no token is returned in pInfo
// TODO!!! there is nothing to keep us from writing past end of pTokenBuffer
bool
Shell::ParseToken( ParseInfo *pInfo )
{
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    const char *pSrc;
    const char *pEndSrc = nullptr;
    char *pDst;
    bool gotAToken = false;

    pInfo->SetAllFlags( 0 );
    if ( mFlags & SHELL_FLAG_POP_NEXT_TOKEN )
    {
        // previous symbol ended in ")", so next token to process is on top of control stack
        mFlags &= ~SHELL_FLAG_POP_NEXT_TOKEN;
        ControlStackEntry* pEntry = mpStack->Peek();
        if ( CheckSyntaxError( ")", pEntry->tag, kCSTagParen ) )
        {
            if ( pEntry->name != nullptr )
            {
                strncpy(pInfo->GetToken(), pEntry->name, pInfo->GetMaxChars());
				pSrc = pInfo->GetToken();

				 // set parseInfo flags for popped token
				 while ( *pSrc != '\0' )
				 {
					 switch ( *pSrc )
					 {
					 case '.':
						 pInfo->SetFlag( PARSE_FLAG_HAS_PERIOD );
						 break;

					 case ':':
						pInfo->SetFlag( PARSE_FLAG_HAS_COLON );
						break;

					 default:
						 break;
					 }

					 pSrc++;
				 }
            }
            else
            {
                char* pTagString = (char *)malloc(512);
                mpStack->GetTagString(pEntry->tag, pTagString, 512);
                sprintf(mErrorString,  "top of control stack is <%s>, was expecting <string>", pTagString);
                free(pTagString);
                mpEngine->SetError( ForthError::badSyntax, mErrorString );
            }
        }
        mpStack->Drop();
        return false;
    }

    while ( !gotAToken )
    {
        bool advanceInputBuffer = true;

		const char* pSrcLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();
		pSrc = mpInput->GetReadPointer();
        pDst = pInfo->GetToken();
        if ( pSrc >= pSrcLimit )
        {
            // input buffer is empty
            return true;
        }

        *pDst = '\0';

        // eat any leading white space
        while ( (*pSrc == ' ') || (*pSrc == '\t') )
        {
            pSrc++;
        }

        // parse symbol up till next white space
        switch ( *pSrc )
        {
           case '\"':
              // support C-style quoted strings...
              if ( pOuter->CheckFeature( kFFCStringLiterals ) )
              {
                  pEndSrc = pSrc;
                  pInfo->ParseDoubleQuote(pEndSrc, pSrcLimit);
                  gotAToken = true;
              }
              break;

           case '\'':
               // support C-style quoted characters like 'a' or '\n'
               pEndSrc = pInfo->ParseSingleQuote(pSrc, pSrcLimit, mpEngine);
               gotAToken = true;
               break;

           default:
              break;
        }

        if ( pInfo->GetFlags() == 0 )
        {
            // token is not a special case, just parse till blank, space or EOL
            bool done = false;
            pEndSrc = pSrc;
            while ( !done && (pSrc < pSrcLimit) )
            {
                char ch = *pEndSrc;
                switch ( ch )
                {
                  case ' ':
                  case '\t':
                  case '\0':
                     done = true;
                     *pDst++ = '\0';
                     // set token length byte
                     pInfo->SetToken();
                     gotAToken = pInfo->GetTokenLength() != 0;
                     pEndSrc++;
                     break;

                  case '(':
                     if (pEndSrc == pSrc)
                     {
                         // paren at start of token is part of token (allows old forth-style inline comments to work)
                         *pDst++ = *pEndSrc++;
                     }
                     else if (pOuter->CheckFeature(kFFParenIsExpression))
                     {
                         // push accumulated token (if any) onto control stack
						 advanceInputBuffer = false;
						 if (mExpressionInputStream == NULL)
						 {
							 mExpressionInputStream = new ExpressionInputStream;
						 }
						 pInfo->SetAllFlags(0);
						 mpInput->SetReadPointer(pSrc);
						 mExpressionInputStream->ProcessExpression(mpInput->Top());
                         // begin horrible nasty kludge for elseif
                         const char* endOfExpression = mExpressionInputStream->GetBufferBasePointer() + (mExpressionInputStream->GetWriteOffset() - 1);
                         int charsToCrop = 7;
                         while (*endOfExpression == ' ')
                         {
                             --endOfExpression;
                             ++charsToCrop;
                         }
                         if (strncmp(" elseif", endOfExpression - 6, 7) == 0)
                         {
                             mExpressionInputStream->CropCharacters(charsToCrop);
                             mExpressionInputStream->PrependString(" else ");
                             mExpressionInputStream->AppendString(" ]if");
                         }
                         // end horrible nasty kludge for elseif
                         mpInput->PushInputStream(mExpressionInputStream);
						 done = true;
						 /*pEndSrc++;
                         *pDst++ = '\0';
                         pDst = pInfo->GetToken();
                         mpStack->PushString( pDst );
                         mpStack->PushTag( kCSTagParen );*/
                     }
					 else
					 {
						 *pDst++ = *pEndSrc++;
					 }
                     break;

                  case ')':
					 if (pOuter->CheckFeature(kFFParenIsExpression))
					 {
                        // process accumulated token (if any), pop control stack, compile/interpret if not empty
                        pEndSrc++;
                        done = true;
                        *pDst++ = '\0';
                        mFlags |= SHELL_FLAG_POP_NEXT_TOKEN;
                        pInfo->SetToken();
                        gotAToken = true;
                     }
					 else
					 {
						 *pDst++ = *pEndSrc++;
					 }
					 break;

                  case '.':
                     pInfo->SetFlag( PARSE_FLAG_HAS_PERIOD );
                     *pDst++ = *pEndSrc++;
                     break;

                  case ':':
                     pInfo->SetFlag( PARSE_FLAG_HAS_COLON );
                     *pDst++ = *pEndSrc++;
                     break;

                  default:
                     *pDst++ = *pEndSrc++;
                     break;

               }
            } // while not done
        }

		if (advanceInputBuffer)
		{
			mpInput->SetReadPointer((char *)pEndSrc);
		}
    }   // while ! gotAToken

    return false;
}


char *
Shell::GetNextSimpleToken( void )
{
	while (mpInput->IsEmpty() && mpInput->Top()->IsGenerated())
	{
		SPEW_SHELL("GetNextSimpleToken: %s empty, popping\n", mpInput->Top()->GetName());
		if (mpInput->PopInputStream())
		{
			// no more input streams available
			return NULL;
		}
	}
    const char *pEndToken = mpInput->GetReadPointer();
    const char* pTokenLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();
    char c;
    bool bDone;
    char* pDst = &mToken[0];

    // eat any leading white space
    while ( (*pEndToken == ' ') || (*pEndToken == '\t') )
    {
        pEndToken++;
    }

    bDone = false;
    while ( !bDone && (pEndToken <= pTokenLimit) )
    {
        c = *pEndToken++;
        switch( c )
        {
        case ' ':
        case '\t':
        case '\0':
            bDone = true;
            break;
        default:
            *pDst++ = c;
        }
    }
    *pDst++ = '\0';
    mpInput->SetReadPointer( pEndToken );

    //SPEW_SHELL( "GetNextSimpleToken: |%s|%s|\n", &mToken[0], pEndToken );
    return &mToken[0];
}


char *
Shell::GetToken( int idelim, bool bSkipLeadingDelims )
{
    char delim = (char)idelim;
    
    while (mpInput->IsEmpty() && mpInput->Top()->IsGenerated())
	{
		SPEW_SHELL("GetToken: %s empty, popping\n", mpInput->Top()->GetName());
		if (mpInput->PopInputStream())
		{
			// no more input streams available
			return NULL;
		}
	}
	const char *pEndToken = mpInput->GetReadPointer();
    const char* pTokenLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();
    char c;
    bool bDone;
    char* pDst = &mToken[0];

    if (bSkipLeadingDelims)
    {
        // eat any leading white space
        if (delim == ' ')
        {
            while ((*pEndToken == ' ') || (*pEndToken == '\t'))
            {
                pEndToken++;
            }
        }
        else
        {
            while (*pEndToken == delim)
            {
                pEndToken++;
            }
        }
    }

    bDone = false;
    if (idelim == -1)
    {
        // -1 delimiter means continue until whitespace, end-of-line, or end of buffer
        while (!bDone && (pEndToken <= pTokenLimit))
        {
            c = *pEndToken++;
            if (c == ' ' || c == '\t' || c == '\n')
            {
                bDone = true;
            }
            else
            {
                *pDst++ = c;
            }
        }
    }
    else
    {
        while (!bDone && (pEndToken <= pTokenLimit))
        {
            c = *pEndToken++;
            if (c == '\\')
            {
                // another 
                *pDst++ = c;
            }
            else if (c == delim)
            {
                bDone = true;
            }
            else
            {
                *pDst++ = c;
            }
        }
    }
    *pDst++ = '\0';
    mpInput->SetReadPointer( pEndToken );

    //SPEW_SHELL( "GetToken: |%s|%s|\n", &mToken[0], pEndToken );
    return &mToken[0];
}

char* Shell::GetToken2012(int idelim)
{
    const char* pEndToken = mpInput->GetReadPointer();
    const char* pTokenLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();
    char c;
    bool bDone;
    char* pDst = &mToken[0];

    // this method is done solely to allow implementing the ANSI 2012 version of S\",
    // using the usual GetToken or other parsing words would break because the spec
    // includes an extra escape sequence for double quote (\"), which wouldn't work
    // with GetToken.

    // eat any leading white space
    while ((*pEndToken == ' ') || (*pEndToken == '\t'))
    {
        pEndToken++;
    }

    bDone = false;
    char delim = (char)idelim;
    while (!bDone && (pEndToken <= pTokenLimit))
    {
        c = *pEndToken++;
        if (c == '\\')
        {
            // copy the backslash and following char (3 chars for \xNN hex escape sequence)
            *pDst++ = c;
            if (pEndToken <= pTokenLimit)
            {
                c = *pEndToken++;
                *pDst++ = c;
                if (c == 'x')
                {
                    // copy 2 chars after the x
                    if (pEndToken <= pTokenLimit)
                    {
                        *pDst++ = *pEndToken++;
                        if (pEndToken <= pTokenLimit)
                        {
                            *pDst++ = *pEndToken++;
                        }
                    }
                }
            }
        }
        else if (c == delim)
        {
            bDone = true;
        }
        else
        {
            *pDst++ = c;
        }
    }
    *pDst++ = '\0';
    mpInput->SetReadPointer(pEndToken);

    //SPEW_SHELL( "GetToken: |%s|%s|\n", &mToken[0], pEndToken );
    return &mToken[0];
}

void
Shell::SetCommandLine( int argc, const char ** argv )
{
    int i, len;

    if (argv == nullptr)
    {
        return;
    }
    DeleteCommandLine();

    mArgs.resize(argc);

    for (i = 0; i < argc; ++i)
    {
        mArgs[i].assign(argv[i]);
    }

    // see if there are files appended to the executable file
    // the format is:
    //
    // PER FILE:
    // ... file1 databytes...
    // length of file1 as int
    // 0xDEADBEEF
    // ... file1name ...
    // length of file1name as int
    //
    // ... fileZ databytes...
    // length of fileZ as int
    // 0xDEADBEEF
    // ... fileZname ...
    // length of fileZname as int
    //
    // number of appended files as int
    // 0x34323137
    if ( (argc > 0) && (argv[0] != NULL) )
    {
        FILE* pFile = NULL;
		pFile = fopen( argv[0], "rb" );
        if ( pFile != NULL )
        {
            int res = fseek( pFile, -4, SEEK_END );
            int token = 0;
            if ( res == 0 )
            {
                size_t nItems = fread( &token, sizeof(token), 1, pFile );
                if ( nItems == 1 )
#define FORTH_MAGIC_1 0x37313234
#define FORTH_MAGIC_2 0xDEADBEEF
                {
                    if ( token == FORTH_MAGIC_1 )
                    {
                        int count = 0;
                        res = fseek( pFile, -8, SEEK_CUR );
                        nItems = fread( &count, sizeof(count), 1, pFile );
                        if ( (res == 0) && (nItems == 1) )
                        {
                            mpInternalFiles = new sInternalFile[ count ];
                            mInternalFileCount = count;
                            for ( i = 0; i < count; i++ )
                            {
                                mpInternalFiles[i].pName = NULL;
                                mpInternalFiles[i].length = 0;
                            }
                            res = fseek( pFile, -4, SEEK_CUR );
                            for ( i = 0; i < mInternalFileCount; i++ )
                            {
                                // first get the filename length
                                res = fseek( pFile, -4, SEEK_CUR );
                                nItems = fread( &count, sizeof(count), 1, pFile );
                                if ( (res != 0) || (nItems != 1) )
                                {
                                    break;
                                }
                                res = fseek( pFile, -(count + 8), SEEK_CUR );
                                // filepos is at start of token2, followed by filename
                                int offset = ftell( pFile );
                                // check for magic token2
                                nItems = fread( &token, sizeof(token), 1, pFile );
                                if ( (res != 0) || (nItems != 1) || (token != FORTH_MAGIC_2) )
                                {
                                    break;
                                }
                                // filepos is at filename
                                mpInternalFiles[i].pName = new char[ count + 1 ];
                                mpInternalFiles[i].pName[count] = '\0';
                                nItems = fread( mpInternalFiles[i].pName, count, 1, pFile );
                                if ( (res != 0) || (nItems != 1) )
                                {
                                    delete [] mpInternalFiles[i].pName;
                                    mpInternalFiles[i].pName = NULL;
                                    break;
                                }
                                // seek back to data length just before filename
                                res = fseek( pFile, (offset - 4), SEEK_SET );
                                nItems = fread( &count, sizeof(count), 1, pFile );
                                if ( (res != 0) || (nItems != 1) )
                                {
                                    break;
                                }
                                res = fseek( pFile, -(count + 4), SEEK_CUR );
                                if ( res != 0 )
                                {
                                    break;
                                }
                                // filepos is at start of data section
                                mpInternalFiles[i].offset = ftell( pFile );
                                mpInternalFiles[i].length = count;
                                SPEW_SHELL( "Found file %s, %d bytes at 0x%x\n", mpInternalFiles[i].pName,
                                        count, mpInternalFiles[i].offset );
                            }
                        }
                    }
                }
            }
        }
    }
}


void
Shell::SetEnvironmentVars( const char ** envp )
{
    int i, nameLen;
    const char *pValue;
    std::string tempDir;

    if (envp == nullptr)
    {
        return;
    }
    DeleteEnvironmentVars();
    mScriptPaths.clear();
    mDllPaths.clear();
    mResourcePaths.clear();

    // make copies of vars
    i = 0;
    std::vector<std::string> tokens;
    while (envp[i] != nullptr)
    {
        std::string envString = envp[i];
        tokens.clear();
        splitString(envString, tokens, '=');

        if (tokens.size() == 2)
        {
            std::string& varName = tokens[0];
            std::string& varValue = tokens[1];
            mEnvVarNames.push_back(varName);
            mEnvVarValues.push_back(varValue);
            size_t valueLen = varValue.size();
            if (varName == "FORTH_ROOT")
            {
                mSystemDir.assign(varValue);
            }
#if defined(FORTH64)
            else if (varName == "FORTH_DLL64")
#else
            else if (varName == "FORTH_DLL32")
#endif
            {
                // ignore paths already set from FORTH_DLL
                mDllPaths.clear();
                splitString(varValue, mDllPaths, ';');
            }
            else if (varName == "FORTH_DLL")
            {
                // if DLL paths have already been set by FORTH_DLL64 or FORTH_DLL32, ignore FORTH_DLL
                if (mDllPaths.empty())
                {
                    splitString(varValue, mDllPaths, ';');
                }
            }
            else if (varName == "FORTH_TEMP")
            {
                mTempDir = varValue;
            }
            else if (varName == "FORTH_BLOCKFILE")
            {
                mBlockfilePath = varValue;
            }
            else if (varName == "FORTH_SCRIPTS")
            {
                splitString(varValue, mScriptPaths, ';');
            }
            else if (varName == "FORTH_RESOURCES")
            {
                splitString(varValue, mResourcePaths, ';');
            }
            else if (varName == "TMP" || varName == "TEMP")
            {
                tempDir = varValue;
            }
        }
        else if (tokens.size() == 1)
        {
            mEnvVarNames.push_back(tokens[0]);
            mEnvVarValues.push_back("");
        }
        i++;
    }

    if (mSystemDir.empty())
    {
        // FORTH_ROOT environment variable is missing, define it as the current working dir
        mSystemDir.assign(mWorkingDirPath);
        mSystemDir.append(PATH_SEPARATOR);
        mEnvVarNames.push_back("FORTH_ROOT");
        mEnvVarValues.push_back(mSystemDir);
    }

    if (mScriptPaths.empty())
    {
        // FORTH_SCRIPTS environment variable is missing, define it as FORTH_ROOT
        std::string path(mSystemDir);
        mScriptPaths.push_back(path);
        // and also add the system subdir to script paths
        path.append("system/");
        mScriptPaths.push_back(path);
    }
    else
    {
        for (std::string& path : mScriptPaths)
        {
            char delim = *PATH_SEPARATOR;
            if (path.at(path.length() - 1) != delim)
            {
                path.append(PATH_SEPARATOR);
            }
        }
    }

    if (mDllPaths.empty())
    {
        // FORTH_DLL environment variable is missing, define it as FORTH_ROOT
        mEnvVarNames.push_back("FORTH_DLL");
        mEnvVarValues.push_back(mSystemDir);
        mDllPaths.push_back(mSystemDir);
    }
    else
    {
        for (std::string& path : mDllPaths)
        {
            char delim = *PATH_SEPARATOR;
            if (path.at(path.length() - 1) != delim)
            {
                path.append(PATH_SEPARATOR);
            }
        }
    }

    if (mResourcePaths.empty())
    {
        // FORTH_RESOURCES environment variable is missing, define it as FORTH_ROOT
        mEnvVarNames.push_back("FORTH_RESOURCES");
        mEnvVarValues.push_back(mSystemDir);
        mResourcePaths.push_back(mSystemDir);
    }
    else
    {
        for (std::string& path : mResourcePaths)
        {
            char delim = *PATH_SEPARATOR;
            if (path.at(path.length() - 1) != delim)
            {
                path.append(PATH_SEPARATOR);
            }
        }
    }

    if (mTempDir.empty())
    {
        // FORTH_TEMP environment variable is missing, if TEMP or TMP environment variables
        // is defined, use those, else define it as FORTH_ROOT
        if (tempDir.empty())
        {
            mTempDir = mSystemDir;
        }
        else
        {
            mTempDir = tempDir + PATH_SEPARATOR;
        }
        mEnvVarNames.push_back("FORTH_TEMP");
        mEnvVarValues.push_back(mTempDir);
    }

    if (mBlockfilePath.empty())
    {
        mBlockfilePath = mSystemDir + "_blocks.blk";
        mEnvVarNames.push_back("FORTH_BLOCKFILE");
        mEnvVarValues.push_back(mBlockfilePath);
    }
}

void
Shell::DeleteCommandLine( void )
{
    mArgs.clear();

    // Huh? why is this here?
    if ( mpInternalFiles != NULL )
    {
        for ( int i = 0; i < mInternalFileCount; i++ )
        {
            if ( mpInternalFiles[i].pName != NULL )
            {
                delete [] mpInternalFiles[i].pName;
            }
        }
        delete [] mpInternalFiles;
        mpInternalFiles = NULL;
    }
    mInternalFileCount = 0;
}


void
Shell::DeleteEnvironmentVars( void )
{
    mEnvVarNames.clear();
    mEnvVarValues.clear();
}


const char*
Shell::GetEnvironmentVar(const char* envVarName)
{
    const char* envVarValue = NULL;
    for (int i = 0; i < mEnvVarNames.size(); i++)
    {
        if (strcmp(envVarName, mEnvVarNames[i].c_str()) == 0)
        {
            return mEnvVarValues[i].c_str();
        }
    }
    return nullptr;
}

const char* Shell::GetArg(int argNum) const
{
    if (argNum < mArgs.size())
    {
        return mArgs[argNum].c_str();
    }
    return nullptr;
}

bool
Shell::CheckSyntaxError(const char *pString, ControlStackTag tag, ucell allowedTags)
{
    ucell tagVal = (ucell)tag;
    bool tagsMatched = ((tagVal & allowedTags) != 0);
	// special case: BranchZ will match either Branch or BranchZ
    /*
	if (!tagsMatched && (desiredTag == kCSTagBranchZ) && (tag == kCSTagBranch))
	{
		tagsMatched = true;
	}
    */
	if (!tagsMatched)
	{
        char* pExpected = (char *) malloc(64);
        char* pActual = (char *) malloc(512);
        mpStack->GetTagString(tagVal, pExpected, 64);
        mpStack->GetTagString(allowedTags, pActual, 512);
		sprintf(mErrorString, "<%s> preceeded by <%s>, was expecting <%s>", pString, pExpected, pActual);
        free(pExpected);
        free(pActual);
		mpEngine->SetError(ForthError::badSyntax, mErrorString);
		return false;
	}
	return true;
}


void
Shell::StartDefinition(const char* pSymbol, ControlStackTag definitionType)
{
    mpStack->Push(definitionType, nullptr, pSymbol);
}


bool Shell::CheckDefinitionEnd(const char* pDisplayName, ucell allowedTags, ControlStackEntry* pEntryOut)
{
    ControlStackEntry* pEntry = mpStack->Peek();
    bool result = false;

    if (pEntry != nullptr && CheckSyntaxError(pDisplayName, pEntry->tag, allowedTags))
    {
        if (pEntryOut != nullptr)
        {
            *pEntryOut = *pEntry;
        }

		if (pEntry->name != nullptr)
		{
			result = true;
		}
	}
    mpStack->Drop();
	return result;
}


ForthFileInterface*
Shell::GetFileInterface()
{
    return &mFileInterface;
}

char
Shell::GetChar()
{
    return getchar();
}

FILE*
Shell::FileOpen( const char* filePath, const char* openMode )
{
	FILE* pFile = NULL;
    pFile = fopen( filePath, openMode );
	return pFile;
}

int
Shell::FileClose( FILE* pFile )
{
    return fclose( pFile );
}

int
Shell::FileSeek( FILE* pFile, int offset, int control )
{
    return fseek( pFile, offset, control );
}

int
Shell::FileRead( FILE* pFile, void* pDst, int itemSize, int numItems )
{
    return (int)fread( pDst, itemSize, numItems, pFile );
}

int
Shell::FileWrite( FILE* pFile, const void* pSrc, int itemSize, int numItems ) 
{
    return (int)fwrite( pSrc, itemSize, numItems, pFile );
}

int
Shell::FileGetChar( FILE* pFile )
{
    return fgetc( pFile );
}

int
Shell::FilePutChar( FILE* pFile, int outChar )
{
    return fputc( outChar, pFile );
}

int
Shell::FileAtEOF( FILE* pFile )
{
    return feof( pFile );
}

int
Shell::FileCheckExists( const char* pFilename )
{
    FILE* pFile = fopen( pFilename, "r" );
    int result = (pFile != NULL) ? ~0 : 0;
    if ( pFile != NULL )
    {
        fclose( pFile );
    }
    return result;
}

int
Shell::FileGetLength( FILE* pFile )
{
    int oldPos = ftell( pFile );
    fseek( pFile, 0, SEEK_END );
    int result = ftell( pFile );
    fseek( pFile, oldPos, SEEK_SET );
    return result;
}

int
Shell::FileGetPosition( FILE* pFile )
{
    return ftell( pFile );
}

char*
Shell::FileGetString( FILE* pFile, char* pBuffer, int maxChars )
{
    return fgets( pBuffer, maxChars, pFile );
}


int
Shell::FilePutString( FILE* pFile, const char* pBuffer )
{
    return fputs( pBuffer, pFile );
}

void*
Shell::ReadDir(void* pDir, void* pEntry)
{
    return readDir(pDir, pEntry);
}

/*int
Shell::FileRemove( Fonst char* pFilename )
{
    return remove( pFilename );
}*/

//
// support for conditional compilation ops
//

void Shell::PoundIf()
{
    //mPoundIfDepth++;

    // set flags so Run will know to check result at end of line
    //   and either continue compiling or skip section
    if ( mpEngine->IsCompiling() )
    {
        // this means set state back to compile at end of line
        mFlags |= SHELL_FLAG_START_IF_C;
    }
    else
    {
        mFlags |= SHELL_FLAG_START_IF_I;
    }

    // evaluate rest of input line
    mpEngine->SetCompileState( 0 );
}


void Shell::PoundIfdef( bool isDefined )
{
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    Vocabulary* pVocab = pOuter->GetSearchVocabulary();
    char* pToken = GetNextSimpleToken();
    if ( (pToken == NULL) || (pVocab == NULL) || ((pVocab->FindSymbol( pToken ) != NULL) != isDefined) )
    {
        // skip to "else" or "endif"
        mpStack->Push(kCSTagPoundIf);
        mFlags |= SHELL_FLAG_SKIP_SECTION;
        mPoundIfDepth = 0;
    }
}


void Shell::PoundElse()
{
    ControlStackEntry* pEntry = mpStack->Peek();
    if ( pEntry->tag == kCSTagPoundIf)
    {
        mFlags |= SHELL_FLAG_SKIP_SECTION;
        // leave the marker for PoundEndif
    }
    else
    {
        // error - unexpected else
        mpEngine->SetError( ForthError::preprocessorError, "unexpected #else" );
        mpStack->Drop();
    }
}


void Shell::PoundEndif()
{
    ControlStackEntry* pEntry = mpStack->Peek();
    if (pEntry->tag != kCSTagPoundIf)
    {
        // error - unexpected endif
        mpEngine->SetError( ForthError::preprocessorError, "unexpected #endif" );
    }
    mpStack->Drop();
}


FILE* Shell::OpenInternalFile( const char* pFilename )
{
    // see if file is an internal file, and if so use it
	FILE* pFile = NULL;
    for ( int i = 0; i < mInternalFileCount; i++ )
    {
        if ( strcmp( mpInternalFiles[i].pName, pFilename ) == 0 )
        {
            // there is an internal file, open this .exe and seek to internal file
            pFile = fopen( mArgs[0].c_str(), "r" );
            if ( fseek( pFile, mpInternalFiles[i].offset, 0 ) != 0 )
            {
                fclose( pFile );
                pFile = NULL;
            }
            break;
        }
    }
	return pFile;
}


FILE* Shell::OpenForthFile(const char* pPath, std::string& containingDir)
{
    containingDir.clear();

    // see if file is an internal file, and if so use it
    FILE *pFile = OpenInternalFile( pPath );
    if (pFile != nullptr)
    {
        SPEW_SHELL("Shell::OpenForthFile found internal file %s\n", pPath);
        return pFile;
    }

    pFile = fopen( pPath, "r" );
    if (pFile != nullptr)
    {
        SPEW_SHELL("Shell::OpenForthFile found %s\n", pPath);
        std::string path(pPath);
        auto pathEnd = path.find_last_of("/\\");
        if (pathEnd != std::string::npos)
        {
            containingDir = path.substr(0, pathEnd);
        }
        return pFile;
    }

	bool pathIsRelative = true;
#if defined( WIN32 )
	if ( strchr( pPath, ':' ) != NULL )
	{
		pathIsRelative = false;
	}
#elif defined(LINUX) || defined(MACOSX)
	if ( *pPath == '/' )
	{
		pathIsRelative = false;
	}
#endif
    if (pathIsRelative)
    {
        if (FindFileInPaths(pPath, mScriptPaths, containingDir))
        {
            std::string path(containingDir);
            path.append(pPath);
            SPEW_SHELL("Shell::OpenForthFile found %s\n", path.c_str());
            pFile = fopen(path.c_str(), "r");
        }
    }
	return pFile;
}

bool Shell::FindFileInPaths(const char* pPath, const std::vector<std::string> paths, std::string& containingDir)
{
    for (const std::string& path : paths)
    {
        std::string leaf(path);
        leaf.append(pPath);
        if (mFileInterface.fileExists(leaf.c_str()))
        {
            containingDir = path;
            std::string inPath(pPath);
            auto pathEnd = inPath.find_last_of("/\\");
            if (pathEnd != std::string::npos)
            {
                containingDir.append(inPath.substr(0, pathEnd));
            }
            return true;
        }
    }

    return false;
}

int32_t Shell::FourCharToLong(const char* pFourCC)
{
	int32_t retVal = 0;
	memcpy(&retVal, pFourCC, sizeof(retVal));
	return retVal;
}

void Shell::GetWorkDir(std::string& dstString)
{
//#define INITIAL_BUFFER_SIZE 128
#define INITIAL_BUFFER_SIZE 4
    char* pBuffer = (char *) malloc(INITIAL_BUFFER_SIZE);
    int actualLen = mFileInterface.getWorkDir(pBuffer, INITIAL_BUFFER_SIZE);
    if (actualLen >= INITIAL_BUFFER_SIZE)
    {
        pBuffer = (char*)realloc(pBuffer, actualLen + 4);
        mFileInterface.getWorkDir(pBuffer, actualLen + 1);
    }
    dstString.assign(pBuffer);
}

void Shell::SetWorkDir(const std::string& workDir)
{
    mFileInterface.setWorkDir(workDir.c_str());
}

void Shell::GetFileLeafName(const char* pPath, std::string& leafOut)
{
#if defined(WINDOWS_BUILD)
    const char* pLeaf = strrchr(pPath, '\\');
#else
    const char* pLeaf = strrchr(pPath, '/');
#endif
    leafOut.assign((pLeaf != nullptr) ? pLeaf : pPath);
}

void Shell::OnForget()
{
    forthop* newDP = mpEngine->GetDP();
    size_t ix = mLoadedFiles.size();
    if (ix)
    {
        while (mLoadedFiles[ix - 1].startDP >= newDP)
        {
            SPEW_SHELL("Shell::OnForget: forgetting %s @ %p\n", mLoadedFiles[ix - 1].filename.c_str(), mLoadedFiles[ix - 1].startDP);
            ix--;
            if (ix == 0)
            {
                break;
            }
        }
        mLoadedFiles.resize(ix);

        for (loadedFileInfo& info : mLoadedFiles)
        {
            SPEW_SHELL("Shell::OnForget: keeping %s @ %p\n", info.filename.c_str(), info.startDP);
        }
    }
}

bool Shell::IsLoaded(std::string& fileName)
{
    for (loadedFileInfo info : mLoadedFiles)
    {
        if (info.filename.compare(fileName) == 0)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     Windows Thread Procedures
// 

#if defined(WIN32)
DWORD WINAPI ConsoleInputThreadRoutine( void* pThreadData )
#elif defined(LINUX) || defined(MACOSX)
uint32_t ConsoleInputThreadRoutine( void* pThreadData )
#endif
{
    Shell* pShell = (Shell *) pThreadData;

#if 0
    return pShell->ConsoleInputLoop();
#else
    return NULL;
#endif
}

