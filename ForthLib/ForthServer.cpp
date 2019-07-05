//////////////////////////////////////////////////////////////////////
//
// ForthServer.cpp: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#ifdef WIN32
//#include <winsock2.h>
//#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include "ForthServer.h"
#include "ForthPipe.h"
#include "ForthMessages.h"
#include "ForthExtension.h"

#ifndef SOCKADDR
#define SOCKADDR struct sockaddr
#endif

using namespace std;

#ifdef DEBUG_WITH_NDS_EMULATOR
#include <nds.h>
#endif

namespace
{
    void consoleOutToClient( ForthCoreState   *pCore,
							 void             *pData,
                             const char       *pMessage )
    {
    #ifdef DEBUG_WITH_NDS_EMULATOR
	    iprintf( "%s", pMessage );
    #else
        ForthServerShell* pShell = (ForthServerShell *) (((ForthEngine *)(pCore->pEngine))->GetShell());
        pShell->SendTextToClient( pMessage );
    #endif
    }

    FILE* fileOpen( const char* pPath, const char* pAccess )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileOpen( pPath, pAccess );
    }

    int fileClose( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileClose( pFile );
    }

    size_t fileRead( void* data, size_t itemSize, size_t numItems, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileRead( pFile, data, itemSize, numItems );
    }

    size_t fileWrite( const void* data, size_t itemSize, size_t numItems, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileWrite( pFile, data, itemSize, numItems );
    }

    int fileGetChar( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetChar( pFile );
    }

    int filePutChar( int val, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FilePutChar( pFile, val );
    }

    int fileAtEnd( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileAtEOF( pFile );
    }

    int fileExists( const char* pPath )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileCheckExists( pPath );
    }

    int fileSeek( FILE* pFile, long offset, int ctrl )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileSeek( pFile, offset, ctrl );
    }

    long fileTell( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetPosition( pFile );
    }

    int fileGetLength( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetLength( pFile );
    }

    char* fileGetString( char* buffer, int bufferLength, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileGetString( pFile, buffer, bufferLength );
    }

    int filePutString( const char* buffer, FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FilePutString( pFile, buffer );
    }

	int fileRemove( const char* buffer )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileRemove( buffer );
    }

	int fileDup( int fileHandle )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileDup( fileHandle );
    }

	int fileDup2( int srcFileHandle, int dstFileHandle )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileDup2( srcFileHandle, dstFileHandle );
    }

	int fileNo( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileNo( pFile );
    }

	int fileFlush( FILE* pFile )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->FileFlush( pFile );
    }

	int renameFile( const char* pOldName, const char* pNewName )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->RenameFile( pOldName, pNewName );
    }

	int runSystem( const char* pCmdline )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->RunSystem( pCmdline );
    }

	int changeDir( const char* pPath )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->ChangeDir( pPath );
    }

	int makeDir( const char* pPath, int mode )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->MakeDir( pPath, mode );
    }

	int removeDir( const char* pPath )
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->RemoveDir( pPath );
    }

	FILE* getStdIn()
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->GetStdIn();
    }

	FILE* getStdOut()
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->GetStdOut();
    }

	FILE* getStdErr()
    {
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->GetStdErr();
    }

	void* serverOpenDir( const char* pPath )
	{
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->OpenDir( pPath );
	}

	void* serverReadDir( void* pDir )
	{
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->ReadDir( pDir );
	}

	int serverCloseDir( void* pDir )
	{
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        return pShell->CloseDir( pDir );
	}

	void serverRewindDir( void* pDir )
	{
        ForthServerShell* pShell = (ForthServerShell *) (ForthEngine::GetInstance()->GetShell());
        pShell->RewindDir( pDir );
	}

	// trace output in client/server mode
	void serverTraceOutRoutine(void *pData, const char* pFormat, va_list argList)
	{
		(void)pData;

		ForthEngine* pEngine = ForthEngine::GetInstance();
		if ((pEngine->GetTraceFlags() & kLogToConsole) != 0)
		{
			vprintf(pFormat, argList);
		}
#if !defined(LINUX) && !defined(MACOSX)
		else
		{
			TCHAR buffer[1000];
			wvnsprintf(buffer, sizeof(buffer), pFormat, argList);

			OutputDebugString(buffer);
		}
#endif
	}


};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int ForthServerMainLoop( ForthEngine *pEngine, bool doAutoload, unsigned short portNum )
{
    SOCKET ServerSocket;
    struct sockaddr_in ServerInfo;
    int iRetVal = 0;
    ForthServerShell *pShell;

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
					*(reinterpret_cast<long*>(pAddrBytes)) );
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
							*(reinterpret_cast<long*>(pAddrBytes)) );
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
				ForthShell* pOldShell = pEngine->GetShell();
                pShell = new ForthServerShell( doAutoload, pEngine );

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

ForthServerInputStream::ForthServerInputStream( ForthPipe* pPipe, bool isFile, int bufferLen )
:   ForthInputStream( bufferLen )
,   mpMsgPipe( pPipe )
,   mIsFile( isFile )
{
}

ForthServerInputStream::~ForthServerInputStream()
{
}

int ForthServerInputStream::GetSourceID()
{
    // this is wrong, but will compile
    return mIsFile ? 1 : 0;
}


char* ForthServerInputStream::GetLine( const char *pPrompt )
{
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
        printf( "ForthServerShell::GetLine unexpected message type %d\n", msgType );
        break;
    }

    return result;
}

char* ForthServerInputStream::AddContinuationLine()
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
                    srcLen = (mBufferLen - mWriteOffset) - 1;
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
        printf("ForthServerShell::GetLine unexpected message type %d\n", msgType);
        break;
    }

    return result;
}

bool ForthServerInputStream::IsInteractive( void )
{
    return !mIsFile;
}

ForthPipe* ForthServerInputStream::GetPipe()
{
    return mpMsgPipe;
}


long*
ForthServerInputStream::GetInputState()
{
    // TBD!
    return (long *) NULL;
}

bool
ForthServerInputStream::SetInputState( long* pState )
{
    // TBD!
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ForthServerShell::ForthServerShell( bool doAutoload, ForthEngine *pEngine, ForthExtension *pExtension, int shellStackLongs )
:   ForthShell( 0, nullptr, nullptr, pEngine, pExtension, shellStackLongs )
,   mDoAutoload( doAutoload )
,   mpMsgPipe( NULL )
,	mClientSocket( -1 )
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
	mFileInterface.changeDir = changeDir;
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

ForthServerShell::~ForthServerShell()
{
}

int ForthServerShell::Run( ForthInputStream *pInputStream )
{
    ForthServerInputStream* pStream = (ForthServerInputStream *) pInputStream;
    mpMsgPipe = pStream->GetPipe();

    const char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    eForthResult result = kResultOk;
    bool bInteractiveMode = pStream->IsInteractive();

	ForthCoreState* pCore = mpEngine->GetCoreState();
	CreateForthFunctionOutStream( pCore, mConsoleOutObject, NULL, NULL, consoleOutToClient, pCore );
	mpEngine->ResetConsoleOut( pCore );
    mpInput->PushInputStream( pStream );

    if ( mDoAutoload )
    {
        mpEngine->PushInputFile( "forth_autoload.txt" );
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

            case kResultExitShell:
            case kResultShutdown:
                // users has typed "bye", exit the shell
                bQuit = true;
                retVal = 0;
                mpMsgPipe->StartMessage( kClientMsgGoAway );
                mpMsgPipe->SendMessage();
                break;

            case kResultError:
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

            case kResultFatalError:
                // a fatal error has occured, exit the shell
                bQuit = true;
                retVal = 1;
                break;

            default:
                break;
            }

        }
    }
	if ( result == kResultShutdown )
	{
		exit( 0 );
	}

    return retVal;
}

bool ForthServerShell::PushInputFile( const char *pFileName )
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
            mpInput->PushInputStream( new ForthServerInputStream( mpMsgPipe, true ) );
        }
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::PushInputFile unexpected message type %d\n", msgType );
    }

    return (result != 0);
}

bool
ForthServerShell::PopInputStream( void )
{
    mpMsgPipe->StartMessage( kClientMsgPopStream );
    mpMsgPipe->SendMessage();

    return mpInput->PopInputStream();
}


void ForthServerShell::SendTextToClient( const char *pMessage )
{
    mpMsgPipe->StartMessage( kClientMsgDisplayText );
    mpMsgPipe->WriteString( pMessage );
    mpMsgPipe->SendMessage();
}

char ForthServerShell::GetChar()
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
        printf( "ForthServerShell::GetChar unexpected message type %d\n", msgType );
    }
    return (int) c;
}

FILE*
ForthServerShell::FileOpen( const char* filePath, const char* openMode )
{
    FILE* pFile = NULL;
    int c;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgFileOpen );
    mpMsgPipe->WriteString( filePath );
    mpMsgPipe->WriteString( openMode );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( c );
        pFile = (FILE *) c;
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileOpen unexpected message type %d\n", msgType );
    }
    return pFile;
}

int
ForthServerShell::FileClose( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileClose );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileClose unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileSeek( FILE* pFile, int offset, int control )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileSetPosition );
    mpMsgPipe->WriteInt( (int) pFile );
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
        printf( "ForthServerShell::FileSeek unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileRead( FILE* pFile, void* pDst, int itemSize, int numItems )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileRead );
    mpMsgPipe->WriteInt( (int) pFile );
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
                printf( "ForthServerShell::FileRead unexpected data size %d != itemsRead %d * itemSize %d\n",
                            numBytes, result, itemSize );
            }
        }
        else
        {
            // TODO: report error
            printf( "ForthServerShell::FileRead returned %d \n", result );
        }
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileRead unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileWrite( FILE* pFile, const void* pSrc, int itemSize, int numItems ) 
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileWrite );
    mpMsgPipe->WriteInt( (int) pFile );
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
        printf( "ForthServerShell::FileWrite unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileGetChar( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetChar );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileGetChar unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FilePutChar( FILE* pFile, int outChar )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFilePutChar );
    mpMsgPipe->WriteInt( (int) pFile );
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
        printf( "ForthServerShell::FilePutChar unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileAtEOF( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileCheckEOF );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileAtEOF unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileGetLength( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetLength );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileGetLength unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileCheckExists( const char* pFilename )
{
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
        printf( "ForthServerShell::FileCheckExists unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileGetPosition( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileGetPosition );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileGetPosition unexpected message type %d\n", msgType );
    }
    return result;
}

char*
ForthServerShell::FileGetString( FILE* pFile, char* pBuffer, int maxChars )
{
    int msgType, msgLen;
    char* result = NULL;

    if ( maxChars <= 0 )
    {
        return NULL;
    }
    mpMsgPipe->StartMessage( kClientMsgFileGetString );
    mpMsgPipe->WriteInt( (int) pFile );
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
            printf( "ForthServerShell::FileGetString unexpected data size %d\n", numBytes );
        }
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileGetString unexpected message type %d\n", msgType );
    }
    return result;
}


int
ForthServerShell::FilePutString( FILE* pFile, const char* pBuffer )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFilePutString );
    mpMsgPipe->WriteInt( (int) pFile );
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
        printf( "ForthServerShell::FilePutString unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileRemove( const char* pBuffer )
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
        printf( "ForthServerShell::FileRemove unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileDup( int fileHandle )
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
        printf( "ForthServerShell::FileDup unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileDup2( int srcFileHandle, int dstFileHandle )
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
        printf( "ForthServerShell::FileDup2 unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileNo( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileToHandle );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileNo unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::FileFlush( FILE* pFile )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgFileFlush );
    mpMsgPipe->WriteInt( (int) pFile );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::FileFlush unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::RenameFile( const char* pOldName, const char* pNewName )
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
        printf( "ForthServerShell::RenameFile unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::RunSystem( const char* pCmdline )
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
        printf( "ForthServerShell::RunSystem unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::ChangeDir( const char* pPath )
{
    int msgType, msgLen;
    int result = -1;

    mpMsgPipe->StartMessage( kClientMsgChangeDir );
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
        printf( "ForthServerShell::ChangeDir unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::MakeDir( const char* pPath, int mode )
{
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
        printf( "ForthServerShell::MakeDir unexpected message type %d\n", msgType );
    }
    return result;
}

int
ForthServerShell::RemoveDir( const char* pPath )
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
        printf( "ForthServerShell::RemoveDir unexpected message type %d\n", msgType );
    }
    return result;
}

FILE*
ForthServerShell::GetStdIn()
{
    int msgType, msgLen;
    int result = -1;
    FILE* pFile = NULL;

    mpMsgPipe->StartMessage( kClientMsgGetStdIn );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
		pFile = (FILE *) result;
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::kClientMsgStdIn unexpected message type %d\n", msgType );
    }
    return pFile;
}

FILE*
ForthServerShell::GetStdOut()
{
    int msgType, msgLen;
    int result = -1;
    FILE* pFile = NULL;

    mpMsgPipe->StartMessage( kClientMsgGetStdOut );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
		pFile = (FILE *) result;
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::kClientMsgStdOut unexpected message type %d\n", msgType );
    }
    return pFile;
}

FILE*
ForthServerShell::GetStdErr()
{
    int msgType, msgLen;
    int result = -1;
    FILE* pFile = NULL;

    mpMsgPipe->StartMessage( kClientMsgGetStdErr );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgFileOpResult )
    {
        mpMsgPipe->ReadInt( result );
		pFile = (FILE *) result;
    }
    else
    {
        // TODO: report error
        printf( "ForthServerShell::GetStdErr unexpected message type %d\n", msgType );
    }
    return pFile;
}

void *
ForthServerShell::OpenDir( const char* pPath )
{
	return NULL;
}

void *
ForthServerShell::ReadDir( void* pDir )
{
	return NULL;
}

int
ForthServerShell::CloseDir( void* pDir )
{
	return -1;
}

void
ForthServerShell::RewindDir( void* pDir )
{
}

int
ForthServerShell::ProcessConnection( SOCKET serverSocket )
{
    ForthInputStream *pInStream = NULL;

    printf( "Waiting for a client to connect.\n" );
    mClientSocket = accept( serverSocket, NULL, NULL );
    printf("Incoming connection accepted!\n");

    ForthPipe* pMsgPipe = new ForthPipe( mClientSocket, kServerMsgProcessLine, kServerMsgLimit );
    pInStream = new ForthServerInputStream( pMsgPipe );
	int result = Run( pInStream );
	delete pMsgPipe;
    return result;
}

void
ForthServerShell::CloseConnection()
{
#ifdef WIN32
	closesocket( mClientSocket );
#else
	close( mClientSocket );
#endif
}
