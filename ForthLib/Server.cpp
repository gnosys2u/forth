//////////////////////////////////////////////////////////////////////
//
// Server.cpp: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <filesystem>
#include <iostream>

#ifdef WIN32
//#include <winsock2.h>
//#include <windows.h>
#include <ws2tcpip.h>
#include <io.h>
#include "sys/dirent.h"
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#if defined(MACOSX)
#include <sys/uio.h>
#else
#ifndef RASPI
#include <sys/io.h>
#endif
#endif
#include <dirent.h>
#endif

#include "Server.h"
#include "Pipe.h"
#include "ForthMessages.h"
#include "Extension.h"
#include "InputStack.h"

#ifndef SOCKADDR
#define SOCKADDR struct sockaddr
#endif

using namespace std;

#ifdef DEBUG_WITH_NDS_EMULATOR
#include <nds.h>
#endif

namespace
{
    void consoleOutToClient( CoreState   *pCore,
							 void             *pData,
                             const char       *pMessage )
    {
    #ifdef DEBUG_WITH_NDS_EMULATOR
	    iprintf( "%s", pMessage );
    #else
        ServerShell* pShell = (ServerShell *) (((Engine *)(pCore->pEngine))->GetShell());
        pShell->SendTextToClient( pMessage );
    #endif
    }

    FILE* fileOpen( const char* pPath, const char* pAccess )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileOpen( pPath, pAccess );
    }

    int fileClose( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileClose( pFile );
    }

    size_t fileRead( void* data, size_t itemSize, size_t numItems, FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return (size_t)pShell->FileRead( pFile, data, (int)itemSize, (int)numItems );
    }

    size_t fileWrite( const void* data, size_t itemSize, size_t numItems, FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return (size_t)pShell->FileWrite( pFile, data, (int)itemSize, (int)numItems );
    }

    int fileGetChar( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileGetChar( pFile );
    }

    int filePutChar( int val, FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FilePutChar( pFile, val );
    }

    int fileAtEnd( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileAtEOF( pFile );
    }

    int fileExists( const char* pPath )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileCheckExists( pPath );
    }

    int fileSeek( FILE* pFile, long offset, int ctrl )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileSeek( pFile, offset, ctrl );
    }

    long fileTell( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileGetPosition( pFile );
    }

    int32_t fileGetLength( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileGetLength( pFile );
    }

    char* fileGetString( char* buffer, int bufferLength, FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileGetString( pFile, buffer, bufferLength );
    }

    int filePutString( const char* buffer, FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FilePutString( pFile, buffer );
    }

	int fileRemove( const char* buffer )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileRemove( buffer );
    }

	int fileDup( int fileHandle )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileDup( fileHandle );
    }

	int fileDup2( int srcFileHandle, int dstFileHandle )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileDup2( srcFileHandle, dstFileHandle );
    }

	int fileNo( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileNo( pFile );
    }

	int fileFlush( FILE* pFile )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->FileFlush( pFile );
    }

	int renameFile( const char* pOldName, const char* pNewName )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->RenameFile( pOldName, pNewName );
    }

	int runSystem( const char* pCmdline )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->RunSystem( pCmdline );
    }

	int setWorkDir( const char* pPath )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->SetWorkDir( pPath );
    }

    int getWorkDir(char* pDstPath, int dstPathMax)
    {
        ServerShell* pShell = (ServerShell*)(Engine::GetInstance()->GetShell());
        return pShell->GetWorkDir(pDstPath, dstPathMax);
    }

	int makeDir( const char* pPath, int mode )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->MakeDir( pPath, mode );
    }

	int removeDir( const char* pPath )
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->RemoveDir( pPath );
    }

	FILE* getStdIn()
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->GetStdIn();
    }

	FILE* getStdOut()
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->GetStdOut();
    }

	FILE* getStdErr()
    {
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->GetStdErr();
    }

	void* serverOpenDir( const char* pPath )
	{
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->OpenDir( pPath );
	}

	void* serverReadDir( void* pDir, void* pEntry )
	{
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->ReadDir( pDir, pEntry );
	}

	int serverCloseDir( void* pDir )
	{
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        return pShell->CloseDir( pDir );
	}

	void serverRewindDir( void* pDir )
	{
        ServerShell* pShell = (ServerShell *) (Engine::GetInstance()->GetShell());
        pShell->RewindDir( pDir );
	}

	// trace output in client/server mode
	void serverTraceOutRoutine(void *pData, const char* pFormat, va_list argList)
	{
		(void)pData;

		Engine* pEngine = Engine::GetInstance();
		if ((pEngine->GetTraceFlags() & kLogToConsole) != 0)
		{
			vprintf(pFormat, argList);
		}
#if !defined(LINUX) && !defined(MACOSX)
		else
		{
			TCHAR buffer[1000];
            StringCchVPrintfA(buffer, sizeof(buffer), pFormat, argList);

			OutputDebugString(buffer);
		}
#endif
	}


};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int ServerMainLoop( Engine *pEngine, bool doAutoload, unsigned short portNum )
{
    SOCKET ServerSocket;
    struct sockaddr_in ServerInfo;
    int iRetVal = 0;
    ServerShell *pShell;

    startupSockets();

#if 0
    char hostnameBuffer[256];

    if ( gethostname(hostnameBuffer, sizeof(hostnameBuffer)) == 0 )
	{
		struct hostent *host = gethostbyname(hostnameBuffer);
		if (host != NULL)
		{
			printf( "Hostname %s addresses:\n", hostnameBuffer );
			struct in_addr** pInAddr = (struct in_addr **)(host->h_addr_list);
			int i = 0;
			while ( pInAddr[i] != NULL )
			{
				unsigned char* pAddrBytes = (unsigned char*) pInAddr[i];
				printf( "%d.%d.%d.%d   use %d for forth client address\n",
					pAddrBytes[0], pAddrBytes[1], pAddrBytes[2], pAddrBytes[3],
					*(reinterpret_cast<int32_t*>(pAddrBytes)) );
				i++;
			}
		}
	}
#else
    char hostnameBuffer[256];

    if ( gethostname(hostnameBuffer, sizeof(hostnameBuffer)) == 0 )
	{
		struct addrinfo addrHints;
		struct addrinfo *resultAddrs, *resultAddr;

		printf( "Hostname: %s   Forth server port: %d\n", hostnameBuffer, portNum );
		memset( &addrHints, 0, sizeof(struct addrinfo) );
		addrHints.ai_family = AF_UNSPEC;	    // Allow IPv4 or IPv6
		addrHints.ai_socktype = SOCK_STREAM;	// stream sockets only
		addrHints.ai_protocol = IPPROTO_TCP;	// TCP protocol

		int s = getaddrinfo(hostnameBuffer, NULL, &addrHints, &resultAddrs);
		if (s != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		}
		else
		{

		    for ( resultAddr = resultAddrs; resultAddr != NULL; resultAddr = resultAddr->ai_next )
		    {
				switch ( resultAddr->ai_family )
				{

				case AF_UNSPEC:
					printf("Unspecified\n");
					break;

				case AF_INET:
					{
						//struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in *) resultAddr->ai_addr;
						unsigned char* pAddrBytes = (unsigned char*) (&resultAddr->ai_addr->sa_data[2]);
						printf( "IPv4 %d.%d.%d.%d   use %u for forth client address\n",
							pAddrBytes[0], pAddrBytes[1], pAddrBytes[2], pAddrBytes[3],
							*(reinterpret_cast<int32_t*>(pAddrBytes)) );
					}
					break;

				case AF_INET6:
					{
						unsigned char* pAddrBytes = (unsigned char*) (&resultAddr->ai_addr->sa_data[0]);
						printf( "IPv6 %d.%d.%d.%d.%d.%d\n",
							pAddrBytes[0], pAddrBytes[1], pAddrBytes[2], pAddrBytes[3], pAddrBytes[4], pAddrBytes[5] );
					}
					break;
				}
			}
		    freeaddrinfo( resultAddrs );
		}

	}
#endif

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef WIN32
    if (ServerSocket == INVALID_SOCKET)
#else
    if (ServerSocket == -1)
#endif
    {
        printf("error: unable to create the listening socket, errno=%s\n", strerror( errno ));
    }
    else
    {
        ServerInfo.sin_family = AF_INET;
        ServerInfo.sin_addr.s_addr = INADDR_ANY;
        ServerInfo.sin_port = htons( portNum );
        iRetVal = ::bind(ServerSocket, (struct sockaddr*) &ServerInfo, sizeof(struct sockaddr));
#ifdef WIN32
        if (iRetVal == SOCKET_ERROR)
#else
        if (iRetVal == -1)
#endif
        {
            printf("error: unable to bind listening socket, errno=%s\n", strerror( errno ));
        }
        else
        {
            iRetVal = listen(ServerSocket, 10);
#ifdef WIN32
            if (iRetVal == SOCKET_ERROR)
#else
            if (iRetVal == -1)
#endif
            {
                printf("error: unable to listen on listening socket, errno=%s\n", strerror( errno ));
            }
            else
            {
				Shell* pOldShell = pEngine->GetShell();
                pShell = new ServerShell( doAutoload, pEngine );

				traceOutRoutine oldTraceRoutine;
				void* pOldTraceData;
				pEngine->GetTraceOutRoutine(oldTraceRoutine, pOldTraceData);
				pEngine->SetTraceOutRoutine(serverTraceOutRoutine, pEngine);
				pEngine->SetIsServer(true);

				bool notDone = true;
                while (notDone)
                {
					iRetVal = pShell->ProcessConnection( ServerSocket );
					pShell->CloseConnection();
                }

				pEngine->SetIsServer(false);
				pEngine->GetTraceOutRoutine(oldTraceRoutine, pOldTraceData);
				delete pShell;
				pEngine->SetShell( pOldShell );
            }
        }
    }
#ifdef WIN32
    closesocket(ServerSocket);
#else
    // TODO
    close( ServerSocket );
#endif
    shutdownSockets();
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ServerInputStream::ServerInputStream( Pipe* pPipe, bool isFile, int bufferLen )
:   InputStream( bufferLen )
,   mpMsgPipe( pPipe )
,   mIsFile( isFile )
{
}

ServerInputStream::~ServerInputStream()
{
}

cell ServerInputStream::GetSourceID() const
{
    // this is wrong, but will compile
    return mIsFile ? 1 : 0;
}


char* ServerInputStream::GetLine( const char *pPrompt )
{
    if (mbForcedEmpty)
    {
        return nullptr;
    }

    int msgType, msgLen;
    char* result = NULL;

    mReadOffset = 0;

    mpMsgPipe->StartMessage( kClientMsgSendLine );
    mpMsgPipe->WriteString( mIsFile ? NULL : pPrompt );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    switch ( msgType )
    {
    case kServerMsgProcessLine:
        {
            const char* pSrcLine;
            int srcLen = 0;
            if ( mpMsgPipe->ReadCountedData( pSrcLine, srcLen ) )
            {
                if ( srcLen != 0 )
                {
#ifdef PIPE_SPEW
                    printf( "line = '%s'\n", pSrcLine );
#endif
                    memcpy( mpBufferBase, pSrcLine, srcLen );
                    mWriteOffset = srcLen;
                    result = mpBufferBase;
                }
            }
            else
            {
                // TODO: report error
            }
        }
        break;

    case kServerMsgPopStream:
        break;

    default:
        // TODO: report unexpected message type error
        printf( "ServerShell::GetLine unexpected message type %d\n", msgType );
        break;
    }

    return result;
}

char* ServerInputStream::AddLine()
{
    int msgType, msgLen;
    char* result = NULL;

    mpMsgPipe->StartMessage(kClientMsgSendLine);
    mpMsgPipe->WriteString(nullptr);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage(msgType, msgLen);
    switch (msgType)
    {
    case kServerMsgProcessLine:
    {
        const char* pSrcLine;
        int srcLen = 0;
        if (mpMsgPipe->ReadCountedData(pSrcLine, srcLen))
        {
            if (srcLen != 0)
            {
#ifdef PIPE_SPEW
                printf("line = '%s'\n", pSrcLine);
#endif
                char* pBufferDst = mpBufferBase + mWriteOffset;
                if ((srcLen + mWriteOffset) > mBufferLen)
                {
                    srcLen = (int)((mBufferLen - mWriteOffset) - 1);
                }
                memcpy(pBufferDst, pSrcLine, srcLen);
                pBufferDst[srcLen] = '\0';
                mWriteOffset = srcLen;
                result = mpBufferBase;
            }
        }
        else
        {
            // TODO: report error
        }
    }
    break;

    case kServerMsgPopStream:
        break;

    default:
        // TODO: report unexpected message type error
        printf("ServerShell::GetLine unexpected message type %d\n", msgType);
        break;
    }

    return result;
}

bool ServerInputStream::IsInteractive( void )
{
    return !mIsFile;
}

Pipe* ServerInputStream::GetPipe()
{
    return mpMsgPipe;
}


cell*
ServerInputStream::GetInputState()
{
    // TBD!
    return nullptr;
}

bool
ServerInputStream::SetInputState(cell* pState)
{
    // TBD!
    return false;
}

InputStreamType ServerInputStream::GetType(void) const
{
    return InputStreamType::kServer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ServerShell::ServerShell( bool doAutoload, Engine *pEngine, Extension *pExtension, int shellStackLongs )
:   Shell( 0, nullptr, nullptr, pEngine, pExtension, shellStackLongs )
,   mDoAutoload( doAutoload )
,   mpMsgPipe( NULL )
,	mClientSocket( -1 )
,   mUseLocalFiles(false)
{
    mFileInterface.fileOpen = fileOpen;
    mFileInterface.fileClose = fileClose;
    mFileInterface.fileRead = fileRead;
    mFileInterface.fileWrite = fileWrite;
    mFileInterface.fileGetChar = fileGetChar;
    mFileInterface.filePutChar = filePutChar;
    mFileInterface.fileAtEnd = fileAtEnd;
    mFileInterface.fileExists = fileExists;
    mFileInterface.fileSeek = fileSeek;
    mFileInterface.fileTell = fileTell;
    mFileInterface.fileGetLength = fileGetLength;
    mFileInterface.fileGetString = fileGetString;
    mFileInterface.filePutString = filePutString;
	mFileInterface.fileRemove = fileRemove;
	mFileInterface.fileDup = fileDup;
	mFileInterface.fileDup2 = fileDup2;
	mFileInterface.fileNo = fileNo;
	mFileInterface.fileFlush = fileFlush;
	mFileInterface.renameFile = renameFile;
	mFileInterface.runSystem = runSystem;
	mFileInterface.setWorkDir = setWorkDir;
    mFileInterface.getWorkDir = getWorkDir;
    mFileInterface.makeDir = makeDir;
	mFileInterface.removeDir = removeDir;
	mFileInterface.getStdIn = getStdIn;
	mFileInterface.getStdOut = getStdOut;
	mFileInterface.getStdErr = getStdErr;
	mFileInterface.openDir = serverOpenDir;
	mFileInterface.readDir = serverReadDir;
	mFileInterface.closeDir = serverCloseDir;
	mFileInterface.rewindDir = serverRewindDir;

	mConsoleOutObject = nullptr;
}

ServerShell::~ServerShell()
{
}

void ServerShell::setupFileInterface(bool useLocalFiles)
{
    mUseLocalFiles = useLocalFiles;

    mFileInterface.fileExists = fileExists;
    mFileInterface.fileGetLength = fileGetLength;
    mFileInterface.makeDir = makeDir;
    mFileInterface.getStdIn = getStdIn;
    mFileInterface.getStdOut = getStdOut;
    mFileInterface.getStdErr = getStdErr;
    mFileInterface.openDir = serverOpenDir;
    mFileInterface.readDir = serverReadDir;
    mFileInterface.closeDir = serverCloseDir;
    mFileInterface.rewindDir = serverRewindDir;

    if (mUseLocalFiles)
    {
        mFileInterface.fileOpen = fopen;
        mFileInterface.fileClose = fclose;
        mFileInterface.fileRead = fread;
        mFileInterface.fileWrite = fwrite;
        mFileInterface.fileGetChar = fgetc;
        mFileInterface.filePutChar = fputc;
        mFileInterface.fileAtEnd = feof;
        mFileInterface.fileSeek = fseek;
        mFileInterface.fileTell = ftell;
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
#ifdef WIN32
        mFileInterface.setWorkDir = _chdir;
        mFileInterface.removeDir = _rmdir;
#else
        mFileInterface.setWorkDir = chdir;
        mFileInterface.removeDir = rmdir;
#endif
    }
    else
    {
        mFileInterface.fileOpen = fileOpen;
        mFileInterface.fileClose = fileClose;
        mFileInterface.fileRead = fileRead;
        mFileInterface.fileWrite = fileWrite;
        mFileInterface.fileGetChar = fileGetChar;
        mFileInterface.filePutChar = filePutChar;
        mFileInterface.fileAtEnd = fileAtEnd;
        mFileInterface.fileSeek = fileSeek;
        mFileInterface.fileTell = fileTell;
        mFileInterface.fileGetString = fileGetString;
        mFileInterface.filePutString = filePutString;
        mFileInterface.fileRemove = fileRemove;
        mFileInterface.fileDup = fileDup;
        mFileInterface.fileDup2 = fileDup2;
        mFileInterface.fileNo = fileNo;
        mFileInterface.fileFlush = fileFlush;
        mFileInterface.renameFile = renameFile;
        mFileInterface.runSystem = runSystem;
        mFileInterface.setWorkDir = setWorkDir;
        mFileInterface.removeDir = removeDir;
    }
}

int ServerShell::Run( InputStream *pInputStream )
{
    ServerInputStream* pStream = (ServerInputStream *) pInputStream;
    mpMsgPipe = pStream->GetPipe();

    const char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    OpResult result = OpResult::kOk;
    bool bInteractiveMode = pStream->IsInteractive();

	CoreState* pCore = mpEngine->GetCoreState();
	CreateForthFunctionOutStream( pCore, mConsoleOutObject, NULL, NULL, consoleOutToClient, pCore );
	mpEngine->ResetConsoleOut( *pCore );
    mpInput->PushInputStream( pStream );

    if ( mDoAutoload )
    {
        mpEngine->PushInputFile( "forth_autoload.fs" );
    }

    while ( !bQuit )
    {

        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine( mpEngine->GetFastMode() ? "ok>" : "OK>" );
        if ( pBuffer == NULL )
        {
            bQuit = PopInputStream();
        }

        if ( !bQuit )
        {

            result = ProcessLine();

            switch( result )
            {

            case OpResult::kExitShell:
            case OpResult::kShutdown:
                // users has typed "bye", exit the shell
                bQuit = true;
                retVal = 0;
                mpMsgPipe->StartMessage( kClientMsgGoAway );
                mpMsgPipe->SendMessage();
                break;

            case OpResult::kError:
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
                // a fatal error has occured, exit the shell
                bQuit = true;
                retVal = 1;
                break;

            default:
                break;
            }

        }
    }

	if ( result == OpResult::kShutdown )
	{
		exit( 0 );
	}

    return retVal;
}

bool ServerShell::PushInputFile( const char *pFileName )
{
    int result = 0;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgStartLoad );
    mpMsgPipe->WriteString( pFileName );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgStartLoadResult )
    {
        mpMsgPipe->ReadInt( result );
        if ( result )
        {
            mpInput->PushInputStream( new ServerInputStream( mpMsgPipe, true ) );
        }
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::PushInputFile unexpected message type %d\n", msgType );
    }

    return (result != 0);
}

bool
ServerShell::PopInputStream( void )
{
    //printf("ServerShell::PopInputStream %s  gen:%d   file:%d\n", mpInput->Top()->GetName(),
    //    mpInput->Top()->IsGenerated(), mpInput->Top()->IsFile());
    if (mpInput->Top()->GetType() == InputStreamType::kFile)
    {
        mpMsgPipe->StartMessage(kClientMsgPopStream);
        mpMsgPipe->SendMessage();
    }

    return mpInput->PopInputStream();
}


void ServerShell::SendTextToClient( const char *pMessage )
{
    mpMsgPipe->StartMessage( kClientMsgDisplayText );
    mpMsgPipe->WriteString( pMessage );
    mpMsgPipe->SendMessage();
}

char ServerShell::GetChar()
{
    int c;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgGetChar );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgProcessChar )
    {
        mpMsgPipe->ReadInt( c );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::GetChar unexpected message type %d\n", msgType );
    }
    return (int) c;
}

FILE*
ServerShell::FileOpen( const char* filePath, const char* openMode )
{
    FILE* pFile = NULL;
    cell c;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgFileOpen );
    mpMsgPipe->WriteString( filePath );
    mpMsgPipe->WriteString( openMode );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgHandleResult)
    {
        mpMsgPipe->ReadCell( c );
        pFile = (FILE *) c;
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileOpen unexpected message type %d\n", msgType );
    }
    return pFile;
}

int
ServerShell::FileClose( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileClose );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileClose unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileSeek( FILE* pFile, int offset, int control )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileSetPosition );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->WriteInt( offset );
    mpMsgPipe->WriteInt( control );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileSeek unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileRead( FILE* pFile, void* pDst, int itemSize, int numItems )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileRead );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->WriteInt( itemSize );
    mpMsgPipe->WriteInt( numItems );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileReadResult )
    {
        mpMsgPipe->ReadInt( result );
        if ( result > 0 )
        {
            int numBytes;
            const char* pData;
            mpMsgPipe->ReadCountedData( pData, numBytes );
            if ( numBytes == (result * itemSize) )
            {
                memcpy( pDst, pData, numBytes );
            }
            else
            {
                // TODO: report error
                printf( "ServerShell::FileRead unexpected data size %d != itemsRead %d * itemSize %d\n",
                            numBytes, result, itemSize );
            }
        }
        else
        {
            // TODO: report error
            printf( "ServerShell::FileRead returned %d \n", result );
        }
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileRead unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileWrite( FILE* pFile, const void* pSrc, int itemSize, int numItems ) 
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileWrite );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->WriteInt( itemSize );
    mpMsgPipe->WriteInt( numItems );
    int numBytes = numItems * itemSize;
    mpMsgPipe->WriteCountedData( pSrc, numBytes );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileWrite unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileGetChar( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetChar );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileGetChar unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FilePutChar( FILE* pFile, int outChar )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFilePutChar );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->WriteInt( outChar );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FilePutChar unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileAtEOF( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileCheckEOF );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileAtEOF unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileGetLength( FILE* pFile )
{
    if (mUseLocalFiles)
    {
        return Shell::FileGetLength(pFile);
    }

    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetLength );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileGetLength unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileCheckExists( const char* pFilename )
{
    if (mUseLocalFiles)
    {
        return Shell::FileCheckExists(pFilename);
    }

    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileCheckExists );
    mpMsgPipe->WriteString( pFilename );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileCheckExists unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileGetPosition( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetPosition );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileGetPosition unexpected message type %d\n", msgType );
    }
    return result;
}

char*
ServerShell::FileGetString( FILE* pFile, char* pBuffer, int maxChars )
{
    int msgType, msgLen;
    char* result = NULL;

    if ( maxChars <= 0 )
    {
        return NULL;
    }
    mpMsgPipe->StartMessage( kClientMsgFileGetString );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->WriteInt( maxChars );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileGetStringResult )
    {
        int numBytes;
        const char* pData;
        mpMsgPipe->ReadCountedData( pData, numBytes );
        if ( (numBytes != 0) && (numBytes <= maxChars) )
        {
            memcpy( pBuffer, pData, numBytes );
            result = pBuffer;
        }
        else
        {
            // TODO: report error
            printf( "ServerShell::FileGetString unexpected data size %d\n", numBytes );
        }
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileGetString unexpected message type %d\n", msgType );
    }
    return result;
}


int
ServerShell::FilePutString( FILE* pFile, const char* pBuffer )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFilePutString );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->WriteString( pBuffer );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FilePutString unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileRemove( const char* pBuffer )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgRemoveFile );
    mpMsgPipe->WriteString( pBuffer );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileRemove unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileDup( int fileHandle )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgDupHandle );
    mpMsgPipe->WriteInt( fileHandle );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileDup unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileDup2( int srcFileHandle, int dstFileHandle )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgDupHandle2 );
    mpMsgPipe->WriteInt( srcFileHandle );
    mpMsgPipe->WriteInt( dstFileHandle );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileDup2 unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileNo( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileToHandle );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileNo unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::FileFlush( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileFlush );
    mpMsgPipe->WriteCell((cell)pFile);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::FileFlush unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::RenameFile( const char* pOldName, const char* pNewName )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgRenameFile );
    mpMsgPipe->WriteString( pOldName );
    mpMsgPipe->WriteString( pNewName );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::RenameFile unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::RunSystem( const char* pCmdline )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgRunSystem );
    mpMsgPipe->WriteString( pCmdline );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::RunSystem unexpected message type %d\n", msgType );
    }
    return result;
}

int ServerShell::SetWorkDir( const char* pPath )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgSetWorkDir );
    mpMsgPipe->WriteString( pPath );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::SetWorkDir unexpected message type %d\n", msgType );
    }
    return result;
}

// return actual size of working directory path (including terminating null)
int ServerShell::GetWorkDir(char* pDstPath, int dstPathMax)
{
    if (mUseLocalFiles)
    {
        std::filesystem::path workDir = std::filesystem::current_path();
        memcpy(pDstPath, workDir.string().c_str(), (size_t)dstPathMax - 1);
        pDstPath[dstPathMax - 1] = '\0';
        return workDir.string().length();
    }

    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage(kClientMsgGetWorkDir);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage(msgType, msgLen);
    if (msgType == kServerMsgFileGetStringResult)
    {
        int numBytes;
        const char* pData;
        mpMsgPipe->ReadCountedData(pData, numBytes);
        result = numBytes;
        if (numBytes != 0)
        {
            if (numBytes >= dstPathMax)
            {
                numBytes = dstPathMax - 1;
            }
            memcpy(pDstPath, pData, numBytes);
            pDstPath[numBytes] = '\0';
        }
    }
    else
    {
        // TODO: report error
        printf("ServerShell::GetWorkDir unexpected message type %d\n", msgType);
    }
    return result;
}

int ServerShell::MakeDir( const char* pPath, int mode )
{
    if (mUseLocalFiles)
    {
#ifdef WIN32
        return _mkdir(pPath);
#else
        return mkdir(pPath, mode);
#endif
    }

    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgMakeDir );
    mpMsgPipe->WriteString( pPath );
    mpMsgPipe->WriteInt( mode );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::MakeDir unexpected message type %d\n", msgType );
    }
    return result;
}

int
ServerShell::RemoveDir( const char* pPath )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgRemoveDir );
    mpMsgPipe->WriteString( pPath );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::RemoveDir unexpected message type %d\n", msgType );
    }

    return result;
}

FILE*
ServerShell::GetStdIn()
{
    if (mUseLocalFiles)
    {
        return stdin;
    }

    int msgType, msgLen;
    cell result = -1;
    FILE* pFile = NULL;

    mpMsgPipe->StartMessage( kClientMsgGetStdIn );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgHandleResult)
    {
        mpMsgPipe->ReadCell(result);
        pFile = (FILE *) result;
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::kClientMsgStdIn unexpected message type %d\n", msgType );
    }

    return pFile;
}

FILE*
ServerShell::GetStdOut()
{
    if (mUseLocalFiles)
    {
        return stdout;
    }

    int msgType, msgLen;
    cell result = -1;
    FILE* pFile = NULL;

    mpMsgPipe->StartMessage( kClientMsgGetStdOut );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgHandleResult)
    {
        mpMsgPipe->ReadCell(result);
        pFile = (FILE *) result;
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::kClientMsgStdOut unexpected message type %d\n", msgType );
    }

    return pFile;
}

FILE*
ServerShell::GetStdErr()
{
    if (mUseLocalFiles)
    {
        return stderr;
    }

    int msgType, msgLen;
    cell result = -1;
    FILE* pFile = NULL;

    mpMsgPipe->StartMessage( kClientMsgGetStdErr );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgHandleResult)
    {
        mpMsgPipe->ReadCell(result);
        pFile = (FILE *) result;
    }
    else
    {
        // TODO: report error
        printf( "ServerShell::GetStdErr unexpected message type %d\n", msgType );
    }

    return pFile;
}

void *
ServerShell::OpenDir( const char* pPath )
{
    if (mUseLocalFiles)
    {
        return opendir(pPath);
    }

    int msgType, msgLen;
    cell result = 0;

    mpMsgPipe->StartMessage(kClientMsgOpenDir);
    mpMsgPipe->WriteString(pPath);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage(msgType, msgLen);
    if (msgType == kServerMsgHandleResult)
    {
        mpMsgPipe->ReadCell(result);
    }
    else
    {
        // TODO: report error
        printf("ServerShell::OpenDir unexpected message type %d\n", msgType);
    }
    return (void *)(result);
}

void *
ServerShell::ReadDir(void* pDir, void* pDstEntry)
{
    if (mUseLocalFiles)
    {
        return Shell::ReadDir(pDir, pDstEntry);
    }

    int msgType, msgLen;
    cell result = 0;

    mpMsgPipe->StartMessage(kClientMsgReadDir);
    mpMsgPipe->WriteCell((cell)pDir);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage(msgType, msgLen);
    if (msgType == kServerMsgReadDirResult)
    {
        const char* pSrcBytes;
        int srcLen = 0;
        int expectedSize = (int)sizeof(struct dirent);
        if (mpMsgPipe->ReadCountedData(pSrcBytes, srcLen))
        {
            if (srcLen == expectedSize)
            {
#ifdef PIPE_SPEW
                printf("read direntry\n");
#endif
                memcpy(pDstEntry, pSrcBytes, srcLen);
                result = -1;
            }
        }
        else
        {
            printf("ServerShell::ReadDir unexpected direntry size, expected %d but got %d\n",
                expectedSize, srcLen);
            // TODO: throw exception?
        }
    }
    else
    {
        // TODO: report error
        printf("ServerShell::ReadDir unexpected message type %d\n", msgType);
    }
    return (void *)(result);
}

int
ServerShell::CloseDir( void* pDir )
{
    if (mUseLocalFiles)
    {
        return closedir((DIR*)pDir);
    }

    int msgType, msgLen;
    int result = 0;

    mpMsgPipe->StartMessage(kClientMsgCloseDir);
    mpMsgPipe->WriteCell((cell) pDir);
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage(msgType, msgLen);
    if (msgType == kServerMsgFileOpResult)
    {
        mpMsgPipe->ReadInt(result);
    }
    else
    {
        // TODO: report error
        printf("ServerShell::CloseDir unexpected message type %d\n", msgType);
    }
    return result;
}

void
ServerShell::RewindDir( void* pDir )
{
    if (mUseLocalFiles)
    {
        return rewinddir((DIR*)pDir);
    }

    mpMsgPipe->StartMessage(kClientMsgRewindDir);
    mpMsgPipe->WriteCell((cell)pDir);
    mpMsgPipe->SendMessage();
}

int
ServerShell::ProcessConnection( SOCKET serverSocket )
{
    InputStream *pInStream = NULL;

    printf( "Waiting for a client to connect.\n" );
    mClientSocket = accept( serverSocket, NULL, NULL );
    printf("Incoming connection accepted!\n");

    Pipe* pMsgPipe = new Pipe( mClientSocket, kServerMsgProcessLine, kServerMsgLimit );
    pInStream = new ServerInputStream( pMsgPipe );
	int result = Run( pInStream );
	delete pMsgPipe;
    return result;
}

void
ServerShell::CloseConnection()
{
#ifdef WIN32
	closesocket( mClientSocket );
#else
	close( mClientSocket );
#endif
}
