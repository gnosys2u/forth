//////////////////////////////////////////////////////////////////////
//
// Client.cpp: Forth client support
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
#include "pch.h"

#if defined(WIN32) || defined(_WIN64)
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <stdio.h>
#if defined(WIN64)
#include <ws2tcpip.h>
#elif defined(WIN32)
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include "Forth.h"
#include "Pipe.h"
#include "Client.h"
#include "ForthMessages.h"
#if defined(WIN64) || defined(WIN32)
#include "sys/dirent.h"
#else
#include <dirent.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 511
#endif

#ifndef SOCKADDR
#define SOCKADDR struct sockaddr
#endif

namespace
{
	void ErrorExit( const char* message )
	{
#if defined(WINDOWS_BUILD)
        shutdownSockets();
#else
		// TODO
#endif
	}
}

static FILE* openForthFile(const char* pPath, const char* pSystemPath)
{
    // it would be good if this could be combined with Shell::OpenForthFile
    // see if file is an internal file, and if so use it
    FILE *pFile = fopen(pPath, "r");
    bool pathIsRelative = true;

#if defined( WIN32 )
    if (strchr(pPath, ':') != NULL)
    {
        pathIsRelative = false;
    }
#elif defined(LINUX) || defined(MACOSX)
    if (*pPath == '/')
    {
        pathIsRelative = false;
    }
#endif
    if ((pFile == NULL) && pathIsRelative)
    {
        char* pAltPath = new char[strlen(pSystemPath) + strlen(pPath) + 16];
        strcpy(pAltPath, pSystemPath);
#if defined( WIN32 )
        strcat(pAltPath, "\\system\\");
#elif defined(LINUX) || defined(MACOSX)
        strcat(pAltPath, "/system/");
#endif
        strcat(pAltPath, pPath);
        pFile = fopen(pAltPath, "r");
        delete[] pAltPath;
    }
    return pFile;
}


int ClientMainLoop( Engine *pEngine, const char* pServerStr, unsigned short portNum )
{
    char workingDirPath[MAX_PATH + 1];
    char errorMessage[128];
	uint32_t ipAddress = 0;
#if defined(WINDOWS_BUILD)
    startupSockets();
#else
		// TODO
#endif

#if defined(WINDOWS_BUILD)
    DWORD result = GetCurrentDirectoryA(MAX_PATH, workingDirPath);
    if (result == 0)
    {
        workingDirPath[0] = '\0';
    }
#elif defined(LINUX) || defined(MACOSX)
    if (getcwd(workingDirPath, MAX_PATH) == NULL)
    {
        // failed to get current directory
        strcpy(workingDirPath, ".");
    }
#endif

#if defined(WIN64)
    addrinfo* pAddrInfo = nullptr;
    int res = getaddrinfo(pServerStr, nullptr, nullptr, &pAddrInfo);
	//struct hostent *host = gethostbyname(pServerStr);
    if (res != 0)
    {
        sprintf(errorMessage, "Error at getaddrinfo(): %ld", WSAGetLastError());
        ErrorExit(errorMessage);
        return -1;
    }

    for (addrinfo* ptr = pAddrInfo; ptr != nullptr; ptr = ptr->ai_next)
    {
        if (ptr->ai_family == AF_INET)
        {
            struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
            ipAddress = *( (uint32_t *) &(sockaddr_ipv4->sin_addr));
            //WSAStringToAddress(sockaddr_ipv4->sin_addr, AF_INET, protoInfo, addrOut, addrOutLen);
            break;
        }
    }

    if (ipAddress == 0)
    {
        sprintf(errorMessage, "host %s not found", pServerStr);
        ErrorExit(errorMessage);
        return -1;
    }

    //*((uint32_t *)(host->h_addr_list[0]));
#else
	struct hostent *host = gethostbyname(pServerStr);
	ipAddress = *((uint32_t *)(host->h_addr_list[0]));
#endif
	printf( "Connecting to host %s (%d) on port %d\n", pServerStr, ipAddress, portNum );

    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    //ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
#if defined(WINDOWS_BUILD)
    if (ConnectSocket == INVALID_SOCKET)
    {
        sprintf( errorMessage, "Error at socket(): %ld", WSAGetLastError() );
        ErrorExit( errorMessage );
        return -1;
    }
#else
    if (ConnectSocket == -1)
    {
        sprintf( errorMessage, "Error at socket(): %d", errno );
        ErrorExit( errorMessage );
        return -1;
    }
#endif

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService; 
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = ipAddress;
    clientService.sin_port = htons( portNum );

    //----------------------
    // Connect to server.
#if defined(WINDOWS_BUILD)
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
        sprintf( errorMessage, "Failed to connect." );
        ErrorExit( errorMessage );
        return -1;
    }
#else
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) != 0 ) {
        sprintf( errorMessage, "Failed to connect." );
        ErrorExit( errorMessage );
        return -1;
    }
#endif

    Pipe* pMsgPipe = new Pipe( ConnectSocket, kClientMsgDisplayText, kClientMsgLimit );
    bool done = false;
    char buffer[ 1024 ];
    int readBufferSize = 16384;
    char* pReadBuffer = (char *)__MALLOC( readBufferSize );
    FILE*   inputStack[ 16 ];
    int inputStackDepth = 0;
    inputStack[ 0 ] = stdin;
    while ( !done )
    {
        int msgLen, msgType;
        if ( pMsgPipe->GetMessage( msgType, msgLen ) )
        {
            switch( msgType )
            {
            case kClientMsgSendLine:
                {
                    // SendLine params:
                    //   string     prompt
                    const char* pPrompt;
                    int promptLen;
                    if ( pMsgPipe->ReadCountedData( pPrompt, promptLen ) )
                    {
                        if ( promptLen != 0 )
                        {
                            printf( "\n%s ", pPrompt );
                        }
                        // read a line of text from top stream on inputStack and send to server
                        char *pBuffer;
                        buffer[0] = '\0';
                        pBuffer = fgets( buffer, sizeof(buffer), inputStack[ inputStackDepth ] );
                        if ( pBuffer != NULL )
                        {
                            char* pNewline = strchr( buffer, '\r' );
                            if ( pNewline != NULL )
                            {
                                *pNewline = '\0';
                            }
                            pMsgPipe->StartMessage( kServerMsgProcessLine );
                            pMsgPipe->WriteString( buffer );
                            pMsgPipe->SendMessage();
                        }
                        else
                        {
                            // this input stream is empty, pop the input stack and let server know
                            pMsgPipe->StartMessage( kServerMsgPopStream );
                            pMsgPipe->SendMessage();
                            fclose( inputStack[ inputStackDepth ] );
                            inputStackDepth--;
                        }
                    }
                    else
                    {
                        printf( "Failed reading prompt string while processing SendLine\n" );
                        done = true;
                    }
                }
                break;

            case kClientMsgStartLoad:
                {
                    const char* pFilename;
                    pMsgPipe->ReadString( pFilename );
                    FILE* newInputFile = openForthFile( pFilename, workingDirPath );
                    int itWorked = 0;
                    if ( newInputFile != NULL )
                    {
                        inputStackDepth++;
                        inputStack[ inputStackDepth ] = newInputFile;
                        itWorked = 1;
                    }
                    else
                    {
                        printf( "Client: failed to open '%s' upon server request!\n", pFilename );
                    }
                    pMsgPipe->StartMessage( kServerMsgStartLoadResult );
                    pMsgPipe->WriteInt( itWorked );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgPopStream:
                {
                    // a "loaddone" was executed on server, so pop input stream
                    if ( inputStackDepth > 0 )
                    {
                        fclose( inputStack[ inputStackDepth ] );
                        inputStackDepth--;
                    }
                }
                break;

            case kClientMsgDisplayText:
                {
                    const char* pText;
                    int textLen;
                    if ( pMsgPipe->ReadCountedData( pText, textLen ) )
                    {
                        if ( textLen != 0 )
                        {
                            printf( "%s", pText );
                        }
                    }
                    else
                    {
                        printf( "Failed reading text string while processing DisplayText\n" );
                        done = true;
                    }
                }
                break;

            case kClientMsgGetChar:
                {
                    int c = getchar();
                    pMsgPipe->StartMessage( kServerMsgProcessChar );
                    pMsgPipe->WriteInt( c );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGoAway:
                done = true;
                break;

            case kClientMsgFileOpen:
                {
                    const char* pFilename;
                    const char* accessMode;
                    pMsgPipe->ReadString( pFilename );
                    pMsgPipe->ReadString( accessMode );
                    FILE* pFile = fopen( pFilename, accessMode );

                    pMsgPipe->StartMessage(kServerMsgHandleResult);
                    pMsgPipe->WriteCell( (cell) pFile );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileClose:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
                    int result = fclose( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileSetPosition:
                {
                    cell file;
                    int offset, control;
                    pMsgPipe->ReadCell( file );
                    pMsgPipe->ReadInt( offset );
                    pMsgPipe->ReadInt( control );
                    int result = fseek( (FILE *) file, offset, control );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileRead:
                {
                    cell file;
                    int numItems, itemSize;
                    pMsgPipe->ReadCell( file );
                    pMsgPipe->ReadInt( itemSize );
                    pMsgPipe->ReadInt( numItems );
                    int numBytes = numItems * itemSize;
                    if ( numBytes > readBufferSize )
                    {
						pReadBuffer = (char *)__REALLOC(pReadBuffer, numBytes);
                        readBufferSize = numBytes;
                    }
                    int result = (int) fread( pReadBuffer, itemSize, numItems, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileReadResult );
                    pMsgPipe->WriteInt( result );
                    if ( result > 0 )
                    {
                        numBytes = result * itemSize;
                        pMsgPipe->WriteCountedData( pReadBuffer, numBytes );
                    }
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileWrite:
                {
                    cell file;
                    int numItems, itemSize, numBytes;
                    const char* pData;
                    pMsgPipe->ReadCell( file );
                    pMsgPipe->ReadInt( itemSize );
                    pMsgPipe->ReadInt( numItems );
                    pMsgPipe->ReadCountedData( pData, numBytes );
                    int result = (int)fwrite( pData, itemSize, numItems, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetChar:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
                    int result = fgetc( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileCheckEOF:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
                    int result = feof( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetLength:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
                    FILE* pFile = (FILE *) file;
                    int oldPos = ftell( pFile );
                    fseek( pFile, 0, SEEK_END );
                    int result = ftell( pFile );
                    fseek( pFile, oldPos, SEEK_SET );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileCheckExists:
                {
                    const char* pFilename;
                    pMsgPipe->ReadString( pFilename );
                    FILE* pFile = fopen( pFilename, "r" );
                    int result = (pFile != NULL) ? ~0 : 0;
                    if ( pFile != NULL )
                    {
                        fclose( pFile );
                    }

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetPosition:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
                    int result = ftell( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetString:
                {
                    cell file;
                    int maxChars;
                    pMsgPipe->ReadCell( file );
                    pMsgPipe->ReadInt( maxChars );
                    if ( maxChars > readBufferSize )
                    {
						pReadBuffer = (char *)__REALLOC(pReadBuffer, maxChars);
                        readBufferSize = maxChars;
                    }
                    char* result = fgets( pReadBuffer, maxChars, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileGetStringResult );
                    pMsgPipe->WriteString( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFilePutString:
                {
                    cell file;
                    const char* pString;
                    pMsgPipe->ReadCell( file );
                    pMsgPipe->ReadString( pString );
                    int result = fputs( pString, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

			case kClientMsgFilePutChar:
			{
				cell file;
				int ch;
				pMsgPipe->ReadCell(file);
				pMsgPipe->ReadInt(ch);
				int result = fputc(ch, (FILE *)file);

				pMsgPipe->StartMessage(kServerMsgFileOpResult);
				pMsgPipe->WriteInt(result);
				pMsgPipe->SendMessage();
			}
			break;

			case kClientMsgFileFlush:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
                    int result = fflush( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRemoveFile:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = remove( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgDupHandle:
                {
                    int fileHandle;
                    pMsgPipe->ReadInt( fileHandle );
#if defined(WINDOWS_BUILD)
                    int result = _dup( fileHandle );
#else
                    int result = dup( fileHandle );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

			case kClientMsgDupHandle2:
                {
                    int srcFileHandle;
                    int dstFileHandle;
                    pMsgPipe->ReadInt( srcFileHandle );
                    pMsgPipe->ReadInt( dstFileHandle );
#if defined(WINDOWS_BUILD)
                    int result = _dup2( srcFileHandle, dstFileHandle );
#else
                    int result = dup2( srcFileHandle, dstFileHandle );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileToHandle:
                {
                    cell file;
                    pMsgPipe->ReadCell( file );
#if defined(WINDOWS_BUILD)
                    int result = _fileno( (FILE *) file );
#else
                    int result = fileno( (FILE *) file );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRenameFile:
                {
                    const char* pOldName;
                    const char* pNewName;
                    pMsgPipe->ReadString( pOldName );
                    pMsgPipe->ReadString( pNewName );
                    int result = rename( pOldName, pNewName );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRunSystem:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = system( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgSetWorkDir:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = chdir( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetWorkDir:
            {
                // TODO!
                /*
                const char* pString;
                pMsgPipe->ReadString(pString);
                int result = chdir(pString);

                pMsgPipe->StartMessage(kServerMsgFileOpResult);
                pMsgPipe->WriteInt(result);
                pMsgPipe->SendMessage();
                */
            }
            break;

            case kClientMsgMakeDir:
                {
                    const char* pString;
					int mode;
                    pMsgPipe->ReadString( pString );
                    pMsgPipe->ReadInt( mode );
#if defined(WINDOWS_BUILD)
                    int result = mkdir( pString );
#else
                    int result = mkdir( pString, mode );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRemoveDir:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = rmdir( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetStdIn:
				{
                    pMsgPipe->StartMessage(kServerMsgHandleResult);
                    pMsgPipe->WriteCell((cell)stdin);
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetStdOut:
				{
                    pMsgPipe->StartMessage(kServerMsgHandleResult);
                    pMsgPipe->WriteCell((cell)stdout);
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetStdErr:
				{
                    pMsgPipe->StartMessage(kServerMsgHandleResult);
                    pMsgPipe->WriteCell((cell)stderr);
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgOpenDir:
                {
                    const char* pString;
                    pMsgPipe->ReadString(pString);
                    cell result = (cell) opendir(pString);

                    pMsgPipe->StartMessage(kServerMsgHandleResult);
                    pMsgPipe->WriteCell((cell)result);
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgReadDir:
                {
                    cell pDir;
                    struct dirent* pEntry = nullptr;
                    pMsgPipe->ReadCell(pDir);
                    if (pDir)
                    {
                        pEntry = readdir((DIR *)pDir);
                    }

                    pMsgPipe->StartMessage(kServerMsgReadDirResult);
                    pMsgPipe->WriteCountedData(pEntry, (pEntry == nullptr) ? 0 : (int)sizeof(struct dirent));
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgCloseDir:
                {
                    cell pDir;
                    pMsgPipe->ReadCell(pDir);
                    int result = closedir((DIR *)pDir);

                    pMsgPipe->StartMessage(kServerMsgFileOpResult);
                    pMsgPipe->WriteInt(result);
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRewindDir:
                {
                    cell pDir;
                    pMsgPipe->ReadCell(pDir);
                    rewinddir((DIR *)pDir);
                }
                break;

            default:
                {
                    printf( "Got unexpected message, type %d len %d \n", msgType, msgLen );
                }
                break;
            }
        }
        else
        {
            done = true;
        }

    }   // end     while ( !done )

#if defined(WINDOWS_BUILD)
    shutdownSockets();
#else
    // TODO
#endif
    __FREE( pReadBuffer );
    delete pMsgPipe;
    return 0;
}
