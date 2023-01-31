#pragma once
//////////////////////////////////////////////////////////////////////
//
// Shell.h: interface for the Shell class.
//
//////////////////////////////////////////////////////////////////////

#include "Engine.h"
#include "Input.h"
#if defined(LINUX) || defined(MACOSX)
#include <limits.h>
#define MAX_PATH PATH_MAX
#endif
#include <vector>
#include <string>

class InputStack;
class Extension;
class ExpressionInputStream;

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

typedef enum
{
   kShellTagNothing  = 0x00000001,
   kShellTagDo       = 0x00000002,
   kShellTagBegin    = 0x00000004,
   kShellTagWhile    = 0x00000008,
   kShellTagCase     = 0x00000010,
   kShellTagIf       = 0x00000020,
   kShellTagElse     = 0x00000040,
   kShellTagParen    = 0x00000080,
   kShellTagString   = 0x00000100,
   kShellTagDefine   = 0x00000200,
   kShellTagPoundIf  = 0x00000400,
   kShellTagOf       = 0x00000800,
   kShellTagOfIf     = 0x00001000,
   kShellTagAndIf    = 0x00002000,
   kShellTagOrIf     = 0x00004000,
   kShellTagElif     = 0x00008000,
   kShellTagTry      = 0x00010000,
   kShellTagExcept   = 0x00020000,
   kShellTagFinally  = 0x00040000,
   kShellLastTag = kShellTagFinally  // update this when you add a new tag
   // if you add tags, remember to update TagStrings in Shell.cpp
} eShellTag;

// NUM_FORTH_ENV_VARS is the number of environment variables which forth uses:
//  FORTH_ROOT
//  FORTH_DLL
//  FORTH_TEMP
//  FORTH_BLOCKFILE
#define NUM_FORTH_ENV_VARS 4

class ForthShellStack
{
public:
   ForthShellStack( int stackEntries = 1024 );
   virtual ~ForthShellStack();


   inline forthop**    GetSP(void)           { return mSSP; };
   inline void         SetSP(forthop** pNewSP)   { mSSP = pNewSP; };
   inline ucell        GetSize(void)         { return mSSLen; };
   inline cell         GetDepth(void)        { return mSST - mSSP; };
   inline void         EmptyStack(void)      { mSSP = mSST; };
   // push tag telling what control structure we are compiling (if/else/for/...)
   void         PushTag(eShellTag tag);
   void         PushAddress(forthop* val);
   void         Push(cell val);
   eShellTag    PopTag(void);
   forthop*     PopAddress(void);
   cell         Pop(void);
   eShellTag    PeekTag(int index = 0);
   forthop*     PeekAddress(int index = 0);
   cell         Peek(int index = 0);

   // push a string, this should be followed by a PushTag of a tag which uses this string (such as paren)
   void                PushString(const char *pString);
   // return true IFF item on top of shell stack is a string
   bool                PopString(char *pString, int maxLen);

   void					ShowStack();

protected:
    forthop**           mSSP;       // shell stack pointer
    forthop**           mSSB;       // shell stack base
    forthop**           mSST;       // empty shell stack pointer
	ucell               mSSLen;     // size of shell stack in longwords
	Engine         *mpEngine;
};

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
    Shell(int argc, const char ** argv, const char ** envp, Engine *pEngine = NULL, Extension *pExtension = NULL, int shellStackLongs = 1024);
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
    char *                  GetToken( char delim, bool bSkipLeadingWhiteSpace = true );

    inline Engine *    GetEngine( void ) { return mpEngine; };
    inline InputStack * GetInput( void ) { return mpInput; };
	inline ForthShellStack * GetShellStack( void ) { return mpStack; };
    inline bool             InputLineReadyToProcess() { return !mInContinuationLine; }
    char*                   AddToInputLine(const char* pBuffer);
    inline int              GetArgCount( void ) const { return mNumArgs; };
    inline const char *     GetArg( int argNum ) const { return mpArgs[argNum]; };
    const char*             GetEnvironmentVar(const char* envVarName);
    inline char**           GetEnvironmentVarNames() { return mpEnvVarNames; }
    inline char**           GetEnvironmentVarValues() const { return mpEnvVarValues; }
    inline int              GetEnvironmentVarCount() const { return mNumEnvVars;  }
    inline const char*      GetTempDir() const { return mTempDir; }
    inline const char*      GetSystemDir() const { return mSystemDir; }
    inline const char*      GetDLLDir() const { return mDLLDir; }
    inline const char*      GetBlockfilePath() const { return mBlockfilePath; }

    bool                    CheckSyntaxError(const char *pString, eShellTag tag, int32_t desiredTag);
	void					StartDefinition(const char*pDefinedSymbol, const char* pFourCharCode);
	bool					CheckDefinitionEnd( const char* pDisplayName, const char* pFourCharCode );

    virtual OpResult    InterpretLine( const char *pSrcLine = NULL );
    virtual OpResult    ProcessLine( const char *pSrcLine = NULL );
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
    ForthShellStack *       mpStack;
    ForthFileInterface      mFileInterface;
	ExpressionInputStream* mExpressionInputStream;

    int32_t                    mTokenBuffer[ TOKEN_BUFF_LONGS ];

    int                     mNumArgs;
    char **                 mpArgs;
    int                     mNumEnvVars;
    char **                 mpEnvVarNames;
    char **                 mpEnvVarValues;
    int                     mFlags;
    char                    mErrorString[ 128 ];
    char                    mToken[MAX_TOKEN_BYTES + 1];
    char                    mContinuationBuffer[DEFAULT_INPUT_BUFFER_LEN];
    int                     mContinuationBytesStored;
    char*                   mTempDir;
    char*                   mSystemDir;
    char*                   mDLLDir;
    char*                   mBlockfilePath;
    int                     mPoundIfDepth;

    std::vector<std::string> mScriptPaths;

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

    bool                    mWaitingForConsoleInput;
    bool                    mConsoleInputReady;
    bool                    mInContinuationLine;
};

