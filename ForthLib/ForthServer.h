#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthServer.h: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#define SOCKET  int
#endif

int ForthServerMainLoop( ForthEngine *pEngine, bool doAutoload, unsigned short portNum );

class ForthPipe;
class ForthExtension;

class ForthServerInputStream : public ForthInputStream
{
public:
    ForthServerInputStream( ForthPipe* pMsgPipe, bool isFile = false, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ForthServerInputStream();

    virtual int     GetSourceID();

    virtual char    *GetLine( const char *pPrompt );

    virtual bool    IsInteractive( void );

    ForthPipe*      GetPipe();

    virtual long*   GetInputState();
    virtual bool    SetInputState( long* pState );

protected:
    ForthPipe*      mpMsgPipe;
    bool            mIsFile;
};



class ForthServerShell : public ForthShell
{
public:
    ForthServerShell( bool doAutoload = true, ForthEngine *pEngine = NULL, ForthExtension *pExtension = NULL, ForthThread *pThread = NULL, int shellStackLongs = 1024 );
    virtual ~ForthServerShell();

    virtual int             Run( ForthInputStream *pInputStream );

    virtual bool            PushInputFile( const char *pFileName );
    virtual bool            PopInputStream( void );

    void                    SendTextToClient( const char *pMessage );

    virtual char            GetChar();

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
	virtual int				FileRemove( const char* buffer );
	virtual int				FileDup( int fileHandle );
	virtual int				FileDup2( int srcFileHandle, int dstFileHandle );
	virtual int				FileNo( FILE* pFile );
	virtual int				FileFlush( FILE* pFile );
	virtual char*			GetTmpnam( char* path );
	virtual int				RenameFile( const char* pOldName, const char* pNewName );
	virtual int				RunSystem( const char* pCmdline );
	virtual int				ChangeDir( const char* pPath );
	virtual int				MakeDir( const char* pPath, int mode );
	virtual int				RemoveDir( const char* pPath );
	virtual FILE*			GetStdIn();
	virtual FILE*			GetStdOut();
	virtual FILE*			GetStdErr();
	virtual void*			OpenDir( const char* pPath );
	virtual void*			ReadDir( void* pDir );
	virtual int				CloseDir( void* pDir );
	virtual void			RewindDir( void* pDir );

	int						ProcessConnection( SOCKET serverSocket );
	void					CloseConnection();

protected:

    ForthPipe*              mpMsgPipe;
    bool                    mDoAutoload;

    // mSendLinePending is true IFF server is waiting for a reply to kClientMsgSendLine
    bool                    mSendLinePending;

    // mRequestPending is true IFF server is waiting for a reply to a message other than kClientMsgSendLine
    // mPendingCommand is the message sent to the client (kClientMsg*)
    bool                    mRequestPending;
    int                     mPendingCommand;

#if 0
	// threads which are waiting to send a message to client
    ForthThreadQueue*       mClientWaitingThreads;

    // threads which are ready to run
    ForthThreadQueue*       mReadyThreads;
#endif

	ForthObject				mConsoleOutObject;

    SOCKET					mClientSocket;

};

