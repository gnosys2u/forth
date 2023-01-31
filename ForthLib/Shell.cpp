//////////////////////////////////////////////////////////////////////
//
// Shell.cpp: implementation of the Shell class.
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
#include "Input.h"
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

namespace
{
    const char * TagStrings[] =
    {
        "NOTHING",
        "do",
        "begin",
        "while",
        "case/of",
        "if",
        "else",
        "paren",
        "string",
        "define",
        "poundDirective",
		"of",
		"ofif",
		"andif",
		"orif",
        "]if",
        "try",
        "except",
        "finally",
        nullptr
    };

    void GetTagString( cell tag, char* pMsg )
    {
        bool foundOne = false;
        int mask = 1;
        int index = 0;
        pMsg[0] = '\0';
        while ((tag != 0) && (TagStrings[index] != nullptr))
        {
            if ((mask & tag) != 0)
            {
                if (foundOne)
                {
                    strcat(pMsg, " or ");
                }
                strcat(pMsg, TagStrings[index]);
                foundOne = true;
            }
            index++;
            tag >>= 1;
        }
        if (!foundOne)
        {
            sprintf(pMsg, "UNKNOWN TAG 0x%x", tag);
        }
    }

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

Shell::Shell(int argc, const char ** argv, const char ** envp, Engine *pEngine, Extension *pExtension, int shellStackLongs)
: mpEngine(pEngine)
, mFlags(0)
, mNumArgs(0)
, mpArgs(NULL)
, mNumEnvVars(0)
, mpEnvVarNames(NULL)
, mpEnvVarValues(NULL)
, mPoundIfDepth(0)
, mpInternalFiles(NULL)
, mInternalFileCount(0)
, mExpressionInputStream(NULL)
, mSystemDir(NULL)
, mDLLDir(NULL)
, mTempDir(NULL)
, mBlockfilePath(nullptr)
, mContinuationBytesStored(0)
, mInContinuationLine(false)
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
	mpStack = new ForthShellStack( shellStackLongs );

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
    delete[] mTempDir;
    delete[] mSystemDir;
    delete[] mDLLDir;
    delete[] mBlockfilePath;
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
bool
Shell::PushInputFile( const char *pFilename )
{
    std::string containingDir;
    FILE *pInFile = OpenForthFile( pFilename, containingDir);
    if ( pInFile != NULL )
    {
        //printf("%s is contained in {%s}\n", pFilename, containingDir.c_str());
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
    if (curStream->IsFile())
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
        pBuffer = AddToInputLine(pBuffer);
        if (pBuffer == nullptr)
		{
            bQuit = PopInputStream() || (mpInput->Top() == pOldInput);
		}

		if (!bQuit && !mInContinuationLine)
		{
			result = ProcessLine(pBuffer);

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
			case OpResult::kException:
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
    mpEngine->PushInputFile( autoloadFilename );

    const char* pPrompt = mpEngine->GetFastMode() ? "ok>" : "OK>";
    while ( !bQuit )
    {
        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine(pPrompt);
        pBuffer = AddToInputLine(pBuffer);
        if (pBuffer == nullptr)
        {
            bQuit = PopInputStream();
        }

        if ( !bQuit && !mInContinuationLine)
        {
            result = ProcessLine(pBuffer);

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
            case OpResult::kException:
                // an error has occured, empty input stream stack
                // TODO
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
    
    return retVal;
}

#define CONTINUATION_MARKER "\\+"
#define CONTINUATION_MARKER_LEN 2
char* Shell::AddToInputLine(const char* pBuffer)
{
    char* pResult = nullptr;

    mInContinuationLine = false;
    if (pBuffer != nullptr)
    {
        // add on continuation lines if necessary
        int lineLen = (int)strlen(pBuffer);
        int lenWithoutContinuation = lineLen - CONTINUATION_MARKER_LEN;
        if (lenWithoutContinuation >= 0)
        {
            if (!strcmp(pBuffer + lenWithoutContinuation, CONTINUATION_MARKER))
            {
                // input line ends in continuation marker "\+"
                lineLen = lenWithoutContinuation;
                mInContinuationLine = true;
            }
        }
        int newContinuationBytesStored = mContinuationBytesStored + lineLen;
        if (newContinuationBytesStored < DEFAULT_INPUT_BUFFER_LEN)
        {
            memcpy(&mContinuationBuffer[mContinuationBytesStored], pBuffer, lineLen);
            mContinuationBuffer[mContinuationBytesStored + lineLen] = '\0';
            pResult = &mContinuationBuffer[0];
            mContinuationBytesStored = (mInContinuationLine) ? newContinuationBytesStored : 0;
        }
        else
        {
            // TODO - string would overflow continuation buffer
        }
    }

    return pResult;
}

// ProcessLine is the layer between Run and InterpretLine that implements pound directives
OpResult Shell::ProcessLine( const char *pSrcLine )
{
    OpResult result = OpResult::kOk;

    mInContinuationLine = false;
    const char* pLineBuff = mpInput->GetBufferBasePointer();
    if ( pSrcLine != NULL )
	{
        mpInput->Top()->StuffBuffer( pSrcLine );
	}

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
					cell marker = mpStack->Pop();
					if (marker != kShellTagPoundIf)
					{
						// error - unexpected else
						mpEngine->SetError(ForthError::kBadPreprocessorDirective, "unexpected #endif");
					}
					mFlags &= ~SHELL_FLAG_SKIP_SECTION;
                }
                else
                {
                    mPoundIfDepth--;
                }

            }
        }
        if (mpEngine->GetError() != ForthError::kNone)
        {
            result = OpResult::kError;
        }
    }
    else
    {
        // we are currently not skipping input lines
        result = InterpretLine();

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
                    mpStack->PushTag(kShellTagPoundIf);
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
                    mpEngine->SetError( ForthError::kBadPreprocessorDirective, "#if expression left empty stack" );
                }
                mFlags &= ~SHELL_FLAG_START_IF;
            }
        }
    }
    return result;
}

static bool gbCatchExceptions = false;
//
// return true IFF the forth shell should exit
//
OpResult Shell::InterpretLine( const char *pSrcLine )
{
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    OpResult  result = OpResult::kOk;
    bool bLineEmpty;
    ParseInfo parseInfo( mTokenBuffer, sizeof(mTokenBuffer) >> 2 );

    const char *pLineBuff;

    // TODO: set exit code on exit due to error

    pLineBuff = mpInput->GetBufferBasePointer();
    if ( pSrcLine != NULL )
	{
		mpInput->Top()->StuffBuffer( pSrcLine );
	}
	SPEW_SHELL( "\n*** InterpretLine {%s}\n", pLineBuff );
    bLineEmpty = false;
    mpEngine->SetError( ForthError::kNone );
    while ( !bLineEmpty && (result == OpResult::kOk) )
	{
        bLineEmpty = ParseToken( &parseInfo );
        if (mpEngine->GetError() != ForthError::kNone)
        {
            result = OpResult::kError;
        }
        InputStream* pInput = mpInput->Top();
        SPEW_SHELL("input %s:%s[%d] buffer 0x%x readoffset %d write %d\n", pInput->GetType(), pInput->GetName(),
            pInput->GetLineNumber(), pInput->GetBufferPointer(), pInput->GetReadOffset(), pInput->GetWriteOffset() );

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
					result = OpResult::kException;
					mpEngine->SetError( ForthError::kIllegalOperation );
                    mInContinuationLine = false;
				}
			}
			else
			{
                result = pOuter->ProcessToken( &parseInfo );
                CHECK_STACKS( mpEngine->GetMainFiber() );
			}
#else
            result = pOuter->ProcessToken( &parseInfo );
            CHECK_STACKS( mpEngine->GetMainFiber() );
#endif
            if ( result == OpResult::kOk )
			{
                result = mpEngine->CheckStacks();
            }
        }

        if (result != OpResult::kOk)
        {
            // in case console out was redirected, point it back to the user-visible console
            mpEngine->ResetConsoleOut();

            bool exitingShell = (result == OpResult::kExitShell) || (result == OpResult::kShutdown);
            if (!exitingShell)
            {
                ReportError();
                if (mpEngine->GetError() == ForthError::kUnknownSymbol)
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

    mpEngine->GetErrorString( errorBuf1, sizeof(errorBuf1) );
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
    pLastInputToken = mpInput->GetBufferPointer();
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
    pLastInputToken = mpInput->GetBufferPointer();
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

        pSrc = mpInput->GetBufferPointer();
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

        // support C++ end-of-line style comments
        if ( (*pSrc == '/') && (pSrc[1] == '/') && pOuter->CheckFeature( kFFDoubleSlashComment ) )
        {
            return true;
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

        mpInput->SetBufferPointer( (char *) pEndSrc );
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
        // previous symbol ended in ")", so next token to process is on top of shell stack
        mFlags &= ~SHELL_FLAG_POP_NEXT_TOKEN;
        if ( CheckSyntaxError( ")", mpStack->PopTag(), kShellTagParen ) )
        {
            cell tag = mpStack->Peek();
            if ( mpStack->PopString( pInfo->GetToken(), pInfo->GetMaxChars() ) )
            {
                 pInfo->SetToken();
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
                GetTagString(tag, pTagString);
                sprintf(mErrorString,  "top of shell stack is <%s>, was expecting <string>", pTagString);
                free(pTagString);
                mpEngine->SetError( ForthError::kBadSyntax, mErrorString );
            }
        }
        return false;
    }

    while ( !gotAToken )
    {
        bool advanceInputBuffer = true;

		const char* pSrcLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();
		pSrc = mpInput->GetBufferPointer();
        pDst = pInfo->GetToken();
        if ( pSrc >= pSrcLimit )
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

        // support C++ end-of-line style comments
        if ( (*pSrc == '/') && (pSrc[1] == '/') && pOuter->CheckFeature( kFFDoubleSlashComment ) )
        {
            return true;
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

           case '`':
              // support C-style quoted characters like `a` or `\n`
              if ( pOuter->CheckFeature( kFFCCharacterLiterals ) )
              {
				  pEndSrc = pInfo->ParseSingleQuote(pSrc, pSrcLimit, mpEngine);
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
                     gotAToken = true;
                     pEndSrc++;
                     break;

                  case '(':
                     if ((pEndSrc == pSrc) || pOuter->CheckFeature(kFFParenIsComment))
                     {
                         // paren at start of token is part of token (allows old forth-style inline comments to work)
                         if (pOuter->CheckFeature(kFFParenIsComment) == 0)
                         {
                             ReportWarning("Possibly misplaced parentheses");
                         }
                         *pDst++ = *pEndSrc++;
                     }
                     else if (pOuter->CheckFeature(kFFParenIsExpression))
                     {
                         // push accumulated token (if any) onto shell stack
						 advanceInputBuffer = false;
						 if (mExpressionInputStream == NULL)
						 {
							 mExpressionInputStream = new ExpressionInputStream;
						 }
						 pInfo->SetAllFlags(0);
						 mpInput->SetBufferPointer(pSrc);
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
                         mpStack->PushTag( kShellTagParen );*/
                     }
					 else
					 {
						 *pDst++ = *pEndSrc++;
					 }
                     break;

                  case ')':
                     if ( pOuter->CheckFeature( kFFParenIsComment ) )
                     {
                        *pDst++ = *pEndSrc++;
                     }
					 else if (pOuter->CheckFeature(kFFParenIsExpression))
					 {
                        // process accumulated token (if any), pop shell stack, compile/interpret if not empty
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
			mpInput->SetBufferPointer((char *)pEndSrc);
		}
    }   // while ! gotAToken

    return false;
}


char *
Shell::GetNextSimpleToken( void )
{
	while (mpInput->IsEmpty() && mpInput->Top()->IsGenerated())
	{
		SPEW_SHELL("GetNextSimpleToken: %s:%s empty, popping\n", mpInput->Top()->GetType(), mpInput->Top()->GetName());
		if (mpInput->PopInputStream())
		{
			// no more input streams available
			return NULL;
		}
	}
    const char *pEndToken = mpInput->GetBufferPointer();
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
    mpInput->SetBufferPointer( pEndToken );

    //SPEW_SHELL( "GetNextSimpleToken: |%s|%s|\n", &mToken[0], pEndToken );
    return &mToken[0];
}


char *
Shell::GetToken( char delim, bool bSkipLeadingWhiteSpace )
{
	while (mpInput->IsEmpty() && mpInput->Top()->IsGenerated())
	{
		SPEW_SHELL("GetToken: %s %s empty, popping\n", mpInput->Top()->GetType(), mpInput->Top()->GetName());
		if (mpInput->PopInputStream())
		{
			// no more input streams available
			return NULL;
		}
	}
	const char *pEndToken = mpInput->GetBufferPointer();
    const char* pTokenLimit = mpInput->GetBufferBasePointer() + mpInput->GetWriteOffset();
    char c;
    bool bDone;
    char* pDst = &mToken[0];

    if ( bSkipLeadingWhiteSpace )
    {
        // eat any leading white space
        while ( (*pEndToken == ' ') || (*pEndToken == '\t') )
        {
            pEndToken++;
        }
    }

    bDone = false;
    while ( !bDone && (pEndToken <= pTokenLimit) )
    {
        c = *pEndToken++;
        if ( c == delim )
        {
            bDone = true;
        }
        else
        {
            *pDst++ = c;
        }
    }
    *pDst++ = '\0';
    mpInput->SetBufferPointer( pEndToken );

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

    i = 0;
    mpArgs = new char *[ argc ];
    while ( i < argc )
    {
        len = (int)strlen( argv[i] ) + 1;
        mpArgs[i] = new char [ len ];
        strcpy( mpArgs[i], argv[i] );
        i++;
    }
    mNumArgs = argc;

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
    char *pValue;

    if (envp == nullptr)
    {
        return;
    }
    DeleteEnvironmentVars();

    // count number of environment variables
    mNumEnvVars = 0;
    while ( envp[mNumEnvVars] != NULL )
    {
        mNumEnvVars++;
    }
    // leave room for 3 environment vars we may need to add: FORTH_ROOT, FORTH_TEMP and FORTH_BLOCKFILE
    mpEnvVarNames = new char *[mNumEnvVars + NUM_FORTH_ENV_VARS];
    mpEnvVarValues = new char *[mNumEnvVars + NUM_FORTH_ENV_VARS];
    const char* tempDir = NULL;

    // make copies of vars
    i = 0;
    while ( i < mNumEnvVars )
    {
        nameLen = (int)strlen( envp[i] ) + 1;
        mpEnvVarNames[i] = new char[nameLen];
        strcpy( mpEnvVarNames[i], envp[i] );
        pValue = strchr( mpEnvVarNames[i], '=' );
        if ( pValue != NULL )
        {
            *pValue++ = '\0';
            mpEnvVarValues[i] = pValue;
            size_t valueLen = strlen(pValue);
            if (strcmp(mpEnvVarNames[i], "FORTH_ROOT") == 0)
            {
                mSystemDir = new char[valueLen + 1];
                strcpy(mSystemDir, pValue);
            }
            else if (strcmp(mpEnvVarNames[i], "FORTH_DLL") == 0)
            {
                mDLLDir = new char[valueLen + 1];
                strcpy(mDLLDir, pValue);
            }
            else if (strcmp(mpEnvVarNames[i], "FORTH_TEMP") == 0)
            {
                mTempDir = new char[valueLen + 1];
                strcpy(mTempDir, pValue);
            }
            else if (strcmp(mpEnvVarNames[i], "FORTH_BLOCKFILE") == 0)
            {
                mBlockfilePath = new char[valueLen + 1];
                strcpy(mBlockfilePath, pValue);
            }
            else if (strcmp(mpEnvVarNames[i], "FORTH_SCRIPTS") == 0)
            {
                char* pToken = strtok(pValue, ";");
                if (pToken == nullptr)
                {
                    // scripts path has just one entry
                    mScriptPaths.emplace_back(pValue);
                }
                else
                {
                    while (pValue != nullptr)
                    {
                        mScriptPaths.emplace_back(pValue);
                        pValue = strtok(nullptr, ";");
                    }
                }
            }
            else if (strcmp(mpEnvVarNames[i], "TMP") == 0)
            {
                tempDir = mpEnvVarValues[i];
            }
            else if (strcmp(mpEnvVarNames[i], "TEMP") == 0)
            {
                tempDir = mpEnvVarValues[i];
            }
        }
        else
        {
            printf( "Malformed environment variable: %s\n", envp[i] );
        }
        i++;
    }

    if (mSystemDir == nullptr)
    {
        mSystemDir = new char[strlen(mWorkingDirPath) + 2];
        strcpy(mSystemDir, mWorkingDirPath);
        strcat(mSystemDir, PATH_SEPARATOR);
        mpEnvVarNames[mNumEnvVars] = new char[16];
        strcpy(mpEnvVarNames[mNumEnvVars], "FORTH_ROOT");
        mpEnvVarValues[mNumEnvVars] = mSystemDir;
        mNumEnvVars++;
    }

    if (mScriptPaths.size() == 0)
    {
        std::string path(mSystemDir);
        mScriptPaths.push_back(path);
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

    if (mDLLDir == nullptr)
    {
        mDLLDir = new char[strlen(mSystemDir) + 2];
        strcpy(mDLLDir, mSystemDir);
        mpEnvVarNames[mNumEnvVars] = new char[16];
        strcpy(mpEnvVarNames[mNumEnvVars], "FORTH_DLL");
        mpEnvVarValues[mNumEnvVars] = mDLLDir;
        mNumEnvVars++;
    }

    if (mTempDir == nullptr)
    {
        if (tempDir == nullptr)
        {
            mTempDir = new char[strlen(mSystemDir) + 1];
            strcpy(mTempDir, mSystemDir);
        }
        else
        {
            mTempDir = new char[strlen(tempDir) + 2];
            strcpy(mTempDir, tempDir);
            strcat(mTempDir, PATH_SEPARATOR);
        }
        mpEnvVarNames[mNumEnvVars] = new char[16];
        strcpy(mpEnvVarNames[mNumEnvVars], "FORTH_TEMP");
        mpEnvVarValues[mNumEnvVars] = mTempDir;
        mNumEnvVars++;
    }

    if (mBlockfilePath == nullptr)
    {
        if (tempDir == NULL)
        {
            tempDir = mSystemDir;
        }
        mBlockfilePath = new char[strlen(mSystemDir) + 16];
        strcpy(mBlockfilePath, mSystemDir);
        strcat(mBlockfilePath, "_blocks.blk");
        mpEnvVarNames[mNumEnvVars] = new char[20];
        strcpy(mpEnvVarNames[mNumEnvVars], "FORTH_BLOCKFILE");
        mpEnvVarValues[mNumEnvVars] = mBlockfilePath;
        mNumEnvVars++;
    }
}

void
Shell::DeleteCommandLine( void )
{
    while ( mNumArgs > 0 )
    {
        mNumArgs--;
        delete [] mpArgs[mNumArgs];
    }
    delete [] mpArgs;

    mpArgs = NULL;

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
    while ( mNumEnvVars > 0 )
    {
        mNumEnvVars--;
        delete [] mpEnvVarNames[mNumEnvVars];
    }
    delete [] mpEnvVarNames;
    delete [] mpEnvVarValues;

    mpEnvVarNames = NULL;
    mpEnvVarValues = NULL;
}


const char*
Shell::GetEnvironmentVar(const char* envVarName)
{
    const char* envVarValue = NULL;
    for (int i = 0; i < mNumEnvVars; i++)
    {
        if (strcmp(envVarName, mpEnvVarNames[i]) == 0)
        {
            return mpEnvVarValues[i];
        }
    }
    return NULL;
}


bool
Shell::CheckSyntaxError(const char *pString, eShellTag tag, int32_t desiredTags)
{
    int32_t tagVal = (int32_t)tag;
    bool tagsMatched = ((tagVal & desiredTags) != 0);
	// special case: BranchZ will match either Branch or BranchZ
    /*
	if (!tagsMatched && (desiredTag == kShellTagBranchZ) && (tag == kShellTagBranch))
	{
		tagsMatched = true;
	}
    */
	if (!tagsMatched)
	{
        char* pExpected = (char *) malloc(32);
        char* pActual = (char *) malloc(512);
        GetTagString(tagVal, pExpected);
        GetTagString(desiredTags, pActual);
		sprintf(mErrorString, "<%s> preceeded by <%s>, was expecting <%s>", pString, pExpected, pActual);
        free(pExpected);
        free(pActual);
		mpStack->PushTag(tag);
		mpEngine->SetError(ForthError::kBadSyntax, mErrorString);
		return false;
	}
	return true;
}


void
Shell::StartDefinition(const char* pSymbol, const char* pFourCharCode)
{
	mpStack->PushString(pSymbol);
	mpStack->Push(FourCharToLong(pFourCharCode));
    mpStack->PushTag(kShellTagDefine);
}


bool
Shell::CheckDefinitionEnd(const char* pDisplayName, const char* pFourCharCode)
{
    eShellTag defineTag = mpStack->PopTag();

	if (CheckSyntaxError(pDisplayName, defineTag, kShellTagDefine))
	{
		cell defineType = mpStack->Pop();
		int32_t expectedDefineType = FourCharToLong(pFourCharCode);
        char definedSymbol[128];
		definedSymbol[0] = '\0';
		bool gotString = mpStack->PopString(definedSymbol, sizeof(definedSymbol) - 1);

		if (gotString && (defineType == expectedDefineType))
		{
			return true;
		}

		char actualType[8];
		memcpy(actualType, &defineType, sizeof(defineType));
		actualType[4] = '\0';
		sprintf(mErrorString, "at end of <%s> definition of {%s}, got <%s>, was expecting <%s>",
			pDisplayName, definedSymbol, actualType, pFourCharCode);

		mpStack->PushString(definedSymbol);
		mpStack->Push(defineType);
		mpStack->Push(defineTag);
		mpEngine->SetError(ForthError::kBadSyntax, mErrorString);
	}
	return false;
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
        mpStack->PushTag(kShellTagPoundIf);
        mFlags |= SHELL_FLAG_SKIP_SECTION;
        mPoundIfDepth = 0;
    }
}


void Shell::PoundElse()
{
    cell marker = mpStack->Pop();
    if ( marker == kShellTagPoundIf )
    {
        mFlags |= SHELL_FLAG_SKIP_SECTION;
        // put the marker back for PoundEndif
        mpStack->Push( marker );
    }
    else
    {
        // error - unexpected else
        mpEngine->SetError( ForthError::kBadPreprocessorDirective, "unexpected #else" );
    }
}


void Shell::PoundEndif()
{
    cell marker = mpStack->Pop();
    if ( marker != kShellTagPoundIf )
    {
        // error - unexpected endif
        mpEngine->SetError( ForthError::kBadPreprocessorDirective, "unexpected #endif" );
    }
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
            pFile = fopen( mpArgs[0], "r" );
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
        return pFile;
    }

    pFile = fopen( pPath, "r" );
    if (pFile != nullptr)
    {
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
        for (std::string& path : mScriptPaths)
        {
            std::string leaf(path);
            leaf.append(pPath);
            pFile = fopen(leaf.c_str(), "r");
            if (pFile != nullptr)
            {
                containingDir = path;
                std::string inPath(pPath);
                auto pathEnd = inPath.find_last_of("/\\");
                if (pathEnd != std::string::npos)
                {
                    containingDir.append(inPath.substr(0, pathEnd));
                }
                break;
            }
        }
    }
	return pFile;
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

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthShellStack
// 

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#define GAURD_AREA 4

ForthShellStack::ForthShellStack( int numLongs )
: mSSLen( numLongs )
{
   mSSB = new forthop*[mSSLen + (GAURD_AREA * 2)];
   mSSB += GAURD_AREA;
   mSST = mSSB + mSSLen;
   EmptyStack();
}

ForthShellStack::~ForthShellStack()
{
   delete [] (mSSB - GAURD_AREA);
}


void
ForthShellStack::PushTag(eShellTag tag)
{
    char tagString[256];
    if (mSSP > mSSB)
    {
        *--mSSP = (forthop*) tag;
        if (Engine::GetInstance()->GetTraceFlags() & kLogShell)
        {
            GetTagString(tag, tagString);
            SPEW_SHELL("ShellStack: pushed tag %s 0x%08x\n", tagString, (int32_t) tag);
        }
    }
    else
    {
        Engine::GetInstance()->SetError(ForthError::kShellStackOverflow);
    }
}

void
ForthShellStack::Push(cell val)
{
    if (mSSP > mSSB)
    {
        *--mSSP = (forthop*) val;
        SPEW_SHELL("ShellStack: pushed value 0x%08x\n", val);
    }
    else
    {
        Engine::GetInstance()->SetError(ForthError::kShellStackOverflow);
    }
}

void
ForthShellStack::PushAddress(forthop* addr)
{
    if (mSSP > mSSB)
    {
        *--mSSP = addr;
        SPEW_SHELL("ShellStack: pushed address 0x%08x\n", addr);
    }
    else
    {
        Engine::GetInstance()->SetError(ForthError::kShellStackOverflow);
    }
}

static bool mayBeAShellTag(cell tag)
{
    // we assume this is a shell tag if it has exactly one bit set
    bool couldBeATag = false;
    if (tag <= kShellLastTag)
    {
        if ((tag & kShellTagNothing) == 0)
        {
            tag >>= 1;
            while (tag != 0)
            {
                if ((tag & 1) != 0)
                {
                    couldBeATag = ((tag >> 1) == 0);
                    break;
                }
                tag >>= 1;
            }
        }
    }
    return couldBeATag;
}

forthop* ForthShellStack::PopAddress( void )
{
    if (mSSP == mSST)
    {
        Engine::GetInstance()->SetError( ForthError::kShellStackUnderflow );
        return (forthop*)kShellTagNothing;
    }
    forthop* pOp = *mSSP++;
#ifdef TRACE_SHELL
    char tagString[256];
    if (Engine::GetInstance()->GetTraceFlags() & kLogShell)
    {
        if (mayBeAShellTag((cell)pOp))
        {
            GetTagString((cell)pOp, tagString);
            SPEW_SHELL("ShellStack: popped Tag %s\n", tagString);
        }
        else
        {
            SPEW_SHELL("ShellStack: popped address 0x%08x\n", pOp);
        }
    }
#endif
    return pOp;
}

cell ForthShellStack::Pop(void)
{
    if (mSSP == mSST)
    {
        Engine::GetInstance()->SetError(ForthError::kShellStackUnderflow);
        return kShellTagNothing;
    }
    cell val = (cell)*mSSP++;
#ifdef TRACE_SHELL
    char tagString[256];
    if (Engine::GetInstance()->GetTraceFlags() & kLogShell)
    {
        if (mayBeAShellTag(val))
        {
            GetTagString(val, tagString);
            SPEW_SHELL("ShellStack: popped Tag %s\n", tagString);
        }
        else
        {
            SPEW_SHELL("ShellStack: popped value 0x%08x\n", val);
        }
    }
#endif
    return val;
}

eShellTag ForthShellStack::PopTag(void)
{
    return (eShellTag)Pop();
}

cell
ForthShellStack::Peek( int index )
{
    if ( (mSSP + index) >= mSST )
    {
        return kShellTagNothing;
    }
    return (cell)mSSP[index];
}

forthop*
ForthShellStack::PeekAddress(int index)
{
    if ((mSSP + index) >= mSST)
    {
        return nullptr;
    }
    return mSSP[index];
}

eShellTag ForthShellStack::PeekTag(int index)
{
    if ((mSSP + index) >= mSST)
    {
        return kShellTagNothing;
    }
    return (eShellTag) ((cell)mSSP[index]);
}

void
ForthShellStack::PushString( const char *pString )
{
    int len = (int)strlen( pString );
    mSSP -= (len >> CELL_SHIFT) + 1;
	if ( mSSP > mSSB )
	{
		strcpy( (char *) mSSP, pString );
		SPEW_SHELL( "Pushed String \"%s\"\n", pString );
        PushTag(kShellTagString);
	}
	else
	{
        Engine::GetInstance()->SetError( ForthError::kShellStackOverflow );
	}
}

bool
ForthShellStack::PopString(char *pString, int maxLen)
{
    eShellTag topTag = PeekTag();
    if (topTag != kShellTagString )
    {
        *pString = '\0';
        SPEW_SHELL( "Failed to pop string\n" );
        Engine::GetInstance()->SetError( ForthError::kShellStackUnderflow );
        return false;
    }
    mSSP++;
    int len = (int)strlen( (char *) mSSP );
	if (len > (maxLen - 1))
	{
		// TODO: warn about truncating symbol
		len = maxLen - 1;
	}
    memcpy( pString, (char *) mSSP, len );
	pString[len] = '\0';
    mSSP += (len >> CELL_SHIFT) + 1;
    SPEW_SHELL( "Popped Tag string\n" );
    SPEW_SHELL( "Popped String \"%s\"\n", pString );
    return true;
}

void
ForthShellStack::ShowStack()
{
	forthop** pSP = mSSP;
    char* buff = (char *)malloc(512);

	Engine::GetInstance()->ConsoleOut("Shell Stack:\n");
	
	while (pSP != mSST)
	{
		forthop* tag = *pSP++;
        sprintf(buff, "%16p   ", tag);
        Engine::GetInstance()->ConsoleOut(buff);
        // TODO!
        int32_t tagChars = static_cast<int32_t>(reinterpret_cast<intptr_t>(tag));
        for (int i = 0; i < 4; ++i)
        {
            char ch = (char)(tagChars & 0x7f);
            tagChars >>= 8;
            if ((ch < 0x20) || (ch >= 0x7f))
            {
                ch = '.';
            }
            buff[i] = ch;
        }
        buff[4] = ' ';
        buff[5] = ' ';
        buff[6] = '\0';
        Engine::GetInstance()->ConsoleOut(buff);
        if (mayBeAShellTag((cell)tag))
        {
            GetTagString((cell)tag, buff);
            Engine::GetInstance()->ConsoleOut(buff);
        }
        Engine::GetInstance()->ConsoleOut("\n");
    }
    free(buff);
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
