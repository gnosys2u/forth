#pragma once
//////////////////////////////////////////////////////////////////////
//
// Pipe.h: interface for the Pipe class.
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


