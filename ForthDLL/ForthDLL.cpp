// ForthDLL.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "ForthDLL.h"


#if 0
// This is an example of an exported variable
FORTHDLL_API int nForthDLL=0;

// This is an example of an exported function.
FORTHDLL_API int fnForthDLL(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CForthDLL::CForthDLL()
{
    return;
}
#endif

static HANDLE hLoggingPipe = INVALID_HANDLE_VALUE;

FORTHDLL_API void OutputToLogger(const char* pBuffer)
{
    //OutputDebugString(buffer);

    DWORD dwWritten;
    int bufferLen = 1 + (int) strlen(pBuffer);

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

FORTHDLL_API ForthEngine* CreateForthEngine()
{
    return new ForthEngine;
}

FORTHDLL_API ForthShell* CreateForthShell(int argc, const char** argv, const char** envp, ForthEngine* pEngine, ForthExtension* pExtension, int shellStackLongs)
{
    return new ForthShell(argc, argv, envp, pEngine, pExtension, shellStackLongs);
}

FORTHDLL_API ForthBufferInputStream* CreateForthBufferInputStream(const char* pDataBuffer, int dataBufferLen, int bufferLen, bool deleteWhenEmpty)
{
    ForthBufferInputStream* inStream = new ForthBufferInputStream(pDataBuffer, dataBufferLen, bufferLen);
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

FORTHDLL_API ForthConsoleInputStream* CreateForthConsoleInputStream(int bufferLen, bool deleteWhenEmpty)
{
    ForthConsoleInputStream* inStream = new ForthConsoleInputStream(bufferLen);
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

FORTHDLL_API ForthFileInputStream* CreateForthFileInputStream(FILE* pInFile, const char* pFilename, int bufferLen, bool deleteWhenEmpty)
{
    ForthFileInputStream* inStream = new ForthFileInputStream(pInFile, pFilename, bufferLen);
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

FORTHDLL_API void DeleteForthEngine(ForthEngine* pEngine)
{
    delete pEngine;
}

FORTHDLL_API void DeleteForthShell(ForthShell* pShell)
{
    delete pShell;
}

FORTHDLL_API void DeleteForthFiber(ForthFiber* pFiber)
{
    delete pFiber;
}

FORTHDLL_API void DeleteForthInputStream(ForthInputStream* pStream)
{
    delete pStream;
}

