#pragma once
//////////////////////////////////////////////////////////////////////
//
// Shell.h: interface for the Shell class.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#include "Engine.h"
#include "InputStream.h"
#include "ControlStack.h"

#if defined(LINUX) || defined(MACOSX)
#include <limits.h>
#define MAX_PATH PATH_MAX
#endif
#include <vector>
#include <string>
#include <map>

class InputStack;
class Extension;
class ExpressionInputStream;
class ParseInfo;
class BlockFileManager;

#if defined(WINDOWS_BUILD)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

//
// the ParseInfo class exists to support fast vocabulary searches.
// these searches are sped up by doing symbol comparisons using longwords
// instead of characters
//
#define TOKEN_BUFF_LONGS    (DEFAULT_INPUT_BUFFER_LEN >> 2)
#define TOKEN_BUFF_CHARS    DEFAULT_INPUT_BUFFER_LEN

// These are the flags that can be passed to Engine::ProcessToken
#define PARSE_FLAG_QUOTED_STRING        1
#define PARSE_FLAG_QUOTED_CHARACTER     2
#define PARSE_FLAG_HAS_PERIOD           4
#define PARSE_FLAG_HAS_COLON            8
#define PARSE_FLAG_FORCE_LONG           16

#define MAX_TOKEN_BYTES     1024

// NUM_FORTH_ENV_VARS is the number of environment variables which forth uses:
//  FORTH_ROOT
//  FORTH_DLL
//  FORTH_TEMP
//  FORTH_BLOCKFILE
#define NUM_FORTH_ENV_VARS 4

typedef std::map<std::string, std::string> EnvironmentMap;

#define SHELL_FLAG_CREATED_ENGINE   1
#define SHELL_FLAG_POP_NEXT_TOKEN   2
#define SHELL_FLAG_START_IF_I       4
#define SHELL_FLAG_START_IF_C       8
#define SHELL_FLAG_START_IF         (SHELL_FLAG_START_IF_I | SHELL_FLAG_START_IF_C)
#define SHELL_FLAG_SKIP_SECTION     16

#define SHELL_FLAGS_NOT_RESET_ON_ERROR (SHELL_FLAG_CREATED_ENGINE)


class Shell  
{
public:

    // if the creator of a Shell passes in non-NULL engine and/or thread params,
    //   that creator is responsible for deleting the engine and/or thread
    Shell(int argc, const char ** argv, const char ** envp, Engine *pEngine = NULL, Extension *pExtension = NULL, int controlStackLongs = 1024);
    virtual ~Shell();

    // returns true IFF file opened successfully
    virtual bool            PushInputFile( const char *pInFileName );
    virtual void            PushInputBuffer( const char *pDataBuffer, int dataBufferLen );
    virtual void            PushInputBlocks(BlockFileManager*  pManager, uint32_t firstBlock, uint32_t lastBlock);
    virtual bool            PopInputStream( void );
    // NOTE: the input stream passed to Run will be deleted by Shell
	virtual int             Run(InputStream *pStream);
	virtual int             RunOneStream(InputStream *pStream);
	char *                  GetNextSimpleToken(void);
    char *                  GetToken( int idelim, bool bSkipLeadingWhiteSpace = true );
    // special kludge to meet ANSI 2012 spec for S\" op
    char*                   GetToken2012(int idelim);

    inline Engine *    GetEngine( void ) { return mpEngine; };
    inline InputStack * GetInput( void ) { return mpInput; };
	inline ControlStack * GetControlStack( void ) { return mpStack; };
    //inline bool             InputLineReadyToProcess() { return !mInContinuationLine; }
    bool                    LineHasContinuation(const char* pBuffer);
    const char*             GetArg(int argNum) const;
    const char*             GetEnvironmentVar(const char* envVarName);
    const std::vector<std::string>& GetCommandLineArgs() { return mArgs; }
    const std::vector<std::string>& GetEnvironmentVarNames() { return mEnvVarNames; }
    const std::vector<std::string>& GetEnvironmentVarValues() { return mEnvVarValues; }
    inline const char*      GetTempDir() const { return mTempDir.c_str(); }
    inline const char*      GetSystemDir() const { return mSystemDir.c_str(); }
    inline const char*      GetBlockfilePath() const { return mBlockfilePath.c_str(); }
    const std::vector<std::string>& GetScriptPaths() { return mScriptPaths; }
    const std::vector<std::string>& GetDllPaths() { return mDllPaths; }
    const std::vector<std::string>& GetResourcePaths() { return mResourcePaths; }

    bool                    CheckSyntaxError(const char *pString, ControlStackTag tag, ucell allowedTags);
	void					StartDefinition(const char*pDefinedSymbol, ControlStackTag definitionType);
	bool					CheckDefinitionEnd(const char* pDisplayName, ucell allowedTags, ControlStackEntry* pEntryOut = nullptr);

    virtual OpResult    InterpretLine();
    virtual OpResult    ProcessLine();
    virtual char            GetChar();

	virtual FILE*			OpenInternalFile( const char* pFilename );
	virtual FILE*			OpenForthFile(const char* pFilename, std::string& containingDir);

    virtual ForthFileInterface* GetFileInterface();

    virtual FILE*           FileOpen( const char* filePath, const char* openMode );
    virtual int             FileClose( FILE* pFile );
    virtual int             FileSeek( FILE* pFile, int offset, int control );
    virtual int             FileRead( FILE* pFile, void* pDst, int itemSize, int numItems );
    virtual int             FileWrite( FILE* pFile, const void* pSrc, int itemSize, int numItems );
    virtual int             FileGetChar( FILE* pFile );
    virtual int             FilePutChar( FILE* pFile, int outChar );
    virtual int             FileAtEOF( FILE* pFile );
    virtual int             FileCheckExists( const char* pFilename );
    virtual int             FileGetLength( FILE* pFile );
    virtual int             FileGetPosition( FILE* pFile );
    virtual char*           FileGetString( FILE* pFile, char* dstBuffer, int maxChars );
    virtual int             FilePutString( FILE* pFile, const char* pBuffer );
    virtual void*			ReadDir(void* pDir, void* pEntry);

    virtual void            PoundIf();
    virtual void            PoundIfdef( bool isDefined );
    virtual void            PoundElse();
    virtual void            PoundEndif();

	static int32_t			FourCharToLong(const char* pFourCC);

    virtual void            GetWorkDir(std::string& dstString);
    virtual void            SetWorkDir(const std::string& workDir);

    bool                    FindFileInPaths(const char* pPath, const std::vector<std::string> paths, std::string& containingDir);

    static void             GetFileLeafName(const char* pPath, std::string& leafOut);
    void                    OnForget();
    bool                    IsLoaded(std::string& fileName);

protected:

    void                    SetCommandLine(int argc, const char ** argv);
    void                    SetEnvironmentVars(const char ** envp);

    // parse next token from input stream into mTokenBuff, padded with 0's up
    // to next longword boundary
    bool                    ParseToken( ParseInfo *pInfo );
    // parse next string from input stream into mTokenBuff, padded with 0's up
    // to next longword boundary
    bool                    ParseString( ParseInfo *pInfo );
    void                    ReportError(void);
    void                    ReportWarning(const char* pMessage);
    void                    ErrorReset(void);

    void                    DeleteEnvironmentVars();
    void                    DeleteCommandLine();

#if 0
    virtual DWORD           TimerThreadLoop();
    virtual DWORD           ConsoleInputThreadLoop();
#endif

    InputStack *       mpInput;
    Engine *           mpEngine;
    ControlStack *       mpStack;
    ForthFileInterface      mFileInterface;
	ExpressionInputStream* mExpressionInputStream;

    int32_t                    mTokenBuffer[ TOKEN_BUFF_LONGS ];

    std::vector<std::string> mArgs;
    std::vector<std::string> mEnvVarNames;
    std::vector<std::string> mEnvVarValues;
    int                     mFlags;
    char                    mErrorString[ 128 ];
    char                    mToken[MAX_TOKEN_BYTES + 1];
    std::string             mTempDir;
    std::string             mSystemDir;
    std::string             mBlockfilePath;
    int                     mPoundIfDepth;

    std::vector<std::string> mScriptPaths;
    std::vector<std::string> mDllPaths;
    std::vector<std::string> mResourcePaths;
    EnvironmentMap          mEnvironment;

#if defined(LINUX) || defined(MACOSX)
#else
    DWORD                   mMainThreadId;
    DWORD                   mConsoleInputThreadId;
    HANDLE                  mConsoleInputThreadHandle;
    HANDLE                  mConsoleInputEvent;
#endif
	char					mWorkingDirPath[MAX_PATH + 1];
#if 0
	ForthThreadQueue*       mpReadyThreads;
    ForthThreadQueue*       mpSleepingThreads;
#endif

    struct sInternalFile
    {
        char*   pName;
        int     length;
        int     offset;
    };
    sInternalFile*          mpInternalFiles;
    int                     mInternalFileCount;

    struct loadedFileInfo {
        std::string     filename;
        forthop*        startDP;
    };
    std::vector<loadedFileInfo> mLoadedFiles;

    bool                    mWaitingForConsoleInput;
    bool                    mConsoleInputReady;
};

