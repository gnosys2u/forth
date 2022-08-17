// ForthServer.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include <string.h>
#include <winsock2.h>
#include "..\ForthLib\ForthShell.h"
#include "..\ForthLib\ForthEngine.h"
#include "..\ForthLib\ForthInput.h"
#include "..\ForthLib\ForthThread.h"
#include "..\ForthLib\ForthPipe.h"
#include "..\ForthLib\ForthServer.h"
#include "..\ForthLib\ForthMessages.h"


using namespace std;


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

//int _tmain(int argc, _TCHAR* argv[])
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    ForthShell* pShell = new ForthShell(argc, (const char **)(argv), (const char **)envp);

	int retVal = ForthServerMainLoop( pShell->GetEngine(), true, FORTH_SERVER_PORT );

    delete pShell;

    return retVal;
}
