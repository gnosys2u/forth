#pragma once
//////////////////////////////////////////////////////////////////////
//
// Server.h: Forth server support
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
#include "Forth.h"
#include "Engine.h"
#include "Shell.h"
#include "InputStream.h"
#include <string.h>
#if defined(WINDOWS_BUILD)
#include <winsock2.h>
#else
#include <sys/socket.h>
#define SOCKET  int
#endif

int ServerMainLoop( Engine *pEngine, bool doAutoload, unsigned short portNum );

class Pipe;
class Extension;

class ServerInputStream : public InputStream
{
public:
    ServerInputStream( Pipe* pMsgPipe, bool isFile = false, int bufferLen = DEFAULT_INPUT_BUFFER_LEN );
    virtual ~ServerInputStream();

    virtual cell    GetSourceID() const;

    virtual char    *GetLine( const char *pPrompt );
    virtual char    *AddLine();

    virtual bool    IsInteractive( void );

    Pipe*      GetPipe();

    virtual cell*   GetInputState();
    virtual bool    SetInputState(cell* pState);

    virtual InputStreamType GetType(void) const;

protected:
    Pipe*      mpMsgPipe;
    bool            mIsFile;
};



class ServerShell : public Shell
{
public:
    ServerShell( bool doAutoload = true, Engine *pEngine = NULL, Extension *pExtension = NULL, int controlStackLongs = 1024 );
    virtual ~ServerShell();

    virtual int             Run( InputStream *pInputStream );

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
	virtual int				RenameFile( const char* pOldName, const char* pNewName );
	virtual int				RunSystem( const char* pCmdline );
	virtual int				SetWorkDir( const char* pPath );
    virtual int             GetWorkDir(char* pDstPath, int dstPathMax);
    virtual int				MakeDir( const char* pPath, int mode );
	virtual int				RemoveDir( const char* pPath );
	virtual FILE*			GetStdIn();
	virtual FILE*			GetStdOut();
	virtual FILE*			GetStdErr();
	virtual void*			OpenDir( const char* pPath );
	virtual void*			ReadDir( void* pDir, void* pEntry );
	virtual int				CloseDir( void* pDir );
	virtual void			RewindDir( void* pDir );

	int						ProcessConnection( SOCKET serverSocket );
	void					CloseConnection();

protected:
    void    setupFileInterface(bool useLocalFiles);

    Pipe*              mpMsgPipe;
    bool                    mDoAutoload;
    
    bool                    mUseLocalFiles;

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

