//////////////////////////////////////////////////////////////////////
//
// ForthPipe.cpp: implementation of the ForthPipe class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

//#define PIPE_SPEW
#include "Forth.h"
#include "ForthPipe.h"

#define FORTH_PIPE_INITIAL_BYTES   		8192
#define FORTH_PIPE_BUFFER_INCREMENT    	4096

#include "ForthMessages.h"

namespace
{

    static const char* clientMsgNames[] =
    {
        "DisplayText",
        "SendLine",
        "StartLoad",
        "PopStream",
        "GetChar",
        "GoAway",
        "FileOpen",
        "FileClose",
        "FileSetPosition",
        "FileRead",
        "FileWrite",
        "FileGetChar",
        "FilePutChar",
        "FileCheckEOF",
        "FileGetPosition",
        "FileGetLength",
        "FileCheckExists",
        "FileGetLine",
        "Limit",
    };

    static const char* serverMsgNames[] =
    {
        "ProcessLine",
        "ProcessChar",
        "PopStream",
        "FileOpResult",
        "Limit"
    };

    const char* MessageName(int msgType)
    {
        if ((msgType >= kClientMsgDisplayText) && (msgType < kClientMsgLimit))
        {
            return clientMsgNames[msgType - kClientMsgDisplayText];
        }

        if ((msgType >= kServerMsgProcessLine) && (msgType < kServerMsgLimit))
        {
            return serverMsgNames[msgType - kServerMsgProcessLine];
        }
        return "WTF!!!????";
    }

    bool socketsNotStarted = true;
}

void startupSockets()
{
#ifdef WIN32
    if (socketsNotStarted)
    {
        //----------------------
        // Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != NO_ERROR)
        {
            printf("Error at WSAStartup()\n");
        }
        else
        {
            socketsNotStarted = false;
        }
    }
#endif
}

void shutdownSockets()
{
#ifdef WIN32
    if (!socketsNotStarted)
    {
        WSACleanup();
        socketsNotStarted = true;
    }
#endif
}

//
//  client/server messages:
//
// #bytes
//   4          message type
//   4		    number of data bytes following (N)
//   N		    message data
//
// Messages are a predefined sequence of message elements.
//
// A message element can be either of fixed or variable size.
//
// A fixed size message element of N bytes is sent in N bytes.
//
// A variable size message element is sent as a 4-byte element size (N),
//    followed by N bytes of message data.
//
// When strings are sent as part of messages, the message includes the terminating null byte.
//

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthPipe
// 

ForthPipe::ForthPipe( SOCKET s, int messageTypeMin, int messageTypeLimit )
:   mSocket( s )
,   mOutBuffer( NULL )
,   mOutOffset( 0 )
,   mOutLen( FORTH_PIPE_INITIAL_BYTES )
,   mInBuffer( NULL )
,   mInOffset( 0 )
,   mInAvailable( 0 )
,   mInLen( FORTH_PIPE_INITIAL_BYTES )
,	mMessageTypeMin( messageTypeMin )
,	mMessageTypeLimit( messageTypeLimit )
{
    mOutBuffer = (char *) __MALLOC( mOutLen );
	mInBuffer = (char *)__MALLOC(mInLen);
}

ForthPipe::~ForthPipe()
{
    __FREE(mOutBuffer);
    __FREE(mInBuffer);
}

void
ForthPipe::StartMessage( int messageType )
{
#ifdef PIPE_SPEW
    printf( "<<<< StartMessage %s  %d\n", MessageName( messageType ), messageType );
#endif
    mOutOffset = 0;
    WriteInt( messageType );
    WriteInt( 0 );
}

void
ForthPipe::WriteData( const void* pData, int numBytes )
{
#ifdef PIPE_SPEW
    printf( "   WriteData %d bytes\n", numBytes );
#endif
    if ( numBytes == 0 )
    {
        return;
    }
    int newOffset = mOutOffset + numBytes;
    if ( newOffset >= mOutLen )
    {
        // resize output buffer
        mOutLen = newOffset + FORTH_PIPE_BUFFER_INCREMENT;
		mOutBuffer = (char *)__REALLOC(mOutBuffer, mOutLen);
    }
    const char* pSrc = (char *) pData;
    char* pDst = &(mOutBuffer[mOutOffset]);

    for ( int i = 0; i < numBytes; i++ )
    {
        *pDst++ = *pSrc++;
    }
    mOutOffset += numBytes;
}

void
ForthPipe::WriteInt( int val )
{
#ifdef PIPE_SPEW
    printf( "   WriteInt %d\n", val );
#endif
    WriteData( &val, sizeof(val) );
}

void
ForthPipe::WriteCell(cell val)
{
#ifdef PIPE_SPEW
#ifdef FORTH64
    printf("   WriteCell %lld\n", val);
#else
    printf("   WriteCell %d\n", val);
#endif
#endif
    WriteData(&val, sizeof(val));
}

void
ForthPipe::WriteCountedData( const void* pSrc, int numBytes )
{
#ifdef PIPE_SPEW
    printf( "   WriteCountedData %d bytes\n", numBytes );
#endif
    WriteInt( numBytes );
    WriteData( pSrc, numBytes );
}

void
ForthPipe::WriteString( const char* pString )
{
#ifdef PIPE_SPEW
    printf( "   WriteString %s\n", (pString == NULL) ? "<NULL>" : pString );
#endif
    int numBytes = (pString == NULL) ? 0 : ((int) strlen( pString ) + 1);
    WriteCountedData( pString, numBytes );
}

void
ForthPipe::SendMessage()
{
    int numBytes = mOutOffset - (2 * sizeof(int));
#ifdef PIPE_SPEW
    printf( ">>>> SendMessage %s, %d bytes\n", MessageName( *((int*)(mOutBuffer)) ), numBytes );
#endif
    // set length field (second int in buffer)
    *((int *)(&(mOutBuffer[sizeof(int)]))) = numBytes;
    send( mSocket, mOutBuffer, mOutOffset, 0 );
    mOutOffset = 0;
}

const void*
ForthPipe::ReceiveBytes( int numBytes )
{
#ifdef PIPE_SPEW
    printf( "ReceiveBytes %d bytes\n", numBytes );
#endif
    if ( numBytes == 0 )
    {
        return NULL;
    }

    int newOffset = mInOffset + numBytes;
    if ( newOffset >= mInLen )
    {
        // resize input buffer
        mInLen = newOffset + FORTH_PIPE_BUFFER_INCREMENT;
		mInBuffer = (char *)__REALLOC(mInBuffer, mInLen);
    }
    char* pDst = &(mInBuffer[ mInOffset ]);
    int bytesRead = 0;
    while ( bytesRead != numBytes )
    {
        bytesRead = recv( mSocket, pDst, numBytes, MSG_PEEK );
#if 0        // this always returns SOCKET_ERROR on NDS until all bytes are received
        if ( bytesRead == SOCKET_ERROR )
        {
#ifdef PIPE_SPEW
            printf( "Socket error in recv!\n" );
#endif
            return NULL;
        }
#endif
    }

    recv( mSocket, pDst, numBytes, 0 );
#ifdef PIPE_SPEW
    for ( int i = 0; i < numBytes; i++ )
    {
        printf( " %02x", (int)(pDst[i] & 0xFF) );
    }
    printf( "\n" );
#endif

    mInOffset = newOffset;
    return pDst;
}

bool
ForthPipe::GetMessage( int& msgTypeOut, int& msgLenOut )
{
    bool responseValid = false;
    mInOffset = 0;
    const char* pSrc = (const char *) ReceiveBytes( 2 * sizeof(int) );
    if ( pSrc == NULL )
    {
        return false;
    }
    int numBytes;
    memcpy( &numBytes, pSrc + sizeof(int), sizeof(numBytes) );
    if ( numBytes < 0 )
    {
#ifdef PIPE_SPEW
        printf( "GetMessage %d bytes\n", numBytes );
#endif
        return false;
    }
    ReceiveBytes( numBytes );

    mInAvailable = mInOffset - (2 * sizeof(int));
    // position read cursor after byte count, before response message type field
    mInOffset = 2 * sizeof(int);

    if ( numBytes >= 0 )
    {
        int messageType;
        memcpy( &messageType, pSrc, sizeof(int) );
        if ( (messageType >= mMessageTypeMin) && (messageType < mMessageTypeLimit) )
        {
#ifdef PIPE_SPEW
            printf( "GetMessage type %s %d, %d bytes\n", MessageName( messageType ), messageType, numBytes );
#endif
            msgTypeOut = messageType;
            msgLenOut = numBytes;
            responseValid = true;
        }
    }
    return responseValid;
}

// return true IFF an int was available
bool
ForthPipe::ReadInt(int& intOut)
{
    if (mInAvailable >= sizeof(int))
    {
        char* pSrc = &(mInBuffer[mInOffset]);
        memcpy(&intOut, pSrc, sizeof(int));
        mInOffset += sizeof(int);
        mInAvailable -= sizeof(int);
        return true;
    }
    return false;
}

// return true IFF an int was available
bool
ForthPipe::ReadCell(cell& cellOut)
{
    if (mInAvailable >= sizeof(cell))
    {
        char* pSrc = &(mInBuffer[mInOffset]);
        memcpy(&cellOut, pSrc, sizeof(cell));
        mInOffset += sizeof(cell);
        mInAvailable -= sizeof(cell);
        return true;
    }
    return false;
}

bool
ForthPipe::ReadCountedData( const char*& pDataOut, int& numBytesOut )
{
    if ( mInAvailable >= sizeof(int) )
    {
        const char* pSrc = &(mInBuffer[ mInOffset ]);
        memcpy( &numBytesOut, pSrc, sizeof(int) );
        pSrc += sizeof(int);
        pDataOut = pSrc;
        mInOffset += (numBytesOut + sizeof(int));
        mInAvailable -= (numBytesOut + sizeof(int));
        return true;
    }
    return false;
}

bool
ForthPipe::ReadString( const char*& pString )
{
    int len;
    return ReadCountedData( pString, len );
}

bool
ForthPipe::ReadData( const char*& pDataOut, int numBytes )
{
    if ( mInAvailable >= numBytes )
    {
        pDataOut = &(mInBuffer[ mInOffset ]);
        mInOffset += numBytes;
        mInAvailable -= numBytes;
        return true;
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

