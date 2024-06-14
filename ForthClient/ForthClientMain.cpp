// Client.cpp : Defines the entry point for the console application.
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

#include "pch.h"
#include <winsock2.h>

#include "Forth.h"
#include "Shell.h"
#include "Client.h"
#include "ForthMessages.h"

#pragma comment(lib, "wininet.lib")

static HANDLE hLoggingPipe = INVALID_HANDLE_VALUE;

void OutputToLogger(const char* pBuffer)
{
    //OutputDebugString(buffer);

    DWORD dwWritten;
    int bufferLen = 1 + strlen(pBuffer);

    if (hLoggingPipe == INVALID_HANDLE_VALUE)
    {
        hLoggingPipe = CreateFile(TEXT("\\\\.\\pipe\\ForthLogPipe"),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    }
    if (hLoggingPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hLoggingPipe,
            pBuffer,
            bufferLen,   // = length of string + terminating '\0' !!!
            &dwWritten,
            NULL);

        //CloseHandle(hPipe);
    }

    return;
}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	const char* serverStr = (argc > 1) ? argv[1] : "127.0.0.1";
	unsigned short portNum = FORTH_SERVER_PORT;

    Shell* pShell = new Shell(argc, (const char **)(argv), (const char **)envp);

    int retVal = ClientMainLoop(pShell->GetEngine(), serverStr, portNum);

    delete pShell;

    return retVal;
}

