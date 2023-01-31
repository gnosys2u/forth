#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthPipe.h: interface for the Pipe class.
//
//////////////////////////////////////////////////////////////////////


#if defined(WINDOWS_BUILD)
#include <winsock2.h>
#else
#include <sys/socket.h>
#define SOCKET  int
#endif


//#define PIPE_SPEW

// it is safe to call startupSockets multiple times
extern void startupSockets();
extern void shutdownSockets();

class Pipe
{
public:

	// when you create a Pipe, you specify the range of valid
    //  message types you expect to receive from the pipe
	Pipe( SOCKET s, int messageTypeMin, int messageTypeLimit );
    virtual ~Pipe();

    void StartMessage( int messageType );
    // use this to add a fixed size piece of data to a message
    void WriteData( const void* pData, int numBytes );
    void WriteInt(int val);
    void WriteCell(cell val);
    // use this to add a variable sized piece of data to a message
    void WriteCountedData( const void* pSrc, int numBytes );
    // use this to add a string to a message
    void WriteString( const char* pString );
    // put message in pipe
    void SendMessage();

    // get a complete message from the pipe
    bool GetMessage( int& msgTypeOut, int& msgLenOut );

    // return true IFF an int was available
    bool ReadInt(int& intOut);
    // return true IFF a cell was available
    bool ReadCell(cell& cellOut);
    // return true IFF a variable sized piece of data was avaialable
    bool ReadCountedData( const char*& pDataOut, int& numBytesOut );
    // return true IFF a variable sized piece of data was avaialable
    bool ReadString( const char*& pDataOut );
    // return true IFF data of the specified size was available
    bool ReadData( const char*& pDataOut, int numBytes );

protected:
    const void* ReceiveBytes( int numBytes );

	SOCKET			mSocket;
	char*			mOutBuffer;
	int				mOutOffset;
	int				mOutLen;
	char*			mInBuffer;
	int				mInOffset;
    int             mInAvailable;
	int				mInLen;
	int				mMessageTypeMin;
	int				mMessageTypeLimit;
};


