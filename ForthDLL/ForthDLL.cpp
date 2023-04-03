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

FORTHDLL_API Engine* CreateForthEngine()
{
    return new Engine;
}

FORTHDLL_API Shell* CreateForthShell(int argc, const char** argv, const char** envp, Engine* pEngine, Extension* pExtension, int shellStackLongs)
{
    return new Shell(argc, argv, envp, pEngine, pExtension, shellStackLongs);
}

FORTHDLL_API BufferInputStream* CreateForthBufferInputStream(const char* pDataBuffer, int dataBufferLen, bool deleteWhenEmpty)
{
    BufferInputStream* inStream = new BufferInputStream(pDataBuffer, dataBufferLen, false);
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

FORTHDLL_API ConsoleInputStream* CreateForthConsoleInputStream(int bufferLen, bool deleteWhenEmpty)
{
    ConsoleInputStream* inStream = new ConsoleInputStream(bufferLen);
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

FORTHDLL_API FileInputStream* CreateForthFileInputStream(FILE* pInFile, const char* pFilename, int bufferLen, bool deleteWhenEmpty)
{
    FileInputStream* inStream = new FileInputStream(pInFile, pFilename, bufferLen);
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

FORTHDLL_API void DeleteForthEngine(Engine* pEngine)
{
    delete pEngine;
}

FORTHDLL_API void DeleteForthShell(Shell* pShell)
{
    delete pShell;
}

FORTHDLL_API void DeleteForthFiber(Fiber* pFiber)
{
    delete pFiber;
}

FORTHDLL_API void DeleteForthInputStream(InputStream* pStream)
{
    delete pStream;
}

FORTHDLL_API void InterruptForth()
{
    Engine::GetInstance()->Interrupt();
}

