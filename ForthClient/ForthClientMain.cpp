// ForthClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <winsock2.h>

#include "Forth.h"
#include "ForthShell.h"
#include "ForthClient.h"
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

    ForthShell* pShell = new ForthShell(argc, (const char **)(argv), (const char **)envp);

    int retVal = ForthClientMainLoop(pShell->GetEngine(), serverStr, portNum);

    delete pShell;

    return retVal;
}

